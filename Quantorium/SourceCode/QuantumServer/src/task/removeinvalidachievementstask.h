#ifndef REMOVEINVALIDACHIEVEMENTSTASK_H
#define REMOVEINVALIDACHIEVEMENTSTASK_H

#include "../core/taskscheduler.h"

class RemoveInvalidAchievementsTask : public AbstractTask
{
public:
  RemoveInvalidAchievementsTask(QString name);

  // AbstractTask interface
public:
  virtual bool perform(QSqlDatabase &db) override;
};

#endif // REMOVEINVALIDACHIEVEMENTSTASK_H
