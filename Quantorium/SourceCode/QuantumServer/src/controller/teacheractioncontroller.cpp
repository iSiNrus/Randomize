#include "teacheractioncontroller.h"
#include "../core/appconst.h"
#include "../core/api.h"
#include "../utils/qsqlqueryhelper.h"
#include "../core/appsettings.h"
#include "../utils/qfileutils.h"
#include <QDate>


TeacherActionController::TeacherActionController(Context &con) : BaseJsonActionController(con)
{
  registerAction(new AssignUserCaseAction(ACT_ASSIGN_USER_CASE, "Запись ученика на кейс"));
  registerAction(new DeleteUserCaseAction(ACT_DELETE_USER_CASE, "Удалить ученика с кейса"));
  registerAction(new RegisterLessonAction(ACT_REGISTER_LESSON, "Зарегистрировать занятие"));
  registerAction(new UnregisterLessonAction(ACT_UNREGISTER_LESSON, "Удалить занятие"));
  registerAction(new RegisterLessonScoreAction(ACT_REGISTER_LESSON_SCORE, "Занесение информации об оценке ученика за занятие"));
  registerAction(new RegisterTaskScoreAction(ACT_REGISTER_TASK_SCORE, "Занесение информации об оценке ученика за задание"));
  registerAction(new UpdateLessonScoreAction(ACT_UPDATE_LESSON_SCORE, "Изменение оценки ученика за занятие"));
  registerAction(new UpdateTaskScoreAction(ACT_UPDATE_TASK_SCORE, "Изменение оценки ученика за задание"));
  registerAction(new DeleteLessonScoreAction(ACT_DELETE_LESSON_SCORE, "Удаление оценки ученика за занятие"));
  registerAction(new DeleteTaskScoreAction(ACT_DELETE_TASK_SCORE, "Удаление оценки ученика за задание"));
  registerAction(new CloneCaseAction(ACT_CLONE_CASE, "Создание копии существующего кейса"));
}

bool TeacherActionController::hasAccess()
{
  //Доступ для всех кроме учеников
  UserType userType = (UserType)_context.userType();
  return (userType == SuperUser)
      || (userType == Admin)
      || (userType == Director)
      || (userType == Teacher);
}

RegisterLessonScoreAction::RegisterLessonScoreAction(QString name, QString description) : AbstractJsonAction(name, description)
{
}

ActionResult RegisterLessonScoreAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)
  if (!params.contains(PRM_LESSON_ID) && !params.contains(PRM_SKILL_UNIT_ID)) {
    return ActionResult("Не хватает обязательного параметра lesson_id или skill_unit_id", ERR_DEFAULT_CUSTOM);
  }

  if (!params.contains(PRM_USER_ID) || !params.contains(PRM_SCORE)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_LESSON_ID ", " PRM_USER_ID ", " PRM_SCORE), ERR_DEFAULT_CUSTOM);
  }
  QString sql;
  if (params.contains(PRM_LESSON_ID)) {
    QString finishDate = "null";
    if (params.contains(PRM_FINISH_DATE)) {
      finishDate = "'" + params.value(PRM_FINISH_DATE).toString() + "'";
    }
    sql = "insert into user_lesson (user_id, score, lesson_id, skill_unit_id, finish_date, lesson_course_id) "
          "select %1, %2, l.id lesson_id, l.skill_unit_id, coalesce(%4, l.lesson_date), l.lesson_course_id "
          "from lesson l where l.id=%3";
    sql = sql.arg(params.value(PRM_USER_ID).toString()).arg(params.value(PRM_SCORE).toString()).arg(params.value(PRM_LESSON_ID).toString())
        .arg(finishDate);
  }
  else {
    sql = "insert into user_lesson (user_id, lesson_id, skill_unit_id, lesson_course_id, finish_date, score) "
        "select %1, id lesson_id, skill_unit_id, lesson_course_id, lesson_date, %3 from lesson where skill_unit_id=%2 "
        "and lesson_course_id in (select lesson_course_id from user_skill where user_id=%1)";
    sql = sql.arg(params.value(PRM_USER_ID).toString()).arg(params.value(PRM_SKILL_UNIT_ID).toString()).arg(params.value(PRM_SCORE).toString());
  }
  QSqlQuery query = QSqlQueryHelper::execSql(sql, con.uid());
  if (query.lastError().isValid()) {    
    return handleDatabaseError(query.lastError());
  }
  else {
    return ActionResult("Оценка за занятие выставлена");
  }
}

