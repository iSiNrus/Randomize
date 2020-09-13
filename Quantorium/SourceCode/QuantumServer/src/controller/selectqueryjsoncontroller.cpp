#include "selectqueryjsoncontroller.h"
#include <QSqlError>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegExp>


SelectQueryJsonController::SelectQueryJsonController(QString sql, Context &con) :
  BaseJsonSqlController(con)
{
  _sql = sql.split(QRegExp("[\\{\\}]"));
}

void SelectQueryJsonController::service(HttpRequest &request, HttpResponse &response)
{  
  BaseJsonSqlController::service(request, response);
  if (response.getStatusCode() != 200)
    return;
  //TODO: Проверку на метод (либо GET либо POST)
  QSqlQuery query(_context.db());
  //Поддержка параметров запроса
  bool notEmpty = prepareQuery(query, request);
  if (!notEmpty) {
    writeError(response, SErrMissingParams.arg(_missingParams.join(", ")));
    return;
  }
  qWarning() << query.lastQuery();
  if (!execSql(query)){
    writeError(response, query.lastError().databaseText());
    return;
  }
  response.setHeader("Content-Type", "application/json; charset=UTF-8");
  QJsonArray jArray;
  while (query.next()) {
    QJsonObject jObject = sqlRecordToJson(query.record());
    modifyResultRecord(jObject);
    jArray.append(QJsonValue(jObject));
  }
  response.write(QJsonDocument(jArray).toJson(), true);
}

bool SelectQueryJsonController::hasAccess()
{
  return true;
}

void SelectQueryJsonController::modifyResultRecord(QJsonObject &jObj)
{
  //Можно переопределить в наследниках для модификации данных и добавления
  //новых полей
}

QVariant SelectQueryJsonController::toVariant(QString str)
{
  bool resOk;
  str.toLongLong(&resOk);
  if (resOk) {
    return str.toLongLong();
  }
  str.toFloat(&resOk);
  if (resOk) {
    return str.toFloat();
  }
  return str;
}

bool SelectQueryJsonController::prepareQuery(QSqlQuery &query, HttpRequest &request)
{
  QString resSql;
  QRegExp rx(":([A-Za-z_]+)");
  foreach(QString sqlPart, _sql) {
    int paramIdx = 0;
    bool include = true;
    while (rx.indexIn(sqlPart, paramIdx) >= 0) {
      QString paramName = rx.cap(1);
      if (!request.parameters.contains(paramName.toUtf8())) {
        _missingParams.append(paramName);
        include = false;
        break;
      }
      paramIdx += rx.pos() + rx.matchedLength();
    }
    if (include)
      resSql.append(sqlPart);
  }
  if (resSql.isEmpty())
    return false;
  qDebug() << resSql;
  query.prepare(resSql);

  //Связывание параметров
  int paramIdx = 0;
  while (rx.indexIn(resSql, paramIdx) >= 0) {
    QString paramName = rx.cap(1);
    QString paramVal = QString::fromLocal8Bit(request.parameters.value(paramName.toLocal8Bit()));
    qDebug() << "param:" << paramName << "=" << paramVal;
    query.bindValue(":" + paramName, toVariant(paramVal));
    paramIdx = rx.pos() + rx.matchedLength();
  }
  return true;
}
