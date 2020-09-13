#ifndef BASEJSONSQLCONTROLLER_H
#define BASEJSONSQLCONTROLLER_H

#include <QObject>
#include <QSqlRecord>
#include <QJsonObject>
#include <QJsonDocument>
#include "abstractsqlcontroller.h"

const QString SErrUrlHasNoHandler = "Нет обработчиков по адресу %1";
const QString SErrNotEnoughParams = "Не хватает по крайней мере одного из обязательных параметров: %1";

class BaseJsonSqlController : public AbstractSqlController
{
  Q_OBJECT
  Q_DISABLE_COPY(BaseJsonSqlController)
public:
  BaseJsonSqlController(Context &con);

  static void setLogSql(bool enable);
  static bool logSql();
  static QString dbErrorDescription(QString dbMsg);
protected:
  static bool _logSql;
  static QMap<QString, QString> _dbErrors;
  static QMap<QString, QString> initDbErrors();
  //Преобразование результатов запроса в json-документ
  QJsonDocument sqlResultToJson(QSqlQuery &query);
  //Преобразование записи в json-объект
  QJsonObject sqlRecordToJson(QSqlRecord rec);

  virtual void handleDatabaseError(HttpResponse &response, const QSqlError &error);

  virtual void writeError(HttpResponse &response, QString errorMsg, int code = 100);
  virtual void writeResultOk(HttpResponse &response, QString msg, QJsonValue dataObj = QJsonObject());

  virtual void readParams(HttpRequest &request);

  qlonglong paramLong(QString name);
  QString paramString(QString name);
  QDate paramDate(QString name);
  bool hasParam(QString name);
  QVariantMap params() const;
  QString method();
private:
  QVariantMap _paramsMap;
  QString _method;
  // HttpRequestHandler interface
public:
  virtual void service(HttpRequest &request, HttpResponse &response);

  // AbstractSqlController interface
protected:
  virtual bool hasAccess();
};

bool execSql(QSqlQuery &query, bool logParams = true);

#endif // BASEJSONSQLCONTROLLER_H
