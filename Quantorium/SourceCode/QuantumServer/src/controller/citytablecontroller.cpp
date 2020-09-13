#include "citytablecontroller.h"

CityTableController::CityTableController(QString tableName, Context &con)
  : BaseRestTableController(tableName, con)
{

}

QString CityTableController::filterFromParams(const QSqlRecord &infoRec)
{
  QString filter = BaseRestTableController::filterFromParams(infoRec);
  switch (_context.userType()) {
  case SuperUser:
    return filter;
  case Admin:
  case Director: {
    QString regionFilter = " and region_id=%1";
    return filter + regionFilter.arg(QString::number(_context.region_id()));
  }
  case Teacher:
  case Student:
  {
    QString cityFilter = " and id=%1";
    return filter + cityFilter.arg(QString::number(_context.city_id()));
  }
  default:
    Q_ASSERT_X(false, "filterFromParams", "Unknown user type");
  }
}

bool CityTableController::hasAccess()
{
  if (method() == HTTP_GET)
    return true;
  return (_context.userType() != Student) && (_context.userType() != Teacher);
}
