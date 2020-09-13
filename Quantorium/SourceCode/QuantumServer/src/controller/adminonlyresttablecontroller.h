#ifndef ADMINONLYRESTTABLECONTROLLER_H
#define ADMINONLYRESTTABLECONTROLLER_H

#include <QObject>
#include "../httpsessionmanager.h"
#include "baseresttablecontroller.h"

class AdminOnlyRestTableController : public BaseRestTableController
{
  Q_OBJECT
  Q_DISABLE_COPY(AdminOnlyRestTableController)
public:
  AdminOnlyRestTableController(QString tableName, Context& con);
  virtual bool hasAccess();
};

#endif // ADMINONLYRESTTABLECONTROLLER_H
