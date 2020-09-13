#include "adminonlyresttablecontroller.h"
#include "../core/appconst.h"

AdminOnlyRestTableController::AdminOnlyRestTableController(QString tableName, Context &con)
  : BaseRestTableController(tableName, con)
{
}

bool AdminOnlyRestTableController::hasAccess()
{
  //Доступ для всех кроме учеников
  UserType userType = (UserType)_context.userType();
  return (userType == SuperUser)
      || (userType == Admin)
      || (userType == Director)
      || (userType == Teacher);
}
