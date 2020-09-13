#ifndef RECREATESUBMITRESTTABLECONTROLLER_H
#define RECREATESUBMITRESTTABLECONTROLLER_H

#include <QObject>
#include "baseresttablecontroller.h"

class RecreateSubmitAction : public AbstractTableAction
{
public:
  RecreateSubmitAction(QString table, QString name, QString description, QStringList fieldFilters = QStringList());
  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue());

  // AbstractJsonAction interface
public:
  virtual bool checkUrlMatch(QString path, QString method);
};

class RecreateSubmitRestTableController : public BaseRestTableController
{
  Q_OBJECT
  Q_DISABLE_COPY(RecreateSubmitRestTableController)
public:
  RecreateSubmitRestTableController(QString tableName, Context& con);
};

#endif // RECREATESUBMITRESTTABLECONTROLLER_H
