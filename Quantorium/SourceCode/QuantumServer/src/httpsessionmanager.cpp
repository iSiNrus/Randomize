#include "httpsessionmanager.h"
#include <QDebug>
#include "core/appconst.h"
#include "utils/strutils.h"
#include "databaseopenhelper.h"

HttpSessionManager::HttpSessionManager(QSettings* settings, QObject* parent)
  : QObject(parent)
{
  sessionStore = new HttpSessionStore(settings);
}

HttpSessionManager::~HttpSessionManager()
{
  delete sessionStore;
}

HttpSession HttpSessionManager::getSession(HttpRequest &request, HttpResponse &response)
{  
  return sessionStore->getSession(request, response, false);
}

HttpSession HttpSessionManager::createSession(HttpRequest &request, HttpResponse &response, qlonglong sysuserId)
{
  //Только одна сессия на пользователя
  if (userSessionMap.contains(sysuserId)) {
    HttpSession oldSession = sessionStore->getSession(userSessionMap.value(sysuserId));
    qDebug() << "remove old session for user" << sysuserId;
    sessionStore->removeSession(oldSession);
  }
  HttpSession session = sessionStore->getSession(request, response, true);
  userSessionMap.insert(sysuserId, session.getId());
  return session;
}

void HttpSessionManager::removeSession(QByteArray sessionId)
{
  qlonglong userId = userSessionMap.key(sessionId, -1);
  HttpSession session = sessionStore->getSession(sessionId);
  userSessionMap.remove(userId);
  sessionStore->removeSession(session);
  qDebug() << "Connection with User" << userId << "was lost";
}

QByteArray HttpSessionManager::getSessionIdByUser(qlonglong userId)
{
  return userSessionMap.value(userId, QByteArray());
}

QByteArray HttpSessionManager::getSessionId(HttpRequest &request, HttpResponse &response)
{
  return sessionStore->getSessionId(request, response);
}


Context::Context()
{
  qDebug() << "Context void";
  initDb();
}

Context::Context(HttpSession &session) : _session(session)
{  
  qDebug() << "Context session";
  initDb();
}

Context::~Context()
{
  QSqlDatabase::removeDatabase(_uid);
}

void Context::initDb()
{
  qDebug() << "InitDb()";
  _uid = StrUtils::uuid();

  DatabaseOpenHelper::newConnection(_uid);
}

QString Context::uid() const
{
  return _uid;
}

qlonglong Context::sysuserId() const
{
  return _session.isNull() ? 0 : _session.get(CONTEXT_USER_ID).toLongLong();
}

int Context::userType() const
{
  return _session.isNull() ? -1 : _session.get(CONTEXT_USER_TYPE_ID).toInt();
}

qlonglong Context::city_id() const
{
  return _session.isNull() ? -1 : _session.get(CONTEXT_CITY_ID).toInt();
}

int Context::region_id() const
{
  return _session.isNull() ? -1 : _session.get(CONTEXT_REGION_ID).toInt();
}

QSqlDatabase Context::db()
{
  qDebug() << "Get DB" << uid();
  return QSqlDatabase::database(uid());
}
