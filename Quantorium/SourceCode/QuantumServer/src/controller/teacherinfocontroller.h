#ifndef TEACHERINFOCONTROLLER_H
#define TEACHERINFOCONTROLLER_H

#include <QObject>
#include "basejsonactioncontroller.h"
#include "basejsonsqlcontroller.h"

class DayOffByCityAction : public AbstractJsonAction
{
public:
  DayOffByCityAction(QString name, QString description);
  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};

class TeacherInfoController : public BaseJsonActionController
{
  Q_OBJECT
  Q_DISABLE_COPY(TeacherInfoController)
public:
  TeacherInfoController(Context &con);

  // HttpRequestHandler interface
public:

  // AbstractSqlController interface
protected:
  virtual bool hasAccess();
private:

};

#endif // TEACHERINFOCONTROLLER_H