ActionResult RegisterLessonScoreAction::handleDatabaseError(const QSqlError &error)
{
  QString errMsg = error.databaseText();
  if (errMsg.contains("user_lesson_uq")) {
    return ActionResult("Оценка этому ученику по данному уроку уже была выставлена", ERR_DATABASE);
  }
  else {
    return AbstractJsonAction::handleDatabaseError(error);
  }
}

RegisterTaskScoreAction::RegisterTaskScoreAction(QString name, QString description) : AbstractJsonAction(name, description)
{
}

ActionResult RegisterTaskScoreAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_TASK_ID) || !params.contains(PRM_USER_ID) || !params.contains(PRM_SCORE)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_TASK_ID ", " PRM_USER_ID ", " PRM_SCORE), ERR_DEFAULT_CUSTOM);
  }
  QDate finishDate = params.contains(PRM_TASK_DATE) ? params.value(PRM_TASK_DATE).toDate() : QDate::currentDate();
  QString sql = "insert into user_task (user_id, task_id, score, finish_date, completed) "
      "values (%1, %2, %3, '%4', true)";
  QString preparedSql = sql.arg(params.value(PRM_USER_ID).toString())
      .arg(params.value(PRM_TASK_ID).toString())
      .arg(params.value(PRM_SCORE).toString()).arg(finishDate.toString(Qt::ISODate));
  QSqlQuery query = QSqlQueryHelper::execSql(preparedSql, con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  else {
    return ActionResult("Оценка за задание выставлена");
  }
}

ActionResult RegisterTaskScoreAction::handleDatabaseError(const QSqlError &error)
{
  QString errMsg = error.databaseText();
  if (errMsg.contains("user_task_uq")) {
    return ActionResult("Оценка этому ученику за данное задание уже была выставлена", ERR_DATABASE);
  }
  else {
    return AbstractJsonAction::handleDatabaseError(error);
  }
}

UpdateLessonScoreAction::UpdateLessonScoreAction(QString name, QString description) : AbstractJsonAction(name, description)
{
}

ActionResult UpdateLessonScoreAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_LESSON_ID) || !params.contains(PRM_USER_ID) || !params.contains(PRM_SCORE)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_LESSON_ID ", " PRM_USER_ID ", " PRM_SCORE), ERR_DEFAULT_CUSTOM);
  }
  QString sql = "update user_lesson set score=%1 where user_id=%2 and lesson_id=%3";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_SCORE).toString()).arg(params.value(PRM_USER_ID).toString()).arg(params.value(PRM_LESSON_ID).toString()), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  else if (query.numRowsAffected() == 0) {
    return ActionResult("У данного ученика нет оценки за данное занятие", ERR_DEFAULT_CUSTOM);
  }
  else {
    return ActionResult("Оценка за занятие выставлена");
  }
}

ActionResult UpdateLessonScoreAction::handleDatabaseError(const QSqlError &error)
{
  return AbstractJsonAction::handleDatabaseError(error);
}

UpdateTaskScoreAction::UpdateTaskScoreAction(QString name, QString description) : AbstractJsonAction(name, description)
{
}

ActionResult UpdateTaskScoreAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_TASK_ID) || !params.contains(PRM_USER_ID) || !params.contains(PRM_SCORE)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_TASK_ID ", " PRM_USER_ID ", " PRM_SCORE), ERR_DEFAULT_CUSTOM);
  }
  QString sql = "update user_task set score=%1 where user_id=%2 and task_id=%3";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_SCORE).toString()).arg(params.value(PRM_USER_ID).toString()).arg(params.value(PRM_TASK_ID).toString()), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  else if (query.numRowsAffected() == 0) {
    return ActionResult("У данного ученика нет оценки за данное задание", ERR_DEFAULT_CUSTOM);
  }
  else {
    return ActionResult("Оценка за задание выставлена");
  }
}

ActionResult UpdateTaskScoreAction::handleDatabaseError(const QSqlError &error)
{
  return AbstractJsonAction::handleDatabaseError(error);
}

DeleteLessonScoreAction::DeleteLessonScoreAction(QString name, QString description) : AbstractJsonAction(name, description)
{
}

