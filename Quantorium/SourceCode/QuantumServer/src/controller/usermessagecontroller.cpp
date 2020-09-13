#include "usermessagecontroller.h"
#include "../core/api.h"
#include "../utils/qsqlqueryhelper.h"
#include "../core/usernotifier.h"
#include "../core/notifications.h"

extern UserNotifier* userNotifier;

UserMessageController::UserMessageController(Context &con) : BaseJsonActionController(con)
{
  registerAction(new SendMessageAction(ACT_SEND_MESSAGE, "Посылка сообщения пользователю"));
}


SendMessageAction::SendMessageAction(QString name, QString description) : AbstractJsonAction(name, description)
{

}

ActionResult SendMessageAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_MESSAGE) || !params.contains(PRM_USER_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_MESSAGE ", " PRM_USER_ID), ERR_DEFAULT_CUSTOM);
  }
  qlonglong userId = params.value(PRM_USER_ID).toLongLong();
  QSqlDatabase db = QSqlDatabase::database(con.uid());
  userNotifier->notify(db, MSG_USER_MESSAGE, userId, params, con.sysuserId());
  return ActionResult("Сообщение послано");
}
