#ifndef CHECKLESSONCOURSETASK_H
#define CHECKLESSONCOURSETASK_H

#include "../core/taskscheduler.h"

class CheckStudentsMinForLearningGroupTask : public AbstractTask
{
public:
  CheckStudentsMinForLearningGroupTask(QString name);

  // AbstractTask interface
public:
  virtual bool perform(QSqlDatabase &db) override;
};

#endif // CHECKLESSONCOURSETASK_H
