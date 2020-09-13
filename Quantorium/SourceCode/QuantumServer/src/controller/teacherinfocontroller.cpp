#include "teacherinfocontroller.h"
#include "../core/appconst.h"
#include "../utils/qsqlqueryhelper.h"
#include <QJsonArray>
#include <QJsonObject>

TeacherInfoController::TeacherInfoController(Context &con) : BaseJsonActionController(con)
{
  registerAction(new DayOffByCityAction("dayOffByCity", "Рабочие и выходные дни недели кванториума"));
}

bool TeacherInfoController::hasAccess()
{
  //Доступ для всех кроме учеников
  UserType userType = (UserType)_context.userType();
  return (userType == SuperUser)
      || (userType == Admin)
      || (userType == Director)
      || (userType == Teacher);
}


DayOffByCityAction::DayOffByCityAction(QString name, QString description) : AbstractJsonAction(name, description)
{

}


ActionResult DayOffByCityAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_CITY_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_CITY_ID), ERR_DEFAULT_CUSTOM);
  }

  QString sql = "select count(case when day_type=1 then 1 else null end) > 0 monday, "
      "count(case when day_type=2 then 1 else null end) > 0 tuesday, "
      "count(case when day_type=3 then 1 else null end) > 0 wednesday, "
      "count(case when day_type=4 then 1 else null end) > 0 thursday, "
      "count(case when day_type=5 then 1 else null end) > 0 friday, "
      "count(case when day_type=6 then 1 else null end) > 0 saturday, "
      "count(case when day_type=7 then 1 else null end) > 0 sunday "
      "from timing_grid_unit "
      "where timing_grid_id in (select id from timing_grid where city_id=%1 and city_default=true)";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_CITY_ID).toString()), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  query.next();
  return ActionResult("Рабочие дни кванториума", ERR_NO_ERROR, sqlRecordToJson(query.record()));
}
