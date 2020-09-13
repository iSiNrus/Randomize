#ifndef CITYTABLECONTROLLER_H
#define CITYTABLECONTROLLER_H

#include <QObject>
#include "baseresttablecontroller.h"

class CityTableController : public BaseRestTableController
{
  Q_OBJECT
  Q_DISABLE_COPY(CityTableController)
public:
  CityTableController(QString tableName, Context& con);
  // BaseRestTableController interface
protected:
  virtual QString filterFromParams(const QSqlRecord &infoRec);

  // AbstractSqlController interface
protected:
  virtual bool hasAccess();
};
#endif // CITYTABLECONTROLLER_H
