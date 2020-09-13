/**
  @file
  @author Stefan Frings
*/

#include "static.h"
#include "startup.h"
#include "filelogger.h"
#include "requestmapper.h"
#include "staticfilecontroller.h"
#include "httpsessionmanager.h"
#include "databaseopenhelper.h"
#include "core/qdatabaseupdater.h"
#include "core/slogger.h"
#include "core/appconst.h"
#include "core/taskscheduler.h"
#include "core/studiesdictionary.h"
#include "core/usernotifier.h"
#include "utils/qfileutils.h"
#include "utils/qsqlqueryhelper.h"
#include "controller/basejsonactioncontroller.h"
#include "task/checklearninggroupstask.h"
#include "task/tomorrowlessonnotifytask.h"
#include "task/removeinvalidachievementstask.h"
#include <QDir>
#include <QFile>

/** Name of this application */
#define APPNAME "QuantumServer"

/** Publisher of this application */
#define ORGANISATION "Curious Crow"

/** Short description of the Windows service */
#define DESCRIPTION "Server for self-served carwash system."

HttpSessionManager* sessionManager;
StudyStructureCache* studyStructureCache;
StaticFileController* staticFileController;
TaskScheduler* taskScheduler;
UserNotifier* userNotifier;


/** Logger class */
SLogger* logger;

/** Search the configuration file */
QString searchConfigFile()
{
  QString binDir=QCoreApplication::applicationDirPath();
  QString appName=QCoreApplication::applicationName();
  QString fileName(appName.toLower()+".ini");

  QStringList searchList;
  searchList.append(binDir);
  searchList.append(binDir+"/etc");
  searchList.append(binDir+"/../etc");
  searchList.append(binDir+"/../../etc"); // for development without shadow build
  searchList.append(binDir+"/../"+appName+"/etc"); // for development with shadow build
  searchList.append(binDir+"/../../"+appName+"/etc"); // for development with shadow build
  searchList.append(binDir+"/../../../"+appName+"/etc"); // for development with shadow build
  searchList.append(binDir+"/../../../../"+appName+"/etc"); // for development with shadow build
  searchList.append(binDir+"/../../../../../"+appName+"/etc"); // for development with shadow build
  searchList.append(QDir::rootPath()+"etc/opt");
  searchList.append(QDir::rootPath()+"etc");

  foreach (QString dir, searchList)
  {
    QFile file(dir+"/"+fileName);
    if (file.exists())
    {
      // found
      fileName=QDir(file.fileName()).canonicalPath();
      qDebug("Using config file %s",qPrintable(fileName));
      return fileName;
    }
  }

  // not found
  foreach (QString dir, searchList)
  {
    qWarning("%s/%s not found",qPrintable(dir),qPrintable(fileName));
  }
  qFatal("Cannot find config file %s",qPrintable(fileName));
  return 0;
}

