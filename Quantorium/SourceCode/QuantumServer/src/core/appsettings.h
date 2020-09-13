#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QSettings>

//Раскомментировать для режима без авторизации
//#define NO_AUTHORIZATION

#define SECTION_DATABASE "database"
#define PRM_TYPE "type"
#define PRM_HOSTNAME "host"
#define PRM_PORT "port"
#define PRM_DATABASE_NAME "name"
#define PRM_USERNAME "username"
#define PRM_PASSWORD "password"

#define SECTION_GENERAL "common"

#define SECTION_LOGGER "logger"
#define PRM_LOG_FILE "logFile"
#define PRM_LOG_CRITICAL "logCritical"
#define PRM_LOG_DEBUG "logDebug"
#define PRM_LOG_FATAL "logFatal"
#define PRM_LOG_WARNING "logWarning"
#define PRM_LOG_FILTER "filter"
#define PRM_LOG_SQL "logSql"

class AppSettings
{
public:
    AppSettings();
    static void setSettingsFile(QString filePath);
    static bool contains(QString section, QString name);
    static QVariant val(QString section, QString param, QVariant defValue = QVariant());
    static QVariant val(QString key, QVariant defValue = QVariant());
    static void setVal(QString key, QVariant val);
    static void setVal(QString section, QString param, QVariant val);
    static QString strVal(QString section, QString name, QVariant defValue = QVariant());
    static int intVal(QString section, QString name, QVariant defValue = QVariant());
    static bool boolVal(QString section, QString name, QVariant defValue = QVariant());
    static void remove(const QString &key);
    static QString applicationPath();
    static QString applicationName();
    static QString settingsPath();
    static void sync();
private:
    static QSettings* _settings;
    static void init();
    static QString _filePath;
};

#endif // APPSETTINGS_H
