#include "courseadmininfocontroller.h"
#include "../utils/qsqlqueryhelper.h"
#include "../core/appconst.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>


CourseAdminInfoController::CourseAdminInfoController(Context &con) : BaseJsonSqlController(con)
{
}


void CourseAdminInfoController::service(HttpRequest &request, HttpResponse &response)
{
  BaseJsonSqlController::service(request, response);
  if (response.getStatusCode() != 200)
    return;
  //TODO: Проверку на метод (либо GET либо POST)
  QString sql = "select * from course ";
  //TODO: Придумать, как указывать допустимые параметры фильтрации
//  if (!params().isEmpty()) {
//    sql.append(" where ");
//    sql.append(filterFromParams(params()));
//  }
  sql.append(" order by original_id, actual desc, locked");
  QSqlQuery query = QSqlQueryHelper::execSql(sql, _context.uid());
  if (query.lastError().isValid()) {
    writeError(response, query.lastError().databaseText());
    return;
  }
  QJsonArray resArr;
  qlonglong originalId = -1;
  int verCount = 0;
  QJsonObject recObj;
  while(query.next()) {
    if (query.value("original_id").toLongLong() != originalId) {
      if (!recObj.isEmpty()) {
        recObj.insert("version_count", verCount);
        resArr.append(recObj);
      }
      recObj = sqlRecordToJson(query.record());
      recObj.insert("has_draft", QJsonValue::fromVariant(false));
      recObj.insert("has_actual", QJsonValue::fromVariant(false));
      originalId = query.value("original_id").toLongLong();
      verCount = 0;
    }
    verCount++;
    if (!query.value("locked").toBool())
      recObj.insert("has_draft", QJsonValue::fromVariant(true));
    if (query.value("actual").toBool())
      recObj.insert("has_actual", QJsonValue::fromVariant(true));
  }
  if (!recObj.isEmpty()) {
    recObj.insert("version_count", verCount);
    resArr.append(recObj);
  }
  response.write(QJsonDocument(resArr).toJson(), true);
}

bool CourseAdminInfoController::hasAccess()
{
  //Доступ для всех кроме учеников и учителей
  UserType userType = (UserType)_context.userType();
  return (userType == SuperUser)
      || (userType == Admin)
      || (userType == Director);
}
