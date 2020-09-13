#ifndef BASEJSONACTIONCONTROLLER_H
#define BASEJSONACTIONCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include "../core/appconst.h"
#include "basejsonsqlcontroller.h"

//Команда для получения списка действий контроллера
#define ACT_INFO_PATH "actionList"

const QString SErrLackOfDataInRequest = "В запросе отсутствуют необходимые данные";

struct ActionResult
{
public:
  ActionResult(QString desc = "", int code = ERR_NO_ERROR, QJsonValue jData = QJsonObject()) {
    _resultOk = (code == 0);
    _errorCode = code;
    _description = desc;
    _jData = jData;
  }
  bool resultOk(){ return _resultOk; }
  int errorCode(){ return _errorCode; }
  QString description(){ return _description; }
  QJsonValue jData(){ return _jData; }
private:
  bool _resultOk;
  int _errorCode;
  QJsonValue _jData;
  QString _description;
};

class AbstractJsonAction
{
public:
  AbstractJsonAction(QString name, QString description);
  virtual ~AbstractJsonAction();
  QString name() const;
  QString description() const;
  virtual bool isEnabled();
  void setEnabled(bool enabled);
  void setLogging(bool val);

  void setFieldFilters(QStringList fieldFilters);
  virtual bool checkUrlMatch(QString path, QString method);
  virtual bool hasAccess(const Context &con);
  ActionResult doAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue());
protected:
  //Преобразование результатов запроса в json-документ
  QJsonDocument sqlResultToJson(QSqlQuery &query);
  //Преобразование записи в json-объект
  QJsonObject sqlRecordToJson(QSqlRecord rec);

  int compareFieldValues(const QJsonObject &obj1, const QJsonObject &obj2, QString fieldName);
  virtual QString filterFromParams(const QVariantMap &params);
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue()) = 0;
  virtual ActionResult handleDatabaseError(const QSqlError &error);
  QStringList _fieldFilters;
private:  
  bool _enabled = true;
  bool _logging = true;
  QString _name;
  QString _description;
  void logAction(const Context &con, ActionResult result);
};

class BaseJsonActionController : public BaseJsonSqlController
{
  Q_OBJECT
public:
  BaseJsonActionController(Context &con);
  void registerAction(AbstractJsonAction* action);
private:
  QList<AbstractJsonAction*> _actions;
  void actionListResponse(HttpResponse &response);
  // HttpRequestHandler interface
protected:
  AbstractJsonAction* actionByName(QString actName);
  void enableAction(QString actName, bool enable);
  void unregisterAction(QString actName);
  void replaceAction(AbstractJsonAction* newAction, QString oldActionName = "");
public:
  virtual void service(HttpRequest &request, HttpResponse &response);
};

#endif // BASEJSONACTIONCONTROLLER_H
