#ifndef ACTIONLOGTABLECONTROLLER_H
#define ACTIONLOGTABLECONTROLLER_H

#include <QObject>
#include "baseresttablecontroller.h"

class ActionLogTableController : public BaseRestTableController
{
  Q_OBJECT
  Q_DISABLE_COPY(ActionLogTableController)
public:
  ActionLogTableController(QString tableName, Context& con);

  // AbstractSqlController interface
protected:
  virtual bool hasAccess() override;
};

#endif // ACTIONLOGTABLECONTROLLER_H
