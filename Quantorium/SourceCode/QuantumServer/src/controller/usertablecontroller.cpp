#include "usertablecontroller.h"
#include "../core/appconst.h"
#include "../core/api.h"
#include "../utils/qsqlqueryhelper.h"

#define COL_TEACHER_ID "teacher_id"

QString SErrNoRightsToModifyUser = "Недостаточно прав для редактирования данного пользователя";
QString SErrNoRightToCreateUser = "Данный пользователь не может заводить пользователей с такими правами";
QString SErrNoRightsToCreateUserInAnotherCity = "Данный пользователь не может заводить пользователей в других городах";

UserTableController::UserTableController(QString tableName, Context &con) : CityRestrictedTableController(tableName, con)
{
  replaceAction(new UserUpdateAction(_tableName, _tableName + "_update", "Обновление пользователя с ограничениями по правам", _fieldFilters));
  replaceAction(new UserDeleteAction(_tableName, _tableName + "_delete", "Удаление пользователя с ограничениями по правам", _fieldFilters));
  replaceAction(new UserInsertAction(_tableName, _tableName + "_insert", "Добавление пользователя с ограничениями по правам", _fieldFilters));
  //Присоединенные поля
  setAttachedField("region_id", "(select region_id from city where id=city_id)");
}

bool UserTableController::hasAccess()
{
  if (!CityRestrictedTableController::hasAccess())
    return false;
  //Удалять пользователей может только суперпользователь
  if (method() == HTTP_DELETE) {
    return _context.userType() == SuperUser;
  }

  if ((_context.userType() == Student || _context.userType() == Teacher)
      && hasParam(PRM_USER_TYPE_ID) && paramLong(PRM_USER_TYPE_ID) < Teacher) {
    //Ученики и учителя могут видеть только учеников и учителей
    qWarning() << "Пользователь с правами Ученик или Учитель пытается получить данные пользователей более высокого уровня";
  }
  return true;
}

QString UserTableController::filterFromParams(const QSqlRecord &infoRec)
{
  QString filter = CityRestrictedTableController::filterFromParams(infoRec);
  if (_context.userType() == Student || _context.userType() == Teacher) {
    QString userTypeFilter = " and sysuser_type_id in (%1, %2)";
    filter += userTypeFilter.arg(QString::number(Student), QString::number(Teacher));
  }
  if (_context.userType() != SuperUser) {
    filter += " and sysuser_type_id<>0";
  }
  return filter;
}

UserUpdateAction::UserUpdateAction(QString table, QString name, QString description, QStringList fieldFilters)
  : UpdateAction(table, name, description, fieldFilters)
{
}

ActionResult UserUpdateAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  QString path = params.value(PRM_PATH).toString();
  int editingUserId = path.section("/", -1).toInt();
  QJsonObject newRecObj = bodyData.toObject();
  if (newRecObj.isEmpty())
    return ActionResult(SErrLackOfDataInRequest, ERR_LACK_OF_DATA);
  QJsonObject oldRecObj = recordObjById(con, editingUserId);
  int oldUserType = oldRecObj.value(PRM_USER_TYPE_ID).toVariant().toInt();
  int newUserType = newRecObj.value(PRM_USER_TYPE_ID).toVariant().toInt();
  if (con.userType() != SuperUser && con.userType() >= oldUserType)
    return ActionResult(SErrNoRightsToModifyUser, ERR_NOT_PERMITED);
  qDebug() << "User type:" << oldUserType << "->" << newUserType;
  if (oldUserType != newUserType && con.userType() != SuperUser)
    return ActionResult("Только суперпользователь может редактировать права других пользователей", ERR_NOT_PERMITED);
  if (newRecObj.contains(PRM_BALANCE) && compareFieldValues(oldRecObj, newRecObj, "balance") != 0)
    return ActionResult("Нельзя менять остаток коинов ученика", ERR_NOT_PERMITED);
  return UpdateAction::doInternalAction(con, params, bodyData);
}

UserDeleteAction::UserDeleteAction(QString table, QString name, QString description, QStringList fieldFilters)
  : DeleteAction(table, name, description, fieldFilters)
{
}

ActionResult UserDeleteAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  QString path = params.value(PRM_PATH).toString();
  int editingUserId = path.section("/", -1).toInt();
  QJsonObject oldRecObj = recordObjById(con, editingUserId);
  int oldUserType = oldRecObj.value(PRM_USER_TYPE_ID).toVariant().toInt();
  if (con.userType() >= oldUserType)
    return ActionResult(SErrNoRightsToModifyUser, ERR_NOT_PERMITED);
  return DeleteAction::doInternalAction(con, params, bodyData);
}

UserInsertAction::UserInsertAction(QString table, QString name, QString description, QStringList fieldFilters)
  : InsertAction(table, name, description, fieldFilters)
{
}

ActionResult UserInsertAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  QJsonObject newRecObj = bodyData.toObject();
  if (newRecObj.isEmpty())
    return ActionResult(SErrLackOfDataInRequest, ERR_LACK_OF_DATA);
  qDebug() << "New user:" << newRecObj;
  int newUserType = newRecObj.value(PRM_USER_TYPE_ID).toVariant().toInt();
  if (con.userType() > newUserType)
    return ActionResult(SErrNoRightToCreateUser, ERR_NOT_PERMITED);
  int newUserCityId = newRecObj.value(COL_CITY_ID).toInt();
  //По идее отсекается на клиенте
  if (con.userType() >= Admin && newUserCityId != con.city_id()) {
    qWarning() << "Should be excluded on client side: Inappropriate user city";
    return ActionResult(SErrNoRightsToCreateUserInAnotherCity, ERR_NOT_PERMITED);
  }
  return InsertAction::doInternalAction(con, params, bodyData);
}
