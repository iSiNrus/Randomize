#ifndef BASERESTTABLECONTROLLER_H
#define BASERESTTABLECONTROLLER_H

#include "httprequest.h"
#include "httpresponse.h"
#include "httprequesthandler.h"
#include <QJsonDocument>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include "../httpsessionmanager.h"
#include "basejsonsqlcontroller.h"
#include "basejsonactioncontroller.h"

const QString SErrMethodIsNotSupported = "Метод %1 не поддерживается для данного URL";

const QString SDeleteResult = "Из таблицы %1 удалена запись c идентификатором %2";
const QString SInsertResult = "В таблицу %1 добавлена новая запись";
const QString SUpdateResult = "В таблице %1 отредактирована запись с идентификатором %2";

QString encaseValue(QString strVal, QVariant::Type type);

/*!
    Класс контроллера с базовым функционалом редактирования таблицы
 */
class AbstractTableAction : public AbstractJsonAction
{
public:
  AbstractTableAction(QString table, QString name, QString description, QStringList fieldFilters = QStringList());
protected:
  QString _tableName;
  QString sqlTableName();
  QJsonObject recordObjById(const Context &con, int id);
};

class InsertAction : public AbstractTableAction
{
public:
  InsertAction(QString table, QString name, QString description, QStringList fieldFilters = QStringList());
  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue());
  // AbstractJsonAction interface
public:
  virtual bool checkUrlMatch(QString path, QString method);
};

class UpdateAction : public AbstractTableAction
{
public:
  UpdateAction(QString table, QString name, QString description, QStringList fieldFilters = QStringList());
  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue());
  // AbstractJsonAction interface
public:
  virtual bool checkUrlMatch(QString path, QString method);
};

class DeleteAction : public AbstractTableAction
{
public:
  DeleteAction(QString table, QString name, QString description, QStringList fieldFilters = QStringList());
  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue());
  // AbstractJsonAction interface
public:
  virtual bool checkUrlMatch(QString path, QString method);
};

class BaseRestTableController : public BaseJsonActionController
{
  Q_OBJECT
  Q_DISABLE_COPY(BaseRestTableController)
public:
  BaseRestTableController(QString tableName, Context& con);
  void setIdField(QString name);
  void setAttachedField(QString name, QString query);
  // AbstractSqlController interface
protected:
  virtual bool hasAccess();
protected:
  QString _tableName;
  QString _idField = "id";
  QString sqlTableName();
  virtual QString filterFromParams(const QSqlRecord &infoRec);
  QString filterExpression(QString colName, QString strExpr, QVariant::Type type);
//  QString encaseValue(QString strVal, QVariant::Type type);
private:
  QMap<QString, QString> _attachedFields;
  void listAction(HttpResponse& response, Context &con);
  QString fieldsString();

  // HttpRequestHandler interface
public:
  virtual void service(HttpRequest &request, HttpResponse &response);
};

#endif // BASERESTTABLECONTROLLER_H
