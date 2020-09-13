#ifndef HTTPSESSIONMANAGER_H
#define HTTPSESSIONMANAGER_H

#include "httpsessionstore.h"
#include <QUuid>
#include <QSqlDatabase>

using namespace stefanfrings;

class Context
{
public:
  Context();
  Context(HttpSession &session);
  ~Context();
  void initDb();
  QString uid() const;
  qlonglong sysuserId() const;
  int userType() const;
  qlonglong city_id() const;
  int region_id() const;
  QSqlDatabase db();
private:
  HttpSession _session;
  QString _uid = "";
};

class HttpSessionManager : public QObject
{
  Q_OBJECT
public:
  HttpSessionManager(QSettings* settings, QObject* parent=NULL);
  ~HttpSessionManager();
  HttpSession getSession(HttpRequest& request, HttpResponse& response);
  HttpSession createSession(HttpRequest& request, HttpResponse& response, qlonglong sysuserId);
  void removeSession(QByteArray sessionId);
  QByteArray getSessionIdByUser(qlonglong userId);

  QByteArray getSessionId(HttpRequest& request, HttpResponse& response);

private:
  HttpSessionStore* sessionStore;
  //Пары пользователь-ID сессия
  QMap<qlonglong, QByteArray> userSessionMap;
};

#endif // HTTPSESSIONMANAGER_H
