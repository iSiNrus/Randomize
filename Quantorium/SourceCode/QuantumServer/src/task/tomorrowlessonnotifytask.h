#ifndef TOMORROWLESSONNOTIFYTASK_H
#define TOMORROWLESSONNOTIFYTASK_H

#include "../core/taskscheduler.h"

class TomorrowLessonNotifyTask : public AbstractTask
{
public:
  TomorrowLessonNotifyTask(QString name);

  // AbstractTask interface
public:
  virtual bool perform(QSqlDatabase &db) override;
};

#endif // TOMORROWLESSONNOTIFYTASK_H
