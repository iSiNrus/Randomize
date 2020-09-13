#include "tomorrowlessonnotifytask.h"
#include "../core/usernotifier.h"
#include "../core/notifications.h"

extern UserNotifier* userNotifier;

TomorrowLessonNotifyTask::TomorrowLessonNotifyTask(QString name) : AbstractTask(name)
{
}


bool TomorrowLessonNotifyTask::perform(QSqlDatabase &db)
{
  QString sql = "select lc.id lesson_course_id, lc.name lesson_course_name, us.user_id student_id, su.last_name || ' ' || su.first_name || ' ' || su.second_name teacher_name "
  "from lesson_course lc "
  "left join user_skill us on lc.id=us.lesson_course_id "
  "left join sysuser su on lc.teacher_id=su.id "
  "where lc.start_date <= current_date + 1 and not lc.finished "
  "and exists(select 'x' from lesson_course_hours where lesson_course_id=lc.id and day_of_week=extract(dow from current_date+1))";

  QSqlQuery query = db.exec(sql);
  QVariantMap params;
  while (query.next()) {
    qlonglong studentId = query.value("student_id").toLongLong();
    params.insert("lesson_course_name", query.value("lesson_course_name").toString());
    params.insert("teacher_name", query.value("teacher_name").toString());
    userNotifier->notify(db, MSG_TOMORROW_LESSON_NOTIFICATION, studentId, params);
  }
  return true;
}
