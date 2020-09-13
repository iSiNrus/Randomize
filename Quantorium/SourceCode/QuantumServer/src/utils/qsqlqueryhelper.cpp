#include "qsqlqueryhelper.h"
#include <QMetaProperty>
#include <QRegExp>
#include <QSqlRecord>
#include <QJsonObject>
#include <QJsonValue>

#define PRM_BRACKET "#"

bool QSqlQueryHelper::_logging = false;

QSqlQuery QSqlQueryHelper::execSql(QString sql, QString connectionName)
{
  if (_logging)
    qDebug() << QString("SQL(%1): %2").arg(connectionName).arg(sql);

  QSqlQuery sqlResult = QSqlDatabase::database(connectionName).exec(sql);  
  if (sqlResult.lastError().isValid() && _logging){
    qWarning() << "Error" << sqlResult.lastError().text();
  }
  return sqlResult;
}

QSqlQuery QSqlQueryHelper::execSql(QString sql, QSqlDatabase &db)
{
  if (_logging)
    qDebug() << QString("SQL(%1): %2").arg(db.connectionName()).arg(sql);

  QSqlQuery sqlResult = db.exec(sql);
  if(sqlResult.lastError().isValid()&& _logging){
    qWarning() << "Error" << sqlResult.lastError().text();
  }
  return sqlResult;
}

qlonglong QSqlQueryHelper::getCurrentSequenceValue(QString sequenceName, QString connectionName)
{
  QString sql = "SELECT gen_id(%1, 0) FROM rdb$database";
  return getSingleValue(sql.arg(sequenceName), connectionName).toLongLong();
}

QVariant QSqlQueryHelper::getSingleValue(QString sql, QString connectionName)
{
  QSqlQuery query = execSql(sql, connectionName);
  return query.next() ? query.value(0) : QVariant();
}

QVariant QSqlQueryHelper::getSingleValue(QString sql, QSqlDatabase &db)
{
  QSqlQuery query = execSql(sql, db);
  return query.next() ? query.value(0) : QVariant();
}

QSqlRecord QSqlQueryHelper::getSingleRow(QString sql, QString connectionName)
{
  QSqlQuery query = execSql(sql, connectionName);
  if (query.next()) {
    return query.record();
  }
  else {
    return QSqlRecord();
  }
}

QString QSqlQueryHelper::databaseName(QString connection)
{
  QSqlDatabase dbCon = QSqlDatabase::database(connection, false);
  Q_ASSERT(dbCon.isValid());
  return dbCon.databaseName();
}

QString QSqlQueryHelper::driverName(QString connection)
{
  QSqlDatabase dbCon = QSqlDatabase::database(connection, false);
  Q_ASSERT(dbCon.isValid());
  return dbCon.driverName();
}

QSqlRecord QSqlQueryHelper::tableRowInfo(QString table, QString connectionName)
{
  QSqlRecord rec = QSqlDatabase::database(connectionName).record(table);
  qDebug() << "Table info:" << rec;
  return rec;
}


QString QSqlQueryHelper::fillSqlPattern(QString pattern, QObject *object)
{
  QRegExp rx = QRegExp(PRM_BRACKET "([A-Za-z]+)" PRM_BRACKET);
  QStringList fields;
  int index = 0;
  while (index >= 0){
    index = rx.indexIn(pattern, index);
    if (index >= 0){
      fields.append(rx.cap(1));
      index += rx.cap().length();
    }
  }
  fields.removeDuplicates();
  QString result(pattern);
  foreach(QString field, fields){
    result = result.replace(PRM_BRACKET+field+PRM_BRACKET, object->property(qPrintable(field)).toString());
  }
  return result;
}

QString QSqlQueryHelper::fillSqlPattern(QString pattern, const QVariantMap &params)
{
  qDebug() << "Params:" << params;
  foreach (QString paramName, params.keys()) {
    pattern.replace(PRM_BRACKET + paramName + PRM_BRACKET, params.value(paramName).toString());
  }
  return pattern;
}

void QSqlQueryHelper::fillObjectFromRecord(QObject *object, QSqlRecord &rec)
{  
  for (int i=0; i<rec.count(); i++){

    object->setProperty(qPrintable(rec.fieldName(i)), rec.value(i));
  }
}

void QSqlQueryHelper::prepareInsertQuery(QSqlQuery &query, QString tableName, const QJsonObject &jObj)
{
  QString sql = "insert into %1(%2) values (%3) returning *";
  QStringList fields;
  QVariantList values;
  QStringList placeholders;
  foreach (QString field, jObj.keys()) {
    fields.append("\"" + field + "\"");
    values.append(jObj.value(field).toVariant());
    placeholders.append("?");
  }
  sql = sql.arg(tableName).arg(fields.join(",")).arg(placeholders.join(","));
  query.prepare(sql);
  foreach (QVariant value, values) {
    query.addBindValue(value);
  }
}

QString QSqlQueryHelper::encaseValue(QString strVal, QVariant::Type type)
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

void QSqlQueryHelper::prepareUpdateQuery(QSqlQuery &query, QString tableName, const QJsonObject &jObj)
{
  QString sql = "update %1 set (%2)=(%3) where id=?";
  QStringList fields;
  QVariantList values;
  QStringList placeholders;
  foreach (QString field, jObj.keys()) {
    if (field == "id")
      continue;
    fields.append("\"" + field + "\"");
    values.append(jObj.value(field).toVariant());
    placeholders.append("?");
  }
  sql = sql.arg(tableName).arg(fields.join(","))
      .arg(placeholders.join(","));
  query.prepare(sql);
  foreach (QVariant value, values) {
    query.addBindValue(value);
  }
  query.addBindValue(jObj.value("id").toVariant());
}

void QSqlQueryHelper::prepareDeleteQuery(QSqlQuery &query, QString tableName, const QVariant &idVal)
{
  QString delPattern = "delete from %1 where id=:id";
  query.prepare(delPattern.arg(tableName));
  query.bindValue(":id", idVal);
}

void QSqlQueryHelper::setLogging(bool enable)
{
  _logging = enable;
}

