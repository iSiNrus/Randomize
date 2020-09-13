#ifndef CASETABLECONTROLLER_H
#define CASETABLECONTROLLER_H

#include <QObject>
#include "baseresttablecontroller.h"

class CaseTableController : public BaseRestTableController
{
  Q_OBJECT
  Q_DISABLE_COPY(CaseTableController)
public:
  CaseTableController(QString tableName, Context& con);

  // AbstractSqlController interface
protected:
  virtual bool hasAccess() override;

  // BaseRestTableController interface
protected:
  virtual QString filterFromParams(const QSqlRecord &infoRec) override;
};

#endif // CASETABLECONTROLLER_H
