#include "cityrestrictedtablecontroller.h"
#include "../core/appconst.h"


CityRestrictedTableController::CityRestrictedTableController(QString tableName, Context &con)
  : BaseRestTableController(tableName, con)
{
}

QString CityRestrictedTableController::filterFromParams(const QSqlRecord &infoRec)
{
  QString filter = BaseRestTableController::filterFromParams(infoRec);
  switch (_context.userType()) {
  case SuperUser:
    return filter;
  case Admin:
  case Director: {
    QString regionFilter = " and city_id in (select id from city where region_id=(select region_id from city where id=%1))";
    return filter + regionFilter.arg(QString::number(_context.city_id()));
  }
  case Teacher:
  case Student:
  {
    QString cityFilter = " and city_id=%1";
    return filter + cityFilter.arg(QString::number(_context.city_id()));
  }
  default:
    Q_ASSERT_X(false, "filterFromParams", "Unknown user type");
  }
}

bool CityRestrictedTableController::hasAccess()
{
  if (method() == HTTP_GET)
    return true;
  return (_context.userType() != Student) && (_context.userType() != Teacher);
}
