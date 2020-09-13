#ifndef COURSETABLECONTROLLER_H
#define COURSETABLECONTROLLER_H

#include <QObject>
#include "baseresttablecontroller.h"

class CourseTableController : public BaseRestTableController
{
  Q_OBJECT
  Q_DISABLE_COPY(CourseTableController)
public:
  CourseTableController(QString tableName, Context& con);

  // AbstractSqlController interface
protected:
  virtual bool hasAccess() override;

  // BaseRestTableController interface
protected:
  virtual QString filterFromParams(const QSqlRecord &infoRec) override;
};

#endif // COURSETABLECONTROLLER_H
