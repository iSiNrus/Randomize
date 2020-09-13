#ifndef ADMINACTIONCONTROLLER_H
#define ADMINACTIONCONTROLLER_H

#include <QObject>
#include "basejsonactioncontroller.h"


class RemoveRequiredCaseAchievementAction : public AbstractJsonAction
{
public:
  RemoveRequiredCaseAchievementAction(QString name, QString desctiption);

  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};

class ActivateCaseAction : public AbstractJsonAction
{
public:
  ActivateCaseAction(QString name, QString description);

  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};

class DeactivateCaseAction : public AbstractJsonAction
{
public:
  DeactivateCaseAction(QString name, QString description);

  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};

class RegisterUserForLearningGroup : public AbstractJsonAction
{
public:
  RegisterUserForLearningGroup(QString name, QString description);

  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;

  // AbstractJsonAction interface
protected:
  virtual ActionResult handleDatabaseError(const QSqlError &error) override;
};

class UnregisterUserFromLearningGroup : public AbstractJsonAction
{
public:
  UnregisterUserFromLearningGroup(QString name, QString description);

  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};

class AdminActionController : public BaseJsonActionController
{
  Q_OBJECT
  Q_DISABLE_COPY(AdminActionController)
public:
  AdminActionController(Context &con);
  // AbstractSqlController interface
protected:
  virtual bool hasAccess();
};

#endif // ADMINACTIONCONTROLLER_H
