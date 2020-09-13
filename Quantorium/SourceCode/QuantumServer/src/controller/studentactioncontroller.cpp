#include "studentactioncontroller.h"
#include "../core/studiesdictionary.h"
#include "../utils/qsqlqueryhelper.h"
#include "../core/appconst.h"

#define DB_USER_COURSE_UQ "user_course_uq"
#define DB_USER_SKILL_UQ "user_skill_uq"

const QString SErrStudentAlreadyEnteredCourse = "Попытка повторной записи на курс";
const QString SErrStudentAlreadyEnrolledLessonCourse = "Попытка повторной записи в группу занятий";

StudentActionController::StudentActionController(Context &con) : BaseJsonActionController(con)
{
  registerAction(new EnterCourseAction("enterCourse", "Запись ученика на курс"));
  registerAction(new EnrolOnLessonCourse("enrolOnLessonCourse", "Запись ученика в группу занятий"));
}

bool StudentActionController::hasAccess()
{
  return true;
}

EnterCourseAction::EnterCourseAction(QString name, QString description) : AbstractJsonAction(name, description)
{
}

ActionResult EnterCourseAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_USER_ID) || !params.contains(PRM_COURSE_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_USER_ID ", " PRM_COURSE_ID), ERR_DEFAULT_CUSTOM);
  }
  QString sql = "insert into user_course(user_id, course_id) values (#user_id#, #course_id#)";
  QString preparedSql = QSqlQueryHelper::fillSqlPattern(sql, params);
  QSqlQuery query = QSqlQueryHelper::execSql(preparedSql, con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  return ActionResult(SInfoRegisterOnCourse);
}

ActionResult EnterCourseAction::handleDatabaseError(const QSqlError &error)
{
  QString errMsg = error.databaseText();
  if (errMsg.contains(DB_USER_COURSE_UQ)) {
    return ActionResult(SErrStudentAlreadyEnteredCourse, ERR_DATABASE);
  }
  else {
    return AbstractJsonAction::handleDatabaseError(error);
  }
}

EnrolOnLessonCourse::EnrolOnLessonCourse(QString name, QString description) : AbstractJsonAction(name, description)
{
}

ActionResult EnrolOnLessonCourse::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_USER_ID) || !params.contains(PRM_LESSON_COURSE_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_USER_ID ", " PRM_LESSON_COURSE_ID), ERR_DEFAULT_CUSTOM);
  }
  QString sql =
      "INSERT INTO user_skill(user_id, skill_id, lesson_course_id) "
      "SELECT %1, skill_id, %2 from lesson_course where id=%2";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_USER_ID).toString()).arg(params.value(PRM_LESSON_COURSE_ID).toString()), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  return ActionResult(SInfoEnrollOnLessonCourse);
}

ActionResult EnrolOnLessonCourse::handleDatabaseError(const QSqlError &error)
{
  QString errMsg = error.databaseText();
  if (errMsg.contains(DB_USER_SKILL_UQ)) {
    return ActionResult(SErrStudentAlreadyEnrolledLessonCourse, ERR_DATABASE);
  }
  else {
    return AbstractJsonAction::handleDatabaseError(error);
  }
}
