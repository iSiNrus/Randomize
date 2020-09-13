#ifndef TEACHERACTIONCONTROLLER_H
#define TEACHERACTIONCONTROLLER_H

#include <QObject>
#include "basejsonactioncontroller.h"

class CloneCaseAction : public AbstractJsonAction
{
public:
  CloneCaseAction(QString name, QString description);

  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
private:
  bool copyUnitResources(QString unitDir, int srcUnitId, int dstUnitId);
  bool copyTaskResources(QString taskDir, int srcTaskId, int dstTaskId);
};

//Запись ученика на кейс
class AssignUserCaseAction : public AbstractJsonAction
{
public:
  AssignUserCaseAction(QString name, QString description);

  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};

class RegisterLessonAction : public AbstractJsonAction
{
public:
  RegisterLessonAction(QString name, QString description);

  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};

class UnregisterLessonAction : public AbstractJsonAction
{
public:
  UnregisterLessonAction(QString name, QString description);

  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};

//Удаления ученика с кейса
class DeleteUserCaseAction : public AbstractJsonAction
{
public:
  DeleteUserCaseAction(QString name, QString description);

  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};

class UpdateLessonScoreAction : public AbstractJsonAction
{
public:
  UpdateLessonScoreAction(QString name, QString description);
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue());
protected:
  virtual ActionResult handleDatabaseError(const QSqlError &error);
};

class UpdateTaskScoreAction : public AbstractJsonAction
{
public:
  UpdateTaskScoreAction(QString name, QString description);
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue());
protected:
  virtual ActionResult handleDatabaseError(const QSqlError &error);
};

class RegisterLessonScoreAction : public AbstractJsonAction
{
public:
  RegisterLessonScoreAction(QString name, QString description);

  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue());
  // AbstractJsonAction interface
protected:
  virtual ActionResult handleDatabaseError(const QSqlError &error);
};

class RegisterTaskScoreAction : public AbstractJsonAction
{
public:
  RegisterTaskScoreAction(QString name, QString description);
  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue());
  virtual ActionResult handleDatabaseError(const QSqlError &error);
};

class DeleteLessonScoreAction : public AbstractJsonAction
{
public:
  DeleteLessonScoreAction(QString name, QString description);
  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue());
};

class DeleteTaskScoreAction : public AbstractJsonAction
{
public:
  DeleteTaskScoreAction(QString name, QString description);
  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue());
};

class TeacherActionController : public BaseJsonActionController
{
  Q_OBJECT
  Q_DISABLE_COPY(TeacherActionController)
public:
  TeacherActionController(Context &con);
protected:
  virtual bool hasAccess();
private:

};

#endif // TEACHERACTIONCONTROLLER_H
