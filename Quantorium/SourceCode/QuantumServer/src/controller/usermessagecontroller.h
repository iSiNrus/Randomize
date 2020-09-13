#ifndef USERMESSAGECONTROLLER_H
#define USERMESSAGECONTROLLER_H

#include <QObject>
#include "basejsonactioncontroller.h"

class SendMessageAction : public AbstractJsonAction
{
public:
  SendMessageAction(QString name, QString description);

  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};

class UserMessageController : public BaseJsonActionController
{
  Q_OBJECT
  Q_DISABLE_COPY(UserMessageController)
public:
  UserMessageController(Context &con);
};

#endif // USERMESSAGECONTROLLER_H
