#include "taskscheduler.h"
#include "../utils/qsqlqueryhelper.h"
#include <QDebug>

#define CONN_LOAD_TASKS "load_tasks"

TaskScheduler::TaskScheduler(QObject *parent) : QObject(parent)
{
  _threadPool = new QThreadPool(this);
  _timer = new QTimer(this);
  connect(_timer, SIGNAL(timeout()), this, SLOT(onTimerTriggered()));
}

void TaskScheduler::registerTask(AbstractTask *task)
{  
  _taskList.append(task);
  //TODO: Временно закомментировано для тестов
//  task->initialize();
  qDebug() << "Task" << task->name() << "registered";
}

void TaskScheduler::activate(int refreshInterval)
{
  qDebug() << "Task scheduler activated";
  _timer->setSingleShot(false);
  _timer->start(refreshInterval);
}

bool TaskScheduler::loadTasks()
{
  {
    QSqlDatabase db = DatabaseOpenHelper::newConnection(CONN_LOAD_TASKS);
    db.exec("select * from task");
  }
  QSqlDatabase::removeDatabase(CONN_LOAD_TASKS);
  return true;
}

void TaskScheduler::runTask(AbstractTask *task)
{
  _threadPool->start(task);
}

void TaskScheduler::onTimerTriggered()
{
//  qDebug() << "Task scheduler: Check for tasks";
  foreach (AbstractTask* task, _taskList) {
    if (task->shouldPerform())
      runTask(task);
  }
}

AbstractTask::AbstractTask(QString name) : QRunnable()
{
  _name = name;
  setAutoDelete(false);
}

void AbstractTask::initialize()
{
  _lastStartTime = QDateTime::currentDateTime();
}

QString AbstractTask::name()
{
  return _name;
}

void AbstractTask::setInterval(int secs)
{
  _type = ByInterval;
  _startTime = QTime::fromMSecsSinceStartOfDay(secs * 1000);
  _lastStartTime = QDateTime::currentDateTime();
}

void AbstractTask::setTime(QTime time)
{
  _type = ByTime;
  _startTime = time;
  _lastStartTime = QDateTime::currentDateTime();
}

bool AbstractTask::shouldPerform()
{  
  if (_type == ByInterval) {
    int interval = _startTime.msecsSinceStartOfDay();
    int msecSinceLastStart = _lastStartTime.msecsTo(QDateTime::currentDateTime());
    return msecSinceLastStart > interval;
  }
  else if (_type == ByTime) {
    return (_lastStartTime.date() < QDate::currentDate()) && (QTime::currentTime() > _startTime);
  }
  return false;
}

void AbstractTask::run()
{
  qDebug() << "Current thread id:" << QThread::currentThreadId();
  qDebug() << "Task" << name() << "started";
  QString connName;
  {
    QSqlDatabase db = DatabaseOpenHelper::newConnection();
    connName = db.connectionName();
    db.transaction();

    bool taskResult = perform(db);
    if (taskResult) {
      qDebug() << "Task" << name() << "completed";
      bool commitResult = db.commit();
      qDebug() << "Commit" << commitResult;
    }
    else {
      qWarning() << "Task" << name() << "failed:" << db.lastError().databaseText();
      bool rollbackResult = db.rollback();
      qDebug() << "Rollback" << rollbackResult;
    }
  }
  QSqlDatabase::removeDatabase(connName);
  initialize();

  qDebug() << "Task" << name() << "finished";
}

TestTask::TestTask(QString name) : AbstractTask(name)
{

}

bool TestTask::perform(QSqlDatabase &db)
{
  QSqlQuery query = db.exec("select count('x') from sysuser");
  query.next();
  int userCount = query.value(0).toInt();
  qDebug() << "Users found:" << userCount;
}
