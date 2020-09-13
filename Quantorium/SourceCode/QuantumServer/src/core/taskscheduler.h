#ifndef TASKSCHEDULER_H
#define TASKSCHEDULER_H

#include <QObject>
#include <QThreadPool>
#include <QTimer>
#include <QTime>
#include <QSqlDatabase>
#include <QRunnable>
#include "../databaseopenhelper.h"

class AbstractTask : public QRunnable
{
public:
  enum Type {
    ByInterval,
    ByTime
  };
  AbstractTask(QString name);
  void initialize();
  virtual bool perform(QSqlDatabase& db) = 0;
  QString name();

  void setInterval(int secs);
  void setTime(QTime time);

  bool shouldPerform();
private:
  QString _name;
  Type _type;
  QTime _startTime;
  QDateTime _lastStartTime;
  // QRunnable interface
public:
  virtual void run() override;
};

class TestTask : public AbstractTask
{
public:
  TestTask(QString name);

  // AbstractTask interface
public:
  virtual bool perform(QSqlDatabase &db) override;
};

class TaskScheduler : public QObject
{
  Q_OBJECT
public:
  explicit TaskScheduler(QObject *parent = 0);
  void registerTask(AbstractTask* task);
  void activate(int refreshInterval);

  bool loadTasks();
  void runTask(AbstractTask* task);
signals:

public slots:
  void onTimerTriggered();
private:
  QTimer* _timer;
  QList<AbstractTask*> _taskList;
  QThreadPool* _threadPool;
};

#endif // TASKSCHEDULER_H
