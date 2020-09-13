#include "casetablecontroller.h"


CaseTableController::CaseTableController(QString tableName, Context &con) : BaseRestTableController(tableName, con)
{

}

bool CaseTableController::hasAccess()
{
  if (method() == HTTP_GET)
    return true;

  return _context.userType() != Student;
}

QString CaseTableController::filterFromParams(const QSqlRecord &infoRec)
{
  QString filter = BaseRestTableController::filterFromParams(infoRec);
  switch (_context.userType()) {
  case SuperUser:
    break;
  case Director:
    filter = filter + QString(" and author_id in (select id from sysuser where city_id in (select id from city where region_id=%1))").arg(_context.region_id());
    break;
  case Admin:
  case Teacher:
  case Student:
    filter = filter + QString(" and author_id in (select id from sysuser where city_id=%1)").arg(_context.city_id());
    break;
  default:
    Q_ASSERT_X(false, "filterFromParams", "Unknown user type");
  }
  return filter;
}
