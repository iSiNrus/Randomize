#include "requestmapper.h"
#include "staticfilecontroller.h"
#include "httpsessionstore.h"
#include "controller/baseresttablecontroller.h"
#include "controller/selectqueryjsoncontroller.h"
#include "controller/courseadmininfocontroller.h"
#include "controller/authorizationcontroller.h"
#include "controller/adminonlyresttablecontroller.h"
#include "controller/studentinfocontoller.h"
#include "controller/cityrestrictedtablecontroller.h"
#include "controller/coursetablecontroller.h"
#include "controller/studentactioncontroller.h"
#include "controller/teacheractioncontroller.h"
#include "controller/teacherinfocontroller.h"
#include "controller/adminactioncontroller.h"
#include "controller/usertablecontroller.h"
#include "controller/recreatesubmitresttablecontroller.h"
#include "controller/hierarchytreecontroller.h"
#include "controller/schedulehourscontroller.h"
#include "controller/adminschedulecontroller.h"
#include "controller/batchsubmitresttablecontroller.h"
#include "controller/basejsonreportcontroller.h"
#include "controller/fileattachmentcontroller.h"
#include "controller/notificationcontroller.h"
#include "controller/usermessagecontroller.h"
#include "controller/citytablecontroller.h"
#include "controller/casetablecontroller.h"
#include "controller/actionlogtablecontroller.h"
#include "controller/logfilecontroller.h"
#include "controller/schedulehourscontroller.h"
#include "core/appconst.h"
#include "core/api.h"
#include "core/appsettings.h"
#include "httpsessionmanager.h"
#include <QTimeZone>
#include <QDateTime>
#include "controller/testcontroller.h"

/** Controller for static files */
extern StaticFileController* staticFileController;

extern HttpSessionManager* sessionManager;

RequestMapper::RequestMapper(QObject* parent)
  :HttpRequestHandler(parent) {}

void RequestMapper::service(HttpRequest& request, HttpResponse& response) {
  QByteArray path = request.getPath();
  qDebug("RequestMapper: path=%s", path.data());

  qDebug() << "Connections before:" << QSqlDatabase::connectionNames();

  if (path.startsWith("/login")) {
    Context dummyCon;
    AuthorizationController(dummyCon).service(request, response);
    qDebug() << "Connections after:" << QSqlDatabase::connectionNames();
    return;
  }
  //TODO: Перенести куки авторизации в NetworkAccessManager по умолчанию
  else if (path.startsWith("/forms/")) {
    //Формы загружаются из QML, а там авторизации нет
  }
  else {
    //Проверка сессии (используется с контроллером авторизации, который должен обрабатываться раньше)
#ifndef NO_AUTHORIZATION
    QByteArray sessionId = sessionManager->getSessionId(request, response);
    if (sessionId.isEmpty()) {
      //Unauthorized
      response.setStatus(401);
      response.write(QString("<h1>Unauthorized</h1>").toUtf8());
      qWarning() << "A try of unauthorized access";
      return;
    }
#endif
  }
  HttpSession session = sessionManager->getSession(request, response);
  Context context = Context(session);
//  Context context = Context();

  if (path.startsWith(DELIMITER CTRL_USER_TYPE_TABLE)) {
    BaseRestTableController("sysuser_type", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_USER_CASE)) {
    BaseRestTableController("user_case", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_LEARNING_GROUP_CASE)) {
    BaseRestTableController("learning_group_case", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_USER_LEARNING_GROUP)) {
    BaseRestTableController("user_learning_group", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_USER_LESSON_TABLE)) {
    BaseRestTableController("user_lesson", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_USER_TASK)) {
    BaseRestTableController("user_task", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_USER_ACHIEVEMENT_TABLE)) {
    BaseRestTableController("user_achievement", context).service(request, response);
  }
  else if (path.startsWith("/test")) {
    TestController(context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_USER_TABLE)) {
    UserTableController("sysuser", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_STUDENT_INFO)) {
    StudentInfoContoller(context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_REGION_TABLE)) {
    BaseRestTableController("region", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_TIMING_GRID_UNIT)) {
    RecreateSubmitRestTableController("timing_grid_unit", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_TIMING_GRID_TABLE)) {
    BaseRestTableController("timing_grid", context).service(request, response);
  }
  //Доступные для ученика квантумы
  else if (path.startsWith(DELIMITER CTRL_QUANTUM_BY_USER)) {
    QString sql = "SELECT * FROM quantum q WHERE q.city_id = (SELECT city_id FROM sysuser WHERE id=:user_id)";
    SelectQueryJsonController(sql, context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_QUANTUM_TABLE)) {
    CityRestrictedTableController("quantum", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_LESSON_TABLE)) {
    BaseRestTableController("lesson", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_TASK_TABLE)) {
    BaseRestTableController("task", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_CONTACT_PERSON_TABLE)) {
    BaseRestTableController("contact_person", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_CITY_TABLE)) {
    CityTableController("city", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_STUDENT_ACTION)) {
    StudentActionController(context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_TEACHER_INFO)) {
    TeacherInfoController(context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_TEACHER_ACTION)) {
    TeacherActionController(context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_ADMIN_SCHEDULE)) {
    AdminScheduleController(context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_ADMIN_ACTION)) {
    AdminActionController(context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_REPORTS)) {
    BaseJsonReportController(context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_ATTACHMENTS)) {
    FileAttachmentController(context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_NOTICE_TYPE_TABLE)) {
    BaseRestTableController("notice_type", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_NOTIFICATIONS)) {
    NotificationController(context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_MESSAGES)) {
    UserMessageController(context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_ACHIEVEMENT_CHILDREN_TABLE)) {
    BaseRestTableController("achievement_children", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_ACHIEVEMENT_GROUP_TABLE)) {
    BaseRestTableController("achievement_group", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_CASE_ACHIEVEMENT)) {
    BaseRestTableController("case_achievement", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_CASE_REQUIREED_ACHIEVEMENT)) {
    BaseRestTableController("case_required_achievement", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_CASE_UNIT_TABLE)) {
    BaseRestTableController("case_unit", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_CASE_TABLE)) {
    CaseTableController("case", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_LEARNING_GROUP_HOURS)) {
    ScheduleHoursController("learning_group_hours", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_LEARNING_GROUP_TABLE)) {
    BaseRestTableController("learning_group", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_REQUIRED_ACHIEVEMENT_GROUP)) {
    BaseRestTableController("required_achievement_group", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_ACHIEVEMENT_TREE)) {
    HierarchyTreeController(context, "required_achievement_group", "achievement_group_id", "required_group_id").service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_ACHIEVEMENT_TABLE)) {
    BaseRestTableController("achievement", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER CTRL_ACTION_LOG)) {
    ActionLogTableController("action_log", context).service(request, response);
  }
  else if (path.startsWith(DELIMITER "logs")) {
    LogFileController(context).service(request, response);
  }

  // All other pathes are mapped to the static file controller.
  else {
    staticFileController->service(request, response);
  }
  qDebug() << "Connections after:" << QSqlDatabase::connectionNames();
  qDebug("RequestMapper: finished request");
}
