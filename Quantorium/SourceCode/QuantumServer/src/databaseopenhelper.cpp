#include "databaseopenhelper.h"
#include <QFileInfo>
#include <QSqlError>
#include <QDebug>
#include "utils/strutils.h"

#define TEST_CONNECTION_NAME "test_connection"

QString DatabaseOpenHelper::mDbName = "";
QString DatabaseOpenHelper::mHostname = "";
int DatabaseOpenHelper::mPort = 0;
QString DatabaseOpenHelper::mUsername = "";
QString DatabaseOpenHelper::mPassword = "";

bool DatabaseOpenHelper::init()
{
  mDbName = AppSettings::strVal(SECTION_DATABASE, PRM_DATABASE_NAME, DB_NAME);
  mHostname = AppSettings::strVal(SECTION_DATABASE, PRM_HOSTNAME, DB_HOSTNAME);
  mPort = AppSettings::intVal(SECTION_DATABASE, PRM_PORT, DB_PORT);
  mUsername = AppSettings::strVal(SECTION_DATABASE, PRM_USERNAME, DB_USERNAME);
  mPassword = AppSettings::strVal(SECTION_DATABASE, PRM_PASSWORD, DB_PASSWORD);
  {
    QSqlDatabase testConn = newConnection(TEST_CONNECTION_NAME);
    if (!testConn.open()) {
      qFatal(qUtf8Printable(SErrConnectionFailed.arg(testConn.lastError().text())));
      return false;
    }
    qDebug() << "Database connection tested: Ok";
  }
  QSqlDatabase::removeDatabase(TEST_CONNECTION_NAME);
  return true;

}

QSqlDatabase DatabaseOpenHelper::newConnection(QString name)
{
  if (name.isEmpty())
    name = StrUtils::uuid();
  QSqlDatabase db = QSqlDatabase::addDatabase(SQL_DRIVER_NAME, name);
  db.setHostName(mHostname);
  db.setPort(mPort);
  db.setUserName(mUsername);
  db.setPassword(mPassword);
  db.setDatabaseName(mDbName);

  if (!db.open()) {
    qFatal(qUtf8Printable(SErrConnectionFailed.arg(db.lastError().text())));
  }

  return db;
}

void DatabaseOpenHelper::removeConnection(QSqlDatabase &db)
{
  QSqlDatabase::removeDatabase(db.connectionName());
}

DatabaseOpenHelper::DatabaseOpenHelper()
{
}


