#include "batchsubmitresttablecontroller.h"
#include "../core/appconst.h"
#include "../utils/qsqlqueryhelper.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

BatchSubmitRestTableController::BatchSubmitRestTableController(QString tableName, Context &con)
  : BaseRestTableController(tableName, con)
{
  unregisterAction(tableName + "_insert");
  enableAction(tableName + "_update", false);
  enableAction(tableName + "_delete", false);
  registerAction(new BatchSubmitAction(_tableName, "batch_submit", "Групповое сохранение таблицы", _fieldFilters));
}

BatchSubmitAction::BatchSubmitAction(QString table, QString name, QString description, QStringList fieldFilters)
  : AbstractTableAction(table, name, description, fieldFilters)
{

}


bool BatchSubmitAction::checkUrlMatch(QString path, QString method)
{
  Q_UNUSED(path)
  return method == HTTP_POST;
}

void BatchSubmitAction::prepareInsertQuery(QSqlQuery &query, const QJsonObject &itemObj)
{
  QSqlQueryHelper::prepareInsertQuery(query, _tableName, itemObj);
}

void BatchSubmitAction::prepareUpdateQuery(QSqlQuery &query, const QJsonObject &itemObj)
{
  QSqlQueryHelper::prepareUpdateQuery(query, _tableName, itemObj);
}

void BatchSubmitAction::prepareDeleteQuery(QSqlQuery &query, const QJsonObject &itemObj)
{
  QSqlQueryHelper::prepareDeleteQuery(query, _tableName, itemObj.value("id").toVariant());
}

ActionResult BatchSubmitAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(params)
  QSqlDatabase db = QSqlDatabase::database(con.uid());
  bool tsAction = db.transaction();
  qDebug() << "Transaction started" << tsAction;
  QSqlError sqlError;
  QSqlQuery query(db);

  QJsonArray jArray = bodyData.toArray();
  foreach (QJsonValue jVal, jArray) {
    qDebug() << jVal;
    QJsonObject jObj = jVal.toObject();
    QString action = jObj.value("action").toString();
    jObj.remove("action");
    if (action == "insert") {
      prepareInsertQuery(query, jObj);
    }
    else if (action == "update") {
      prepareUpdateQuery(query, jObj);
    }
    else if (action == "delete") {
      prepareDeleteQuery(query, jObj);
    }
    else {
      continue;
    }
    if (!execSql(query)) {
      sqlError = query.lastError();
      break;
    }
  }

  if (sqlError.type() == QSqlError::NoError) {
    tsAction = db.commit();
    qDebug() << "Transaction commited" << tsAction;
    return ActionResult("Success");
  }
  else {
    db.rollback();
    qDebug() << "Transaction rolledback" << tsAction;
    return handleDatabaseError(sqlError);
  }
}
