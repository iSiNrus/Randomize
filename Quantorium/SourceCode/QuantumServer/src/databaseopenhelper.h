#ifndef DATABASEOPENHELPER_H
#define DATABASEOPENHELPER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include "core/appsettings.h"

//Параметры соединения с СУБД по умолчанию
#define SQL_DRIVER_NAME "QPSQL"      //Имя драйвера
#define DB_PORT 5432                 //Порт СУБД
#define DB_NAME "QuantumDB"          //Имя БД
#define DB_HOSTNAME "localhost"      //Хост СУБД
#define DB_USERNAME "postgres"       //Имя пользователя СУБД
#define DB_PASSWORD "softerium"      //Пароль

const QString SErrConnectionFailed = "DatabaseOpenHelper - Ошибка соединения с СУБД: %1";

class DatabaseOpenHelper
{
public:
  static bool init();
  static QSqlDatabase newConnection(QString name = "");
  static void removeConnection(QSqlDatabase &db);
private:
  DatabaseOpenHelper();
  static QString mDbName;
  static QString mHostname;
  static QString mUsername;
  static QString mPassword;
  static int mPort;
};

#endif // DATABASEOPENHELPER_H
