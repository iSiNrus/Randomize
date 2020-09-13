#include "authorizationcontroller.h"
#include <QCryptographicHash>
#include "../httpsessionmanager.h"
#include "../core/appconst.h"
#include "../utils/strutils.h"
#include "../utils/qsqlqueryhelper.h"
#include "../core/notifications.h"
#include "../core/usernotifier.h"
#include "../databaseopenhelper.h"
#include "../core/api.h"
#include <QDate>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>

extern HttpSessionManager* sessionManager;
extern UserNotifier* userNotifier;

AuthorizationController::AuthorizationController(Context &con) : BaseJsonSqlController(con)
{
}

void AuthorizationController::service(HttpRequest &request, HttpResponse &response)
{
  qDebug() << "Authorization controller";
  BaseJsonSqlController::service(request, response);
  if (response.getStatusCode() != HTTP_NO_ERROR)
    return;

  QString username = QString::fromUtf8(request.getParameter("username"));
  QByteArray pswd = request.getParameter("pswd");
  QByteArray passHash = QCryptographicHash::hash(
         pswd + username.toUtf8(), QCryptographicHash::Md5).toHex();
  qDebug() << "username:" << username;
  qDebug() << "password hash:" << passHash;
  UserLoginData loginData = checkAuth(username, passHash);
  if (loginData.userId >= 0) {
    if (loginData.enabled) {
      HttpSession session = sessionManager->createSession(request, response, loginData.userId);
      session.set(CONTEXT_USER_ID, loginData.userId);
      session.set(CONTEXT_USER_TYPE_ID, loginData.userType);
      session.set(CONTEXT_CITY_ID, loginData.cityId);
      session.set(CONTEXT_REGION_ID, loginData.regionId);
      QJsonObject jObj;
      jObj.insert(CONTEXT_USER_ID, QJsonValue::fromVariant(loginData.userId));
      jObj.insert(CONTEXT_USER_TYPE_ID, QJsonValue::fromVariant(loginData.userType));
      jObj.insert(CONTEXT_CITY_ID, QJsonValue::fromVariant(loginData.cityId));
      jObj.insert(CONTEXT_REGION_ID, QJsonValue::fromVariant(loginData.regionId));
      response.write(QJsonDocument(jObj).toJson());
      updateLastLoginDate(loginData.userId);
    }
    else {
      response.setStatus(HTTP_CLIENT_ERROR);
      writeError(response, "User is currently disabled", HTTP_CLIENT_ERROR);
    }
  }
  else {
    response.setStatus(HTTP_CLIENT_ERROR);
    writeError(response, "Incorrect username or password", HTTP_CLIENT_ERROR);
  }
}

UserLoginData AuthorizationController::checkAuth(QString username, QString password)
{
  qWarning() << "Authorization:" << username << password;
  UserLoginData userLoginData;
  userLoginData.userId = -1;
  {
    QSqlDatabase db = _context.db();
    QSqlQuery query(db);
    query.prepare(SqlLogin);
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    if (query.exec() && query.next()) {
      userLoginData.userId = query.value("id").toLongLong();
      userLoginData.cityId = query.value("city_id").toLongLong();
      userLoginData.userType = query.value("sysuser_type_id").toLongLong();
      userLoginData.regionId = query.value("region_id").toLongLong();
      userLoginData.enabled = query.value("active").toBool();
    }
    else {
      if (query.lastError().type() != QSqlError::NoError) {
        qWarning() << query.lastError().databaseText();
      }
      userNotifier->notify(db, MSG_LOGIN_FAILED, 1);
    }
  }
  return userLoginData;
}

void AuthorizationController::updateLastLoginDate(int userId)
{
  QString sql = "update sysuser set last_login='%1' where id=%2";
  QSqlQueryHelper::execSql(sql.arg(QDate::currentDate().toString(DATE_FORMAT)).arg(userId), _context.uid());
}
