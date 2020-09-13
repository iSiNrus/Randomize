#include "notificationcontroller.h"
#include "../core/appconst.h"
#include "../core/api.h"
#include "../utils/qsqlqueryhelper.h"
#include <QJsonArray>
#include <QJsonDocument>


NotificationController::NotificationController(Context &con) : BaseJsonActionController(con)
{
  registerAction(new GetNotificationsAction(ACT_GET_NOTIFICATIONS, "Получение оповещений"));
  registerAction(new CheckViewedAction(ACT_CHECK_VIEWED, "Отметка о прочитанных оповещениях"));
}

GetNotificationsAction::GetNotificationsAction(QString name, QString description) : AbstractJsonAction(name, description)
{
}

CheckViewedAction::CheckViewedAction(QString name, QString description) : AbstractJsonAction(name, description)
{
}


ActionResult GetNotificationsAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)
  if (!params.contains(PRM_USER_ID) || !params.contains(PRM_LAST_ID)) {
    return ActionResult(SErrNotEnoughParams, ERR_DEFAULT_CUSTOM);
  }
  QString userId = params.value(PRM_USER_ID).toString();
  QString lastId = params.value(PRM_LAST_ID).toString();
  QString sql = "select * from notice where viewed=false and receiver_id=%1 and id>%2";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(userId).arg(lastId), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  return ActionResult("Новые оповещения", ERR_NO_ERROR, sqlResultToJson(query).array());
}

ActionResult CheckViewedAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)
  if (!params.contains(PRM_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_ID), ERR_DEFAULT_CUSTOM);
  }
  QString sql = "update notice set viewed=true where id=%1";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_ID).toString()), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  return ActionResult("Оповещение отмечено как просмотренное");
}
