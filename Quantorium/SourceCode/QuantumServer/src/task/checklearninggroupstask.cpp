#include "checklearninggroupstask.h"
#include "../core/appconst.h"
#include <QDebug>

CheckStudentsMinForLearningGroupTask::CheckStudentsMinForLearningGroupTask(QString name) : AbstractTask(name)
{
}


bool CheckStudentsMinForLearningGroupTask::perform(QSqlDatabase &db)
{
  QString sql = SqlDeleteLearningGroupByStudentsMin;
  QSqlQuery query = db.exec(sql);
  qDebug() << "Lesson course deleted by students min condition:" << query.numRowsAffected();
  return true;
}
