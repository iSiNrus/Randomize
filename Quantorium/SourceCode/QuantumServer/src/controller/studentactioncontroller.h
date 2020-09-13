#ifndef STUDENTACTIONCONTROLLER_H
#define STUDENTACTIONCONTROLLER_H

#include <QObject>
#include "basejsonactioncontroller.h"

class EnterCourseAction : public AbstractJsonAction
{
public:
  EnterCourseAction(QString name, QString description);

  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue());
  virtual ActionResult handleDatabaseError(const QSqlError &error);
};

class EnrolOnLessonCourse : public AbstractJsonAction
{
public:
  EnrolOnLessonCourse(QString name, QString description);
  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue());
  virtual ActionResult handleDatabaseError(const QSqlError &error);
};

class StudentActionController : public BaseJsonActionController
{
  Q_OBJECT
  Q_DISABLE_COPY(StudentActionController)
public:
  StudentActionController(Context &con);
  // AbstractSqlController interface
protected:
  virtual bool hasAccess();
private:

};

#endif // STUDENTACTIONCONTROLLER_H
