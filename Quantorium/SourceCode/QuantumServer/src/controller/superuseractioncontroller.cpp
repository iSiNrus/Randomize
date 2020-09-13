#include "superuseractioncontroller.h"
#include "../core/api.h"
#include "../utils/qfileutils.h"
#include "../core/appsettings.h"
#include <QDate>


SuperUserActionController::SuperUserActionController(Context &con) : BaseJsonActionController(con)
{
}

void SuperUserActionController::service(HttpRequest &request, HttpResponse &response)
{
  BaseJsonSqlController::service(request, response);
}
