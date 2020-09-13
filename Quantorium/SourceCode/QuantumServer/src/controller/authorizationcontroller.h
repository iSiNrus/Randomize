#ifndef AUTHORIZATIONCONTROLLER_H
#define AUTHORIZATIONCONTROLLER_H

#include "httprequest.h"
#include "httpresponse.h"
#include "httprequesthandler.h"
#include <QJsonDocument>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QJsonObject>
#include "basejsonsqlcontroller.h"

using namespace stefanfrings;

struct UserLoginData
{
  qlonglong userId;
  bool enabled;
  int userType;
  qlonglong cityId;
  qlonglong regionId;
};

class AuthorizationController : public BaseJsonSqlController
{
  Q_OBJECT
  Q_DISABLE_COPY(AuthorizationController)
public:
  AuthorizationController(Context &con);
  void service(HttpRequest& request, HttpResponse& response);
private:
  UserLoginData checkAuth(QString username, QString password);
  //Обновление времени последнего логина
  void updateLastLoginDate(int userId);
};

#endif // AUTHORIZATIONCONTROLLER_H
