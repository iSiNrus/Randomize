#include "actionlogtablecontroller.h"


ActionLogTableController::ActionLogTableController(QString tableName, Context &con) : BaseRestTableController(tableName, con)
{

}


bool ActionLogTableController::hasAccess()
{
  return _context.userType() == SuperUser;
}
