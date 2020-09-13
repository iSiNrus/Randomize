#include "basejsonactioncontroller.h"
#include "../utils/qsqlqueryhelper.h"
#include "../core/appsettings.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>

BaseJsonActionController::BaseJsonActionController(Context &con) : BaseJsonSqlController(con)
{
}

void BaseJsonActionController::registerAction(AbstractJsonAction *action)
{
  return _actions.append(action);
}

void BaseJsonActionController::actionListResponse(HttpResponse &response)
{
  QJsonArray jActions;
  foreach (AbstractJsonAction* action, _actions) {
    QJsonObject jObj;
    jObj.insert(PRM_NAME, action->name());
    jObj.insert(PRM_DESCRIPTION, action->description());
    jActions.append(jObj);
  }
  response.write(QJsonDocument(jActions).toJson());
}

AbstractJsonAction *BaseJsonActionController::actionByName(QString actName)
{
  AbstractJsonAction* act = 0;
  foreach (AbstractJsonAction* item, _actions) {
    if (item->name() == actName) {
      act = item;
      break;
    }
  }
  Q_ASSERT(act != 0);
  return act;
}

void BaseJsonActionController::enableAction(QString actName, bool enable)
{
  actionByName(actName)->setEnabled(enable);
}

void BaseJsonActionController::unregisterAction(QString actName)
{
  AbstractJsonAction* act = actionByName(actName);
  _actions.removeOne(act);
  delete act;
}

void BaseJsonActionController::replaceAction(AbstractJsonAction* newAction, QString oldActionName)
{
  if (oldActionName.isEmpty())
    oldActionName = newAction->name();

  unregisterAction(oldActionName);
  registerAction(newAction);
}

void BaseJsonActionController::service(HttpRequest &request, HttpResponse &response)
{
  BaseJsonSqlController::service(request, response);
  if (response.getStatusCode() != 200)
    return;

  QString path = QString::fromLocal8Bit(request.getPath());
  if (path.endsWith(ACT_INFO_PATH)) {
    actionListResponse(response);
    return;
  }
  foreach (AbstractJsonAction* actionItem, _actions) {
    QString method = QString::fromUtf8(request.getMethod());
    if (!actionItem->checkUrlMatch(path, method))
      continue;
    if (!actionItem->hasAccess(_context)) {
      //Forbidden
      writeError(response, SErrForbidden, HTTP_FORBIDDEN);
      return;
    }
    QJsonDocument jDoc = QJsonDocument::fromJson(request.getBody());
    ActionResult actionResult = actionItem->doAction(_context, params(), jDoc.isObject() ? QJsonValue(jDoc.object()) : QJsonValue(jDoc.array()));
    if (actionResult.resultOk()) {
      writeResultOk(response, actionResult.description(), actionResult.jData());
    }
    else {
      writeError(response, actionResult.description(), actionResult.errorCode());
    }
    return;
  }
  writeError(response, SErrUrlHasNoHandler.arg(path), HTTP_NOT_FOUND);
}

AbstractJsonAction::AbstractJsonAction(QString name, QString description)
{
  _name = name;
  _description = description;
}

AbstractJsonAction::~AbstractJsonAction()
{
}

QString AbstractJsonAction::name() const
{
  return _name;
}

QString AbstractJsonAction::description() const
{
  return _description;
}

bool AbstractJsonAction::isEnabled()
{
  return _enabled;
}

void AbstractJsonAction::setEnabled(bool enabled)
{
  _enabled = enabled;
}

void AbstractJsonAction::setLogging(bool val)
{
  _logging = val;
}

void AbstractJsonAction::setFieldFilters(QStringList fieldFilters)
{
  _fieldFilters = fieldFilters;
}

bool AbstractJsonAction::checkUrlMatch(QString path, QString method)
{
  Q_UNUSED(method)
  return path.endsWith("/" + name());
}

bool AbstractJsonAction::hasAccess(const Context &con)
{
  Q_UNUSED(con)
  return true;
}

ActionResult AbstractJsonAction::doAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  if (!isEnabled()) {
    return ActionResult("Действие " + name() + " недоступно", ERR_DEFAULT_CUSTOM);
  }
  ActionResult actionResult = doInternalAction(con, params, bodyData);
  #ifndef NO_AUTHORIZATION
  if (_logging)
    logAction(con, actionResult);
  #endif
  return actionResult;
}

QJsonDocument AbstractJsonAction::sqlResultToJson(QSqlQuery &query)
{
  QJsonArray jArray;
  while (query.next()) {
    QJsonObject jObject = sqlRecordToJson(query.record());
    jArray.append(QJsonValue(jObject));
  }
  return QJsonDocument(jArray);
}

QJsonObject AbstractJsonAction::sqlRecordToJson(QSqlRecord rec)
{
  QJsonObject jObject;
  for(int i=0; i<rec.count(); i++) {
    //Отфильтровываем ненужные поля
    if (_fieldFilters.contains(rec.fieldName(i), Qt::CaseInsensitive))
      continue;
    QVariant fieldVal = QVariant();
    if (!rec.isNull(i))
      fieldVal = rec.value(i);
    jObject.insert(rec.fieldName(i), QJsonValue::fromVariant(fieldVal));
  }
  return jObject;
}

int AbstractJsonAction::compareFieldValues(const QJsonObject &obj1, const QJsonObject &obj2, QString fieldName)
{
  if (obj1.contains(fieldName) != obj2.contains(fieldName))
    return obj1.contains(fieldName) ? 1 : -1;
  else if (!obj1.contains(fieldName))
    return 0;

  return QString::compare(obj1.value(fieldName).toVariant().toString(),
                          obj2.value(fieldName).toVariant().toString());
}

QString AbstractJsonAction::filterFromParams(const QVariantMap &params)
{
  QString filter = "1=1";
  foreach (QString key, params.keys()) {
    if (key.endsWith("_id")) {
      filter.append(" and ").append(key).append("=").append(params.value(key).toString());
    }
  }
  return filter;
}

ActionResult AbstractJsonAction::handleDatabaseError(const QSqlError &error)
{
  return ActionResult(BaseJsonSqlController::dbErrorDescription(error.databaseText()), ERR_DATABASE);
}

void AbstractJsonAction::logAction(const Context &con, ActionResult result)
{
  //Вывод информации в консоль
  QString consoleMsg = "Action %1: ResultOk=%2, Description: (%3) %4";
  qInfo() << consoleMsg.arg(name()).arg(result.resultOk() ? "true" : "false").arg(QString::number(result.errorCode())).arg(result.description());
  QString sql = "insert into action_log(request_uid, user_id, action, success, description) values ('%1', %2, '%3', %4, '%5')";
  QString preparedSql = sql.arg(con.uid(), QString::number(con.sysuserId()), name(), result.resultOk() ? "TRUE" : "FALSE", result.description());
  QSqlQuery query = QSqlQueryHelper::execSql(preparedSql, con.uid());
  if (query.lastError().isValid())
    qCritical() << "Error while trying to log action:" << query.lastError().databaseText();
}
