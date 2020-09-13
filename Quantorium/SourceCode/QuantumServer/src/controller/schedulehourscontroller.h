#ifndef SCHEDULEHOURSCONTROLLER_H
#define SCHEDULEHOURSCONTROLLER_H

#include <QObject>
#include "baseresttablecontroller.h"

class BatchChangesSubmit : public AbstractTableAction
{
  // AbstractJsonAction interface
public:
  BatchChangesSubmit(QString table, QString name, QString description, QStringList fieldFilters = QStringList());
  virtual bool checkUrlMatch(QString path, QString method) override;

protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};

class ScheduleHoursController : public BaseRestTableController
{
  Q_OBJECT
public:
  ScheduleHoursController(QString tableName, Context& con);
};

#endif // SCHEDULEHOURSCONTROLLER_H
