#ifndef CITYRESTRICTEDTABLECONTROLLER_H
#define CITYRESTRICTEDTABLECONTROLLER_H

#include <QObject>
#include "baseresttablecontroller.h"

class CityRestrictedTableController : public BaseRestTableController
{
  Q_OBJECT
  Q_DISABLE_COPY(CityRestrictedTableController)
public:
  CityRestrictedTableController(QString tableName, Context& con);

  // BaseRestTableController interface
protected:
  virtual QString filterFromParams(const QSqlRecord &infoRec);

  // AbstractSqlController interface
protected:
  virtual bool hasAccess();
};

#endif // CITYRESTRICTEDTABLECONTROLLER_H
