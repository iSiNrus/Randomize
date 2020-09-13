#ifndef ADMINSCHEDULECONTROLLER_H
#define ADMINSCHEDULECONTROLLER_H

#include <QObject>
#include "basejsonactioncontroller.h"

class GetScheduleAction : public AbstractJsonAction
{
public:
  GetScheduleAction(QString name, QString description);

  // AbstractJsonAction interface
public:
  virtual bool checkUrlMatch(QString path, QString method) override;

protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};

class PostScheduleAction : public AbstractJsonAction
{
public:
  PostScheduleAction(QString name, QString description);

  // AbstractJsonAction interface
public:
  virtual bool checkUrlMatch(QString path, QString method) override;
  virtual bool hasAccess(const Context &con) override;

protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};

class AdminScheduleController : public BaseJsonActionController
{
  Q_OBJECT
  Q_DISABLE_COPY(AdminScheduleController)
public:
  AdminScheduleController(Context &con);
};

#endif // ADMINSCHEDULECONTROLLER_H