ActionResult DeleteLessonScoreAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_LESSON_ID) || !params.contains(PRM_USER_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_LESSON_ID ", " PRM_USER_ID), ERR_DEFAULT_CUSTOM);
  }
  QString sql = "delete from user_lesson where user_id=%1 and lesson_id=%2";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_USER_ID).toString()).arg(params.value(PRM_LESSON_ID).toString()), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  else if (query.numRowsAffected() == 0) {
    return ActionResult("У данного ученика нет оценки за данное занятие", ERR_DEFAULT_CUSTOM);
  }
  else {
    return ActionResult("Оценка за занятие удалена");
  }
}

DeleteTaskScoreAction::DeleteTaskScoreAction(QString name, QString description) : AbstractJsonAction(name, description)
{
}

ActionResult DeleteTaskScoreAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_TASK_ID) || !params.contains(PRM_USER_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_TASK_ID ", " PRM_USER_ID), ERR_DEFAULT_CUSTOM);
  }
  QString sql = "delete from user_task where user_id=%1 and task_id=%2";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_USER_ID).toString()).arg(params.value(PRM_TASK_ID).toString()), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  else if (query.numRowsAffected() == 0) {
    return ActionResult("У данного ученика нет оценки за данное задание", ERR_DEFAULT_CUSTOM);
  }
  else {
    return ActionResult("Оценка за задание удалена");
  }
}

AssignUserCaseAction::AssignUserCaseAction(QString name, QString description) : AbstractJsonAction(name, description)
{
}

ActionResult AssignUserCaseAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_CASE_ID) || !params.contains(PRM_USER_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_CASE_ID ", " PRM_USER_ID), ERR_DEFAULT_CUSTOM);
  }

  QString sql = "insert into user_case (user_id, case_id) values (%1, %2)";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_USER_ID).toString()).arg(params.value(PRM_CASE_ID).toString()), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  else {
    return ActionResult("Ученик записан на кейс");
  }
}

DeleteUserCaseAction::DeleteUserCaseAction(QString name, QString description) : AbstractJsonAction(name, description)
{
}

ActionResult DeleteUserCaseAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_CASE_ID) || !params.contains(PRM_USER_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_CASE_ID ", " PRM_USER_ID), ERR_DEFAULT_CUSTOM);
  }

  QString sql = "delete from user_case where user_id=%1 and case_id=%2";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_USER_ID).toString()).arg(params.value(PRM_CASE_ID).toString()), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  else {
    return ActionResult("Ученик отписан от кейса");
  }
}


RegisterLessonAction::RegisterLessonAction(QString name, QString description) : AbstractJsonAction(name, description)
{
}

ActionResult RegisterLessonAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_LEARNING_GROUP_ID) || !params.contains(PRM_LESSON_DATE) || !params.contains(PRM_LESSON_TIME)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_LEARNING_GROUP_ID ", " PRM_LESSON_DATE ", " PRM_LESSON_TIME), ERR_DEFAULT_CUSTOM);
  }

  QString sql = "insert into lesson (learning_group_id, lesson_date, lesson_time) values (%1, '%2', '%3')";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_LEARNING_GROUP_ID).toString())
                                             .arg(params.value(PRM_LESSON_DATE).toString())
                                             .arg(params.value(PRM_LESSON_TIME).toString()), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  else {
    return ActionResult("Занятие зарегистрировано");
  }
}

UnregisterLessonAction::UnregisterLessonAction(QString name, QString description) : AbstractJsonAction(name, description)
{
}

ActionResult UnregisterLessonAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_LESSON_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_LESSON_ID), ERR_DEFAULT_CUSTOM);
  }

  QString sql = "delete from lesson where id=%1";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_LESSON_ID).toString()), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  else if (query.numRowsAffected() == 0) {
    return ActionResult("Занятия с таким ID не найдено", ERR_DEFAULT_CUSTOM);
  }
  else {
    return ActionResult("Занятие группы удалено");
  }
}


CloneCaseAction::CloneCaseAction(QString name, QString description) : AbstractJsonAction(name, description)
{
}

