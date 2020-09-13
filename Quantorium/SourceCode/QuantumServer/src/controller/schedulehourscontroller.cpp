#include "schedulehourscontroller.h"
#include "../core/appconst.h"
#include "../core/api.h"
#include "../utils/qsqlqueryhelper.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>


ScheduleHoursController::ScheduleHoursController(QString tableName, Context &con)
  : BaseRestTableController(tableName, con)
{
  unregisterAction(_tableName + "_insert");
  unregisterAction(_tableName + "_update");
  unregisterAction(_tableName + "_delete");
  registerAction(new BatchChangesSubmit("lesson_course_hours", _tableName + "_batch_submit", "Пакетное обновление записей"));
}


BatchChangesSubmit::BatchChangesSubmit(QString table, QString name, QString description, QStringList fieldFilters)
  : AbstractTableAction(table, name, description, fieldFilters)
{
  //Для отладки отключаем логирование
  //setLogging(false);
}

bool BatchChangesSubmit::checkUrlMatch(QString path, QString method)
{
  Q_UNUSED(path)
  return method == HTTP_POST;
}

ActionResult BatchChangesSubmit::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  QString delSql = "delete from learning_group_hours where learning_group_id=%1 and day_of_week=%2 and order_num=%3";
  QString insSql = "insert into learning_group_hours(learning_group_id, day_of_week, order_num) values (%1, %2, %3)";

  QJsonArray jArr = bodyData.toArray();
  foreach (QJsonValue jVal, jArr) {
    QJsonObject jObj = jVal.toObject();
    qlonglong lessonCourseId = params.value(PRM_LEARNING_GROUP_ID).toLongLong();
    int dayOfWeek = jObj.value("day_of_week").toInt();
    int orderNum = jObj.value("order_num").toInt();
    bool enabled = jObj.value("enabled").toBool();
    QString sql = (enabled ? insSql : delSql).arg(QString::number(lessonCourseId))
        .arg(QString::number(dayOfWeek)).arg(QString::number(orderNum));
    QSqlQueryHelper::execSql(sql, con.uid());
  }
  return ActionResult("OK");
}
