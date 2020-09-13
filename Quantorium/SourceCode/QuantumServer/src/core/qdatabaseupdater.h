#ifndef QDATABASEUPDATER_H
#define QDATABASEUPDATER_H

#include <QObject>
#include <QSqlDatabase>
#include <QStringList>

#define SQL_SET_VERSION "update %1 set %2=%3"
#define SQL_GET_VERSION "select %1 from %2"
#define FILE_LOG_UPDATER "DbUpdate.log"

const QString SErrDatabaseConnection = "QDatabaseUpdater - Отсутствует соединение с СУБД: %1";
const QString SErrCantUpdateToVer = "Ошибка при обновлении до версии %1";
const QString SMsgDbUpdatedToVersion = "База данных обновлена до версии %1";
const QString SMsgCurrentDbVersion = "Текущая версия БД: %1";
const QString SMsgUpdateIsStarting = "Запуск задания обновления БД";
const QString SMsgUpdateCompleted = "База данных успешно обновлена";

/**
 * @brief Класс реализующий автоматическое обновление базы данных
 */
class QDatabaseUpdater : public QObject
{
  Q_OBJECT
public:  
  //Запуск обновления БД
  static bool updateDatabase(QSqlDatabase &db, QString versionTable, QString versionField);
private:
  //Получает текущую версию БД
  static int getDbVersion(QSqlDatabase &db, QString versionTable, QString versionField);
  //Обновляет значение версии в БД
  static void setDbVersion(QSqlDatabase &db, QString versionTable, QString versionField, int version);
  //Формирование списка запросов обновления БД
  static QStringList initializeUpdateScript();
  //Вывод сообщения в лог
  static void log(QString msg);
signals:

public slots:

};

#endif // QDATABASEUPDATER_H
