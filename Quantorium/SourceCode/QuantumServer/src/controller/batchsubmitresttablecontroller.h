#ifndef BATCHSUBMITRESTTABLECONTROLLER_H
#define BATCHSUBMITRESTTABLECONTROLLER_H

#include <QObject>
#include "baseresttablecontroller.h"

class BatchSubmitAction : public AbstractTableAction
{
public:
  BatchSubmitAction(QString table, QString name, QString description, QStringList fieldFilters = QStringList());

  // AbstractJsonAction interface
public:
  virtual bool checkUrlMatch(QString path, QString method) override;

protected:
  void prepareInsertQuery(QSqlQuery &query, const QJsonObject &itemObj);
  void prepareUpdateQuery(QSqlQuery &query, const QJsonObject &itemObj);
  void prepareDeleteQuery(QSqlQuery &query, const QJsonObject &itemObj);
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};

class BatchSubmitRestTableController : public BaseRestTableController
{
  Q_OBJECT
  Q_DISABLE_COPY(BatchSubmitRestTableController)
public:
  BatchSubmitRestTableController(QString tableName, Context& con);
};

#endif // BATCHSUBMITRESTTABLECONTROLLER_H
