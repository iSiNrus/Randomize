#include "removeinvalidachievementstask.h"
#include "../core/appconst.h"
#include <QDebug>

RemoveInvalidAchievementsTask::RemoveInvalidAchievementsTask(QString name) : AbstractTask(name)
{

}


bool RemoveInvalidAchievementsTask::perform(QSqlDatabase &db)
{
  QString sql = SqlDeleteAllInvalidAchievements;
  QSqlQuery query = db.exec(sql);
  qDebug() << "Invalid student achievements deleted:" << query.numRowsAffected();
  return true;
}
