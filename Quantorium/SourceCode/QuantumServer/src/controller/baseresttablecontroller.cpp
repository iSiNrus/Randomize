#include "baseresttablecontroller.h"
#include <QSqlError>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDate>
#include "../core/api.h"
#include "../core/appconst.h"
#include "../utils/qsqlqueryhelper.h"

QString encaseValue(QString strVal, QVariant::Type type)
{
  switch (type) {
  case QVariant::String:
  case QVariant::Date:
  case QVariant::Time:
  case QVariant::DateTime:
    return "'" + strVal + "'";
  default:
    return strVal;
  }
}

BaseRestTableController::BaseRestTableController(QString tableName, Context &con) : BaseJsonActionController(con)
{
  _tableName = tableName;
  registerAction(new InsertAction(_tableName, _tableName + "_insert", "Добавление новой записи в таблицу", _fieldFilters));
  registerAction(new UpdateAction(_tableName, _tableName + "_update", "Обновление записи в таблице", _fieldFilters));
  registerAction(new DeleteAction(_tableName, _tableName + "_delete", "Удаление записи из таблицы", _fieldFilters));
}

void BaseRestTableController::setIdField(QString name)
{
  _idField = name;
}

void BaseRestTableController::setAttachedField(QString name, QString query)
{
  _attachedFields.insert(name, query);
}

bool BaseRestTableController::hasAccess()
{
  return true;
}

QString BaseRestTableController::sqlTableName()
{
  return "\"" + _tableName + "\"";
}

QString BaseRestTableController::filterFromParams(const QSqlRecord &infoRec)
{
  QString filter = "";
  foreach (QString key, params().keys()) {
    QString paramName = key;
    QString paramVal = params().value(key).toString();
    if (infoRec.contains(paramName)) {
      QSqlField filterField = infoRec.field(paramName);
      filter.append(filterExpression(filterField.name(), paramVal, filterField.type()));
    }
  }
  return filter;
}

QString BaseRestTableController::filterExpression(QString colName, QString strExpr, QVariant::Type type)
{
  QString result = "";
  if (type == QVariant::String) {
    //Фильтрация строк (можно использовать условие с LIKE)
    QString oper = strExpr.contains(SQL_LIKE_PLACEHOLDER) ? " LIKE " : "=";
    result = " and " + colName + oper + QSqlQueryHelper::encaseValue(strExpr, type);
  }
  else if (type == QVariant::Int || type == QVariant::LongLong) {
    //Фильтрация для целых чисел
    bool resOk = false;
    QRegExp rx("([0-9]*)\\.\\.([0-9]*)");
    strExpr.toInt(&resOk);
    if (resOk) {
      result = " and " + colName + "=" + QSqlQueryHelper::encaseValue(strExpr, type);
    }
    else if (rx.exactMatch(strExpr)) {
      QString fromInt = rx.cap(1);
      QString toInt = rx.cap(2);
      if (!fromInt.isEmpty()) {
        result += " and " + colName + ">=" + QSqlQueryHelper::encaseValue(fromInt, type);
      }
      if (!toInt.isEmpty()) {
        result += " and " + colName + "<=" + QSqlQueryHelper::encaseValue(toInt, type);
      }
    }
    else if (strExpr.contains(",")) {
      result += " and " + colName + " in (" + strExpr + ")";
    }
    else {
      qWarning() << "Unsupported filter value:" << strExpr;
    }
  }
  else if (type == QVariant::Date) {
    //Фильтрация для даты
    QRegExp rx("(\\d{4}-\\d{2}-\\d{2})?\\.\\.(\\d{4}-\\d{2}-\\d{2})?");
    bool resOk = QDate::fromString(strExpr, DATE_FORMAT).isValid();
    if (resOk) {
      result += " and " + colName + "=" + QSqlQueryHelper::encaseValue(strExpr, type);
    }
    else if (rx.exactMatch(strExpr)) {
      QString fromDate = rx.cap(1);
      QString toDate = rx.cap(2);
      if (!fromDate.isEmpty())
        result += " and " + colName + ">=" + QSqlQueryHelper::encaseValue(fromDate, type);
      if (!toDate.isEmpty())
        result += " and " + colName + "<=" + QSqlQueryHelper::encaseValue(toDate, type);
    }
    else if (strExpr.contains(",")) {
      result += " and " + colName + " in ('" + strExpr.split(",").join("','") + "')";
    }
    else {
      qWarning() << "Unsupported filter value";
    }
  }
  else if (type == QVariant::DateTime) {
    //Фильтрация для даты и времени
    QRegExp rx("(\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2})?\\.\\.(\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2})?");
    bool resOk = QDateTime::fromString(strExpr, DATETIME_FORMAT).isValid();
    if (resOk) {
      result += " and " + colName + "=" + QSqlQueryHelper::encaseValue(strExpr, type);
    }
    else if (rx.exactMatch(strExpr)) {
      QString fromDateTime = rx.cap(1);
      QString toDateTime = rx.cap(2);
      if (!fromDateTime.isEmpty())
        result += " and " + colName + ">=" + QSqlQueryHelper::encaseValue(fromDateTime, type);
      if (!toDateTime.isEmpty())
        result += " and " + colName + "<=" + QSqlQueryHelper::encaseValue(toDateTime, type);
    }
  }
  else if (type == QVariant::Bool) {
    //Можно использовать 1/0 или true/false
    result += " and " + colName + "=" + strExpr + "::boolean";
  }
  else {
    //Строгая фильтрация по значению для остальных типов
    result += " and " + colName + "=" + QSqlQueryHelper::encaseValue(strExpr, type);
  }
  return result;
}

