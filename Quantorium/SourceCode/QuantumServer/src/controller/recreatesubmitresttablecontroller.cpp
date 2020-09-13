#include "recreatesubmitresttablecontroller.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QSqlQuery>
#include "../core/appconst.h"
#include "../utils/qsqlqueryhelper.h"

#define justonce for(int dummyIdx=0; dummyIdx==0; dummyIdx++)

RecreateSubmitRestTableController::RecreateSubmitRestTableController(QString tableName, Context &con) : BaseRestTableController(tableName, con)
{
  qInfo() << "Batch submit controller";
  //TODO: Возможно лучше не отключать стандартные действия
  unregisterAction(tableName + "_insert");
  enableAction(tableName + "_update", false);
  enableAction(tableName + "_delete", false);
  registerAction(new RecreateSubmitAction(_tableName, "batch_submit", "Групповое сохранение таблицы", _fieldFilters));
}

RecreateSubmitAction::RecreateSubmitAction(QString table, QString name, QString description, QStringList fieldFilters)
  : AbstractTableAction(table, name, description, fieldFilters)
{
}

ActionResult RecreateSubmitAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  QSqlDatabase db = QSqlDatabase::database(con.uid());
  bool tsAction = db.transaction();
  qDebug() << "Transaction started" << tsAction;
  QSqlError sqlError;

  QString sql = "delete from %1 where %2";
  QSqlQuery query(db);
  query.prepare(sql.arg(_tableName).arg(filterFromParams(params)));
  if (!execSql(query)){
    sqlError = query.lastError();
  }
  else {
    QJsonArray jArray = bodyData.toArray();
    foreach (QJsonValue jVal, jArray) {
      QSqlQueryHelper::prepareInsertQuery(query, _tableName, jVal.toObject());
      if (!execSql(query)) {
        sqlError = query.lastError();
        break;
      }
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

bool RecreateSubmitAction::checkUrlMatch(QString path, QString method)
{
  Q_UNUSED(path)
  return method == HTTP_POST;
}