ActionResult CloneCaseAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_CASE_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_CASE_ID), ERR_DEFAULT_CUSTOM);
  }
  QVariant sourceCaseId = params.value(PRM_CASE_ID);

  QString sql = "select clone_case(%1, %2)";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(sourceCaseId.toString()).arg(con.sysuserId()), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  int newCaseId = query.next() ? query.value(0).toInt() : -1;
  if (newCaseId <= 0)
    return ActionResult("Не удалось клонировать кейс", ERR_DEFAULT_CUSTOM);

  QString docroot = AppSettings::strVal("docroot", "path", "docroot");
  QString srcDir = AppSettings::settingsPath() + docroot + DELIMITER CTRL_ATTACHMENTS DELIMITER "case" DELIMITER "private" DELIMITER + sourceCaseId.toString();
  QString dstDir = AppSettings::settingsPath() + docroot + DELIMITER CTRL_ATTACHMENTS DELIMITER "case" DELIMITER "private" DELIMITER + QString::number(newCaseId);
  bool attachmentCopied = QFileUtils::copyDirectory(srcDir, dstDir, true);

  srcDir = AppSettings::settingsPath() + docroot + DELIMITER CTRL_ATTACHMENTS DELIMITER "case" DELIMITER "public" DELIMITER + sourceCaseId.toString();
  dstDir = AppSettings::settingsPath() + docroot + DELIMITER CTRL_ATTACHMENTS DELIMITER "case" DELIMITER "public" DELIMITER + QString::number(newCaseId);
  attachmentCopied = QFileUtils::copyDirectory(srcDir, dstDir, true);

  srcDir = AppSettings::settingsPath() + docroot + DELIMITER CTRL_ATTACHMENTS DELIMITER "case" DELIMITER "icon" DELIMITER + sourceCaseId.toString();
  dstDir = AppSettings::settingsPath() + docroot + DELIMITER CTRL_ATTACHMENTS DELIMITER "case" DELIMITER "icon" DELIMITER + QString::number(newCaseId);
  attachmentCopied = QFileUtils::copyDirectory(srcDir, dstDir, true);

  sql = "select cu1.id case_unit1, t1.id task1, cu2.id case_unit2, t2.id task2 "
  "from case_unit cu1 "
  "left join task t1 on cu1.id=t1.case_unit_id "
  "left join case_unit cu2 on cu2.case_id=%1 and cu2.sort_id=cu1.sort_id "
  "left join task t2 on cu2.id=t2.case_unit_id and t1.sort_id=t2.sort_id "
  "where cu1.case_id=%2";
  query = QSqlQueryHelper::execSql(sql.arg(newCaseId).arg(sourceCaseId.toInt()), con.uid());

  int lastUnitId = -1;
  while (query.next()) {
    int srcUnitId = query.value("case_unit1").toInt();
    int dstUnitId = query.value("case_unit2").toInt();
    if (lastUnitId != srcUnitId) {
      srcDir = AppSettings::settingsPath() + docroot + DELIMITER CTRL_ATTACHMENTS DELIMITER "unit" DELIMITER;
      copyUnitResources(srcDir, srcUnitId, dstUnitId);
      lastUnitId = srcUnitId;
    }

    int srcTaskId = query.value("task1").toInt();
    int dstTaskId = query.value("task2").toInt();
    if (srcTaskId > 0) {
      srcDir = AppSettings::settingsPath() + docroot + DELIMITER CTRL_ATTACHMENTS DELIMITER "task" DELIMITER;
      copyTaskResources(srcDir, srcTaskId, dstTaskId);
    }
  }

  return ActionResult("Кейс успешно клонирован", ERR_NO_ERROR, QJsonValue::fromVariant(newCaseId));
}

bool CloneCaseAction::copyUnitResources(QString unitDir, int srcUnitId, int dstUnitId)
{
  qDebug() << "Copy unit" << srcUnitId << "->" << dstUnitId;
  QString srcDir = unitDir + QString::number(srcUnitId);
  QString dstDir = unitDir + QString::number(dstUnitId);
  return QFileUtils::copyDirectory(srcDir, dstDir, true);
}

bool CloneCaseAction::copyTaskResources(QString taskDir, int srcTaskId, int dstTaskId)
{
  qDebug() << "Copy task" << srcTaskId << "->" << dstTaskId;
  QString srcDir = taskDir + QString::number(srcTaskId);
  QString dstDir = taskDir + QString::number(dstTaskId);
  return QFileUtils::copyDirectory(srcDir, dstDir, true);
}
