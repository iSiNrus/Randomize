#include "studentinfocontoller.h"
#include "../core/studiesdictionary.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDate>
#include <QSqlError>
#include "../utils/qsqlqueryhelper.h"
#include "../core/appconst.h"
#include "../core/api.h"

extern StudyStructureCache* studyStructureCache;

StudentInfoContoller::StudentInfoContoller(Context &con) : BaseJsonSqlController(con)
{

}

void StudentInfoContoller::service(HttpRequest &request, HttpResponse &response)
{
  BaseJsonSqlController::service(request, response);
  if (response.getStatusCode() != 200)
    return;
  QString path = QString::fromLocal8Bit(request.getPath());
  if (path.endsWith("/learningGroupCasesForStudent")) {
    if (!hasParam(PRM_USER_ID) || !hasParam(PRM_LEARNING_GROUP_ID)) {
      writeError(response, SErrNotEnoughParams.arg(PRM_USER_ID ", " PRM_LEARNING_GROUP_ID));
      return;
    }
    qlonglong userId = paramLong(PRM_USER_ID);
    qlonglong learningGroupId = paramLong(PRM_LEARNING_GROUP_ID);
    QString sql = "select lgc.*, "
        "exists(select 'x' from case_required_achievement cra left join user_achievement ua on cra.achievement_id=ua.achievement_id and user_id=%1 where cra.case_id=lgc.case_id and ua.id is null) unavailable, "
        "exists(select 'x' from user_case uc where uc.user_id=%1 and uc.case_id=lgc.case_id) done "
        "from learning_group_case lgc "
        "where lgc.learning_group_id=%2";
    QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(userId).arg(learningGroupId), _context.uid());
    QJsonDocument jDoc = sqlResultToJson(query);
    response.write(jDoc.toJson());
  } else if (path.endsWith("/studentSummary")) {
      if (!hasParam(PRM_USER_ID)) {
        writeError(response, SErrNotEnoughParams.arg(PRM_USER_ID));
        return;
      }
      qlonglong userId = paramLong(COL_USER_ID);
      QSqlQuery query = QSqlQueryHelper::execSql(SqlStudentSummary.arg(QString::number(userId)), _context.uid());
      QJsonObject jSummary;
      if (query.next()) {
          jSummary = sqlRecordToJson(query.record());
      }

      const QString sAchievQuery = "select achievement_id from user_achievement ua "
                                   "where ua.user_id=%1 group by ua.achievement_id;";
      query = QSqlQueryHelper::execSql(sAchievQuery.arg(QString::number(userId)), _context.uid());
      QJsonArray jAchiev;
      while (query.next()) {
          jAchiev.append(sqlRecordToJson(query.record()));
      }
      jSummary.insert("achievements", jAchiev);

      response.write(QJsonDocument(jSummary).toJson());
  }
  else {
    writeError(response, SErrUrlHasNoHandler.arg(path), 404);
    response.setStatus(404);
  }
}

bool StudentInfoContoller::hasAccess()
{
    return true;
}

QJsonArray StudentInfoContoller::scheduleArrayFromRec(QSqlRecord rec, qlonglong timingGridId, qlonglong cityId)
{
  TimingGrid timingGrid;
  {
    QSqlDatabase db = _context.db();
    if (timingGridId > 0)
      timingGrid = TimingGridDict::instance()->getById(timingGridId, db);
    else
      timingGrid = TimingGridDict::instance()->getDefaultByCity(cityId, db);
  }
  QJsonArray jScheduleArr;
  jScheduleArr.append(scheduleBitmaskArr(1, rec.value(COL_MONDAY_BITMASK).toInt(), timingGrid));
  jScheduleArr.append(scheduleBitmaskArr(2, rec.value(COL_TUESDAY_BITMASK).toInt(), timingGrid));
  jScheduleArr.append(scheduleBitmaskArr(3, rec.value(COL_WEDNESDAY_BITMASK).toInt(), timingGrid));
  jScheduleArr.append(scheduleBitmaskArr(4, rec.value(COL_THURSDAY_BITMASK).toInt(), timingGrid));
  jScheduleArr.append(scheduleBitmaskArr(5, rec.value(COL_FRIDAY_BITMASK).toInt(), timingGrid));
  jScheduleArr.append(scheduleBitmaskArr(6, rec.value(COL_SATURDAY_BITMASK).toInt(), timingGrid));
  jScheduleArr.append(scheduleBitmaskArr(7, rec.value(COL_SUNDAY_BITMASK).toInt(), timingGrid));

  return jScheduleArr;
}

QJsonArray StudentInfoContoller::scheduleBitmaskArr(int dayOfWeek, int bitmask, const TimingGrid &timingGrid)
{
  QJsonArray hoursArr;
  QBitArray bitArray = toQBit(bitmask);
  for(int i=1; i<=10; i++) {
    if (bitArray.testBit(i)) {
      QJsonObject hourObj;
      QTime beginTime = timingGrid.startTime(dayOfWeek, i);
      hourObj.insert(PRM_BEGIN_TIME, QJsonValue::fromVariant(beginTime));
      hourObj.insert(PRM_END_TIME, QJsonValue::fromVariant(beginTime.addSecs(timingGrid.duration()*60)));
      hourObj.insert(PRM_HOUR_NUMBER, QJsonValue::fromVariant(i));
      hoursArr.append(hourObj);
    }
  }
  return hoursArr;
}




