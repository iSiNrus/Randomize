#include "adminschedulecontroller.h"
#include "../core/appconst.h"
#include "../utils/qsqlqueryhelper.h"
#include <QJsonArray>
#include <QDate>


AdminScheduleController::AdminScheduleController(Context &con) : BaseJsonActionController(con)
{
  registerAction(new GetScheduleAction("getSchedule", "Получение данных по дням текущего года"));
  registerAction(new PostScheduleAction("postSchedule", "Сохранение изменений"));
}

GetScheduleAction::GetScheduleAction(QString name, QString description)
  : AbstractJsonAction(name, description)
{
  setLogging(false);
}

PostScheduleAction::PostScheduleAction(QString name, QString description)
  : AbstractJsonAction(name, description)
{
}

bool GetScheduleAction::checkUrlMatch(QString path, QString method)
{
  Q_UNUSED(path)
  return method == HTTP_GET;
}

ActionResult GetScheduleAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)
  QString sql = "select id, target_date fdate, (substitute_date is NULL and timing_grid_id is NULL) \"isHoliday\", timing_grid_id, substitute_date, city_id from date_substitution where ";
  sql = sql.append(filterFromParams(params));
  QSqlQuery query = QSqlQueryHelper::execSql(sql, con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  return ActionResult("Данные расписания выходных и переносов", 0, sqlResultToJson(query).array());
}

bool PostScheduleAction::checkUrlMatch(QString path, QString method)
{
  Q_UNUSED(path)
  return method == HTTP_POST;
}

ActionResult PostScheduleAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{  
  QSqlDatabase db = QSqlDatabase::database(con.uid());
  bool tsAction = db.transaction();
  qDebug() << "Transaction started" << tsAction;
  QSqlError sqlError;

  QJsonArray submitArr = bodyData.toArray();
  foreach(QJsonValue submitItem, submitArr) {
    QJsonObject submitObj = submitItem.toObject();
    QString action = submitObj.value("action").toString();
    QSqlQuery query(db);
    QDate date = submitObj.value("fdate").toVariant().toDate();
    QVariant timingGridId = submitObj.value("timing_grid_id").toVariant();
    qlonglong cityId = params.value("city_id").toLongLong();
    QDate substituteDate = submitObj.value("substitute_date").toVariant().toDate();
//    bool isHoliday = submitObj.value("isHoliday").toBool();
    if (action == "insert") {
//      qDebug() << "Insert obj:" << submitObj;
      query.prepare("insert into date_substitution(target_date, city_id, timing_grid_id, substitute_date) values (:date, :city_id, :timing_grid_id, :substitute_date)");
      query.bindValue(":date", date);
      query.bindValue(":city_id", cityId);
      query.bindValue(":timing_grid_id", timingGridId);
      query.bindValue(":substitute_date", substituteDate);
    }
    else if (action == "delete") {
      qDebug() << "Delete obj:" << submitObj;
      query.prepare("delete from date_substitution where id=:id");
      query.bindValue(":id", submitObj.value("id").toVariant().toLongLong());
    }
    else if (action == "update") {
      qDebug() << "Update obj:" << submitObj;
      query.prepare("update date_substitution set timing_grid_id=:timing_grid_id, substitute_date=:substitute_date where id=:id");
      query.bindValue(":timing_grid_id", timingGridId);
      query.bindValue(":substitute_date", substituteDate);
      query.bindValue(":id", submitObj.value("id").toVariant().toLongLong());
    }
    else {
      qWarning() << "Incorrect submit data";
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


bool PostScheduleAction::hasAccess(const Context &con)
{
  return con.userType() < Teacher;
}
