#include "adminactioncontroller.h"
#include "../core/appconst.h"
#include "../core/api.h"
#include "../utils/qsqlqueryhelper.h"
#include <QJsonArray>

#define DB_USER_LEARNING_GROUP_UQ "uq_user_learning_group"

const QString SErrUserAlreadyRegisteredForLearningGroup = "Ученик уже записан в данную группу";

AdminActionController::AdminActionController(Context &con) : BaseJsonActionController(con)
{
  registerAction(new RegisterUserForLearningGroup(ACT_REGISTER_USER_FOR_LEARNING_GROUP, "Запись ученика в группу занятий"));
  registerAction(new UnregisterUserFromLearningGroup(ACT_UNREGISTER_USER_FROM_LEARNING_GROUP, "Отписать ученика от группы занятий"));
  registerAction(new ActivateCaseAction(ACT_ACTIVATE_CASE, "Утвердить черновик кейса"));
  registerAction(new DeactivateCaseAction(ACT_DEACTIVATE_CASE, "Деактивировать (отправить в архив) кейс"));
  registerAction(new RemoveRequiredCaseAchievementAction(ACT_REMOVE_REQUIRED_CASE_ACHIEVEMENTS, "Удалить зависимые достижения из требуемых для кейса"));
}

bool AdminActionController::hasAccess()
{
  //Доступ для администраторов
  UserType userType = (UserType)_context.userType();
  return (userType == SuperUser) || (userType == Admin) || (userType == Director);
}

RegisterUserForLearningGroup::RegisterUserForLearningGroup(QString name, QString description) : AbstractJsonAction(name, description)
{
}

UnregisterUserFromLearningGroup::UnregisterUserFromLearningGroup(QString name, QString description) : AbstractJsonAction(name, description)
{
}

ActionResult RegisterUserForLearningGroup::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_LEARNING_GROUP_ID) || !params.contains(PRM_USER_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_LEARNING_GROUP_ID ", " PRM_USER_ID), ERR_DEFAULT_CUSTOM);
  }

  QString sql = "insert into user_learning_group (user_id, learning_group_id) values (%1, %2)";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_USER_ID).toString()).arg(params.value(PRM_LEARNING_GROUP_ID).toString()), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  else {
    return ActionResult("Ученик записан в группу занятий");
  }
}

ActionResult UnregisterUserFromLearningGroup::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_LEARNING_GROUP_ID) || !params.contains(PRM_USER_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_LEARNING_GROUP_ID ", " PRM_USER_ID), ERR_DEFAULT_CUSTOM);
  }

  QString sql = "delete from user_learning_group where user_id=%1 and learning_group_id=%2";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_USER_ID).toString()).arg(params.value(PRM_LEARNING_GROUP_ID).toString()), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  else if (query.numRowsAffected() == 0) {
    return ActionResult("Данный ученик не записан в указанную группу занятий", 100);
  }
  else {
    return ActionResult("Ученик отписан от группы занятий");
  }
}


ActionResult RegisterUserForLearningGroup::handleDatabaseError(const QSqlError &error)
{
  QString errMsg = error.databaseText();
  if (errMsg.contains(DB_USER_LEARNING_GROUP_UQ)) {
    return ActionResult(SErrUserAlreadyRegisteredForLearningGroup, ERR_DATABASE);
  }
  else {
    return AbstractJsonAction::handleDatabaseError(error);
  }
}


ActivateCaseAction::ActivateCaseAction(QString name, QString description) : AbstractJsonAction(name, description)
{
}

ActionResult ActivateCaseAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_CASE_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_CASE_ID), ERR_DEFAULT_CUSTOM);
  }

  QString sql = "select c.id case_id, su.id sysuser_id, c2.id city_id, c2.region_id "
      "from \"case\" c "
      "left join sysuser su on c.author_id=su.id "
      "left join city c2 on su.city_id=c2.id "
      "where c.id=%1 and c.status=0";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_CASE_ID).toString()), con.uid());
  if (query.next()) {
    switch (con.userType()) {
    case SuperUser:
      break;
    case Director: {
      int cityId = query.value("city_id").toInt();
      if (cityId != con.city_id())
        return ActionResult("Нет прав на активацию кейса из другого города", ERR_DEFAULT_CUSTOM);
      break;
    }
    case Admin: {
      int regionId = query.value("region_id").toInt();
      if (regionId != con.region_id())
        return ActionResult("Нет прав на активацию кейса из другого региона", ERR_DEFAULT_CUSTOM);
      break;
    }
    default:
      Q_ASSERT_X(false, "ActivateCaseAction", "Unknown user type");
    }
  }
  else {
    return ActionResult("Кейс не найден, уже утвержден или деактивирован", ERR_DEFAULT_CUSTOM);
  }

  sql = "update \"case\" set status=1 where id=%1";
  query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_CASE_ID).toString()), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  else {
    return ActionResult("Кейс успешно утвержден");
  }
}


DeactivateCaseAction::DeactivateCaseAction(QString name, QString description) : AbstractJsonAction(name, description)
{

}

ActionResult DeactivateCaseAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_CASE_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_CASE_ID), ERR_DEFAULT_CUSTOM);
  }
  QString sql = "select c.id case_id, su.id sysuser_id, c2.id city_id, c2.region_id "
      "from \"case\" c "
      "left join sysuser su on c.author_id=su.id "
      "left join city c2 on su.city_id=c2.id "
      "where c.id=%1 and c.status=1";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_CASE_ID).toString()), con.uid());
  if (query.next()) {
    switch (con.userType()) {
    case SuperUser:
      break;
    case Director: {
      int cityId = query.value("city_id").toInt();
      if (cityId != con.city_id())
        return ActionResult("Нет прав на активацию кейса из другого города", ERR_DEFAULT_CUSTOM);
      break;
    }
    case Admin: {
      int regionId = query.value("region_id").toInt();
      if (regionId != con.region_id())
        return ActionResult("Нет прав на активацию кейса из другого региона", ERR_DEFAULT_CUSTOM);
      break;
    }
    default:
      Q_ASSERT_X(false, "DeactivateCaseAction", "Unknown user type");
    }
  }
  else {
    return ActionResult("Кейс не найден, не утвержден или уже деактивирован", ERR_DEFAULT_CUSTOM);
  }
  sql = "update \"case\" set status=2 where id=%1";
  query = QSqlQueryHelper::execSql(sql.arg(params.value(PRM_CASE_ID).toString()), con.uid());
  if (query.lastError().isValid()) {
    return handleDatabaseError(query.lastError());
  }
  else {
    return ActionResult("Кейс успешно деактивирован");
  }
}

RemoveRequiredCaseAchievementAction::RemoveRequiredCaseAchievementAction(QString name, QString desctiption) : AbstractJsonAction(name, desctiption)
{
}


ActionResult RemoveRequiredCaseAchievementAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_CASE_ID) || !params.contains(PRM_ACHIEVEMENT_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_CASE_ID ", " PRM_ACHIEVEMENT_ID), ERR_DEFAULT_CUSTOM);
  }
  QString sql = "SELECT delete_required_case_achievements(%1, %2)";
  QString caseId = params.value(PRM_CASE_ID).toString();
  QString achievementId = params.value(PRM_ACHIEVEMENT_ID).toString();
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(caseId, achievementId), con.uid());
  bool resOk = query.lastError().type() == QSqlError::NoError;
  if (!resOk)
    return handleDatabaseError(query.lastError());
  else
    return ActionResult("Зависимые достижения удалены", ERR_NO_ERROR);
}