void BaseRestTableController::listAction(HttpResponse &response, Context &con)
{
  QSqlDatabase db = QSqlDatabase::database(con.uid());
  QSqlRecord infoRec = db.record(_tableName);
  QSqlQuery query(db);
  QString sql = "select %1 from %2 t where 1=1";
  QString path = paramString(PRM_PATH);
  qlonglong id = path.section("/", -1).toLongLong();
  //Если последняя секция url число, возвращаем объект с таким id (а параметры игнорируем)
  if (id > 0) {
    sql.append(" and id=").append(QString::number(id));
  }
  else {
    sql.append(filterFromParams(infoRec));
    sql.append(" order by id");
  }  

  if (!query.prepare(sql.arg(fieldsString()).arg(sqlTableName())) || !execSql(query)){
    writeError(response, query.lastError().databaseText(), ERR_DATABASE);
    return;
  }
  QJsonDocument jDoc;
  if (id >0) {
    QJsonObject jObj;
    if (query.next()) {
      QSqlRecord resRec = query.record();
      jObj = sqlRecordToJson(resRec);
    }
    jDoc = QJsonDocument(jObj);
  }
  else {
    jDoc = sqlResultToJson(query);
  }
  response.write(jDoc.toJson(), true);
}

QString BaseRestTableController::fieldsString()
{
  if (_attachedFields.isEmpty())
    return "*";
  QStringList fields;
  fields << "*";
  foreach (QString field, _attachedFields.keys()) {
    fields.append(_attachedFields[field].append(" \"").append(field).append("\""));
  }
  return fields.join(", ");
}

void BaseRestTableController::service(HttpRequest &request, HttpResponse &response)
{
  if (request.getMethod() == "GET" && !request.getPath().endsWith(ACT_INFO_PATH)) {
    BaseJsonSqlController::service(request, response);
    if (response.getStatusCode() != HTTP_NO_ERROR)
      return;
    listAction(response, _context);
  }
  else
    BaseJsonActionController::service(request, response);
}

AbstractTableAction::AbstractTableAction(QString table, QString name, QString description, QStringList fieldFilters)
  : AbstractJsonAction(name, description)
{
  _tableName = table;
  _fieldFilters = fieldFilters;
}

QString AbstractTableAction::sqlTableName()
{
  return "\"" + _tableName + "\"";
}

QJsonObject AbstractTableAction::recordObjById(const Context &con, int id)
{
  QString sql = "select * from %1 where id=%2";
  QSqlRecord rec = QSqlQueryHelper::getSingleRow(sql.arg(sqlTableName()).arg(id), con.uid());
  return sqlRecordToJson(rec);
}

InsertAction::InsertAction(QString table, QString name, QString description, QStringList fieldFilters) : AbstractTableAction(table, name, description, fieldFilters)
{
}

ActionResult InsertAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(params) 

  QJsonObject jObj = bodyData.toObject();
  QSqlQuery query(QSqlDatabase::database(con.uid()));
  QSqlQueryHelper::prepareInsertQuery(query, sqlTableName(), jObj);
  if (!execSql(query)) {
    return handleDatabaseError(query.lastError());
  }
  else {
    query.next();
    return ActionResult(SInsertResult.arg(_tableName), 0, sqlRecordToJson(query.record()));
  }
}

bool InsertAction::checkUrlMatch(QString path, QString method)
{
  Q_UNUSED(path)
  return method == "POST";
}

UpdateAction::UpdateAction(QString table, QString name, QString description, QStringList fieldFilters) : AbstractTableAction(table, name, description, fieldFilters)
{
}

ActionResult UpdateAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(params)
  QJsonObject bodyObj = bodyData.toObject();
  QString path = params.value(PRM_PATH).toString();
  qlonglong id = path.section("/", -1).toLongLong();
  bodyObj.insert("id", id);
  QSqlQuery query = QSqlQuery(QSqlDatabase::database(con.uid()));
  QSqlQueryHelper::prepareUpdateQuery(query, sqlTableName(), bodyObj);
  if (!execSql(query)) {
    return handleDatabaseError(query.lastError());
  }
  return ActionResult(SUpdateResult.arg(_tableName).arg(QString::number(id)));
}

bool UpdateAction::checkUrlMatch(QString path, QString method)
{
  Q_UNUSED(path)
  return method == "PUT";
}

DeleteAction::DeleteAction(QString table, QString name, QString description, QStringList fieldFilters) : AbstractTableAction(table, name, description, fieldFilters)
{
}

ActionResult DeleteAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  QString path = params.value(PRM_PATH).toString();
  qlonglong id = path.section("/", -1).toLongLong();
  QSqlQuery query(QSqlDatabase::database(con.uid()));
  QSqlQueryHelper::prepareDeleteQuery(query, sqlTableName(), id);
  if (!execSql(query)) {
    return handleDatabaseError(query.lastError());
  }
  return ActionResult(SDeleteResult.arg(_tableName, QString::number(id)));
}

bool DeleteAction::checkUrlMatch(QString path, QString method)
{
  Q_UNUSED(path)
  return method == "DELETE";
}
