#include "basejsonsqlcontroller.h"
#include <QJsonArray>
#include <QJsonValue>
#include <QSqlError>
#include <QDate>
#include "../core/appconst.h"

bool BaseJsonSqlController::_logSql = false;

QMap<QString, QString> BaseJsonSqlController::_dbErrors = BaseJsonSqlController::initDbErrors();

BaseJsonSqlController::BaseJsonSqlController(Context &con) : AbstractSqlController(con)
{

}

void BaseJsonSqlController::setLogSql(bool enable)
{
  _logSql = enable;
}

bool BaseJsonSqlController::logSql()
{
  return _logSql;
}

QMap<QString, QString> BaseJsonSqlController::initDbErrors()
{
  QMap<QString, QString> errors;
  errors.insert("uq_child_achievement", "Достижение уже в списке требуемых");
  errors.insert("ErrAchievementLoops", "Ошибка зависимостей достижений. Добавление достижения невозможно.");
  errors.insert("uq_case_achievement", "Достижение уже в списке получаемых");
  errors.insert("uq_case_required_achievement", "Достижение уже в списке требуемых");
  errors.insert("ErrAlreadyInCaseAchievements", "Данное достижение уже в списке получаемых. Добавление достижения невозможно.");
  errors.insert("ErrAlreadyInCaseRequiredAchievements", "Данное достижение уже в списке требуемых. Добавление достижения невозможно.");
  return errors;
}

QString BaseJsonSqlController::dbErrorDescription(QString dbMsg)
{
  QString msg = dbMsg;
  foreach (QString pattern, _dbErrors.keys()) {
    if (msg.contains(pattern)) {
      msg = _dbErrors.value(pattern);
      break;
    }
  }
  return msg;
}

QJsonDocument BaseJsonSqlController::sqlResultToJson(QSqlQuery &query)
{
  QJsonArray jArray;
  while (query.next()) {
    QJsonObject jObject = sqlRecordToJson(query.record());
    jArray.append(QJsonValue(jObject));
  }
  return QJsonDocument(jArray);
}

QJsonObject BaseJsonSqlController::sqlRecordToJson(QSqlRecord rec)
{
  QJsonObject jObject;
  for(int i=0; i<rec.count(); i++) {
    //Отфильтровываем ненужные поля
    if (_fieldFilters.contains(rec.fieldName(i), Qt::CaseInsensitive))
      continue;
    jObject.insert(rec.fieldName(i), rec.isNull(i) ? QJsonValue() : QJsonValue::fromVariant(rec.value(i)));
  }
  return jObject;
}

void BaseJsonSqlController::handleDatabaseError(HttpResponse &response, const QSqlError &error)
{
  writeError(response, dbErrorDescription(error.databaseText()), error.type());
}

void BaseJsonSqlController::writeError(HttpResponse &response, QString errorMsg, int code)
{
  switch (code) {
  case ERR_DEFAULT_CUSTOM:
  case ERR_DATABASE:
    response.setStatus(HTTP_REQUEST_ERROR);
    break;
  case HTTP_FORBIDDEN:
    response.setStatus(HTTP_FORBIDDEN);
    break;
  case HTTP_NOT_FOUND:
    response.setStatus(HTTP_NOT_FOUND);
    break;
  default:
    response.setStatus(HTTP_REQUEST_ERROR);
  }
  response.setHeader("Content-Type", "application/json; charset=UTF-8");
  QJsonObject responseObj;
  responseObj.insert("result", QJsonValue::fromVariant("error"));
  responseObj.insert("code", QJsonValue::fromVariant(code));
  responseObj.insert("description", QJsonValue::fromVariant(errorMsg));
  response.write(QJsonDocument(responseObj).toJson(), true);
}

void BaseJsonSqlController::writeResultOk(HttpResponse &response, QString msg, QJsonValue dataObj)
{
  QJsonObject responseObj;
  responseObj.insert("result", QJsonValue::fromVariant("ok"));
  responseObj.insert("code", QJsonValue::fromVariant(0));
  responseObj.insert("description", QJsonValue::fromVariant(msg));
  responseObj.insert("data", dataObj);
  response.write(QJsonDocument(responseObj).toJson());
}


qlonglong BaseJsonSqlController::paramLong(QString name)
{
  return _paramsMap.value(name).toLongLong();
}

QString BaseJsonSqlController::paramString(QString name)
{
  return _paramsMap.value(name).toString();
}

QDate BaseJsonSqlController::paramDate(QString name)
{
  return _paramsMap.value(name).toDate();
}

bool BaseJsonSqlController::hasParam(QString name)
{
  return _paramsMap.contains(name);
}

QVariantMap BaseJsonSqlController::params() const
{
  return _paramsMap;
}

QString BaseJsonSqlController::method()
{
  return _method;
}

void BaseJsonSqlController::readParams(HttpRequest &request)
{
  foreach (QByteArray baName, request.parameters.keys()) {
    QString name = QString::fromLocal8Bit(baName);
    QVariant value = QVariant(QString::fromUtf8(request.getParameter(baName)));
    _paramsMap.insert(name, value);
  }
  _paramsMap.insert(PRM_PATH, QString::fromLocal8Bit(request.getPath()));
  _paramsMap.insert(PRM_HTTP_METHOD, QString::fromLocal8Bit(request.getMethod()));
}

void BaseJsonSqlController::service(HttpRequest &request, HttpResponse &response)
{
  response.setHeader("Content-Type", "application/json; charset=UTF-8");
  _method = QString::fromLocal8Bit(request.getMethod());
  readParams(request);
  AbstractSqlController::service(request, response);
  if (response.getStatusCode() != HTTP_NO_ERROR)
    return;
}

bool BaseJsonSqlController::hasAccess()
{
  return true;
}

bool execSql(QSqlQuery &query, bool logParams)
{
  if (BaseJsonSqlController::logSql()) {
    qInfo() << QString("SQL: %1").arg(query.lastQuery());
    if (!query.boundValues().isEmpty() && logParams) {

      qInfo() << "Params:" << query.boundValues();
    }
  }
  bool result = query.exec();
  if (!result) {
    qWarning() << "SQL_ERROR:" << query.lastError().databaseText();
  }
  else if (BaseJsonSqlController::logSql()) {
    qInfo() << "SQL_RESULT: Ok";
  }
  return result;
}
