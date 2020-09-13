#ifndef NOTIFICATIONCONTROLLER_H
#define NOTIFICATIONCONTROLLER_H

#include <QObject>
#include "basejsonactioncontroller.h"

class GetNotificationsAction : public AbstractJsonAction
{
public:
  GetNotificationsAction(QString name, QString description);

  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};

class CheckViewedAction : public AbstractJsonAction
{
public:
  CheckViewedAction(QString name, QString description);

  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};

class NotificationController : public BaseJsonActionController
{
  Q_OBJECT
  Q_DISABLE_COPY(NotificationController)
public:
  NotificationController(Context &con);
};

#endif // NOTIFICATIONCONTROLLER_H