void Startup::start() {
  // Initialize the core application
  QCoreApplication* app = application();
  app->setApplicationName(APPNAME);
  app->setOrganizationName(ORGANISATION);

  // Find the configuration file
  QString configFileName = searchConfigFile();
  AppSettings::setSettingsFile(configFileName);
  // Configure logging
  logger = new SLogger();
  logger->setLogInfo(1);
  logger->setLogDebug(AppSettings::boolVal(SECTION_LOGGER, PRM_LOG_DEBUG, false));
  logger->setLogWarning(AppSettings::boolVal(SECTION_LOGGER, PRM_LOG_WARNING, true));
  logger->setLogCritical(AppSettings::boolVal(SECTION_LOGGER, PRM_LOG_CRITICAL, true));
  logger->setLogFatal(AppSettings::boolVal(SECTION_LOGGER, PRM_LOG_FATAL, true));
  logger->setFilter(AppSettings::strVal(SECTION_LOGGER, PRM_LOG_FILTER, ""));
  QString logPath = AppSettings::applicationPath() + DEF_LOG_DIR + PATH_DELIMITER + AppSettings::applicationName() + ".log";
  if (AppSettings::contains(SECTION_LOGGER, PRM_LOG_FILE)) {
    logPath = AppSettings::strVal(SECTION_LOGGER, PRM_LOG_FILE);
  }
  QFileUtils::forcePath(QFileUtils::extractDirPath(logPath));
  logger->setLogToFile(logPath);
  logger->start();

  //Проверка настроек соединения с СУБД
  DatabaseOpenHelper::init();

  QDatabaseUpdater databaseUpdater;
  {
    QSqlDatabase updaterConnection = DatabaseOpenHelper::newConnection("updater_connection");
    if (!databaseUpdater.updateDatabase(updaterConnection, "sys_params", "db_version")) {
      qDebug() << "Database error";
      app->exit();
    }
  }
  QSqlDatabase::removeDatabase("updater_connection");

  //Установка логирования SQL-запросов из настроек
  QSqlQueryHelper::setLogging(AppSettings::boolVal(SECTION_LOGGER, PRM_LOG_SQL, false));
  BaseJsonSqlController::setLogSql(AppSettings::boolVal(SECTION_LOGGER, PRM_LOG_SQL, false));

  // Log the library version
  qDebug("QtWebAppLib has version %s", getQtWebAppLibVersion());

  // Configure session store
  QSettings* sessionSettings = new QSettings(configFileName, QSettings::IniFormat, app);
  sessionSettings->beginGroup("sessions");
  sessionManager = new HttpSessionManager(sessionSettings, app);

  studyStructureCache = new StudyStructureCache(app);

  // Configure static file controller
  QSettings* fileSettings = new QSettings(configFileName, QSettings::IniFormat, app);
  fileSettings->beginGroup("docroot");
  staticFileController = new StaticFileController(fileSettings, app);

  //Объект планировщика
  taskScheduler = new TaskScheduler(app);

  //Задание по удалению групп с недостаточным кол-вом записавшихся
  AbstractTask* deleteLearningGroupByStudentsMinTask = new CheckStudentsMinForLearningGroupTask("CheckLessonCourseByStudentsMin");
  deleteLearningGroupByStudentsMinTask->setTime(QTime(0, 15));
  taskScheduler->registerTask(deleteLearningGroupByStudentsMinTask);

  //Задание по удалению просроченных достижений учеников
  AbstractTask* deleteInvalidUserAchievements = new RemoveInvalidAchievementsTask("RemoveInvalidUserAchievements");
  deleteInvalidUserAchievements->setTime(QTime(0, 0));
  taskScheduler->registerTask(deleteInvalidUserAchievements);

  //Задание для рассылки напоминаний о занятии на следующий день
  AbstractTask* tomorrowLessonNotifyTask = new TomorrowLessonNotifyTask("TomorrowLessonNotification");
  tomorrowLessonNotifyTask->setTime(QTime(15, 0));
  taskScheduler->registerTask(tomorrowLessonNotifyTask);
  //Запуск планировщика
  taskScheduler->activate(2000);

  //Генератор оповещений
  userNotifier = new UserNotifier(app);
  {
    QSqlDatabase notifierConn = DatabaseOpenHelper::newConnection("notifierConn");
    userNotifier->loadTypes(notifierConn);
  }
  QSqlDatabase::removeDatabase("notifierConn");

  // Configure and start the TCP listener
  qDebug("ServiceHelper: Starting service");
  QSettings* listenerSettings = new QSettings(configFileName, QSettings::IniFormat, app);
  listenerSettings->beginGroup("listener");
  listener = new HttpListener(listenerSettings, new RequestMapper(app), app);

  qWarning("ServiceHelper: Service has started");
}

void Startup::stop() {
  // Note that the stop method is not called when you terminate the application abnormally
  // by pressing Ctrl-C or when you kill it by the task manager of your operating system.

  // Deleting the listener here is optionally because QCoreApplication does it already.
  // However, QCoreApplication closes the logger at first, so we would not see the shutdown
  // debug messages, without the following line of code:

  delete listener;
  delete sessionManager;
  delete staticFileController;

  qWarning("ServiceHelper: Service has been stopped");
}


Startup::Startup(int argc, char *argv[])
  : QtService<QCoreApplication>(argc, argv, APPNAME)
{
  setServiceDescription(DESCRIPTION);
  setStartupType(QtServiceController::AutoStartup);

}
