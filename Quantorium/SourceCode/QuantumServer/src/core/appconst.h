#ifndef APPCONST_H
#define APPCONST_H

#define DEF_LOG_DIR "log"
#define PATH_DELIMITER "/"
#define SQL_LIKE_PLACEHOLDER "%"

#define CONTEXT_USER_ID "user_id"
#define CONTEXT_CITY_ID "city_id"
#define CONTEXT_REGION_ID "region_id"
#define CONTEXT_USER_TYPE_ID "user_type_id"

#define PRM_PATH "path"
#define PRM_HTTP_METHOD "method"
#define PRM_USER_ID "user_id"
#define PRM_QUANTUM_ID "quantum_id"
#define PRM_LESSON_ID "lesson_id"
#define PRM_SCORE "score"
#define PRM_TASK_ID "task_id"
#define PRM_TASK_DATE "task_date"
#define PRM_FINISH_DATE "finish_date"
#define PRM_LESSON_COURSE_ID "lesson_course_id"
#define PRM_COURSE_ID "course_id"
#define PRM_SKILL_ID "skill_id"
#define PRM_SKILL_UNIT_ID "skill_unit_id"
#define PRM_ACHIEVEMENT_ID "achievement_id"
#define PRM_ACHIEVEMENT_GROUP_ID "achievement_group_id"
#define PRM_CITY_ID "city_id"
#define PRM_LESSON_DATE "lesson_date"
#define PRM_NAME "name"
#define PRM_DESCRIPTION "description"
#define PRM_USER_TYPE_ID "sysuser_type_id"

#define COL_KNOWLEDGE_OK "knowledge_ok"
#define COL_AGE_OK "age_ok"
#define COL_COMPLETED "completed"
#define COL_IN_STUDY "in_study"
#define COL_PREVIOUS_SKILL_COMPLETED "previous_skill_completed"
#define COL_PREVIOUS_GROUP_COMPLETED "previous_group_completed"
#define COL_SORT_ID "sort_id"
#define COL_READY "ready"
#define COL_LESSON_SCORE "lesson_score"
#define COL_TASK_SCORE "task_score"
#define COL_QUANTUM_SCORE "quantum_score"
#define COL_COMPLETED_LESSONS "completed_lessons"
#define COL_COMPLETED_COURSES "completed_courses"
#define COL_COMPLETED_TASKS "completed_tasks"
#define COL_SCORE "score"
#define COL_MAX_SCORE "max_score"
#define COL_SKILL_ID "skill_id"
#define COL_SKILL_NAME "skill_name"
#define COL_SKILL_ICON "skill_icon"
#define COL_SKILL_DESCRIPTION "skill_description"
#define COL_SKILL_GROUP_ID "skill_group_id"
#define COL_SKILLS "skills"
#define COL_PARENTS "parents"
#define COL_QUANTUM_ID "quantum_id"
#define COL_QUANTUM "quantum"
#define COL_USER_ID "user_id"
#define COL_COURSE_COUNT "course_count"
#define COL_COURSE_ID "course_id"
#define COL_COMPLETED_COURSES "completed_courses"
#define COL_IN_STUDY_COURSES "in_study_courses"

#define COL_ID "id"
#define COL_LAST_NAME "last_name"
#define COL_SECOND_NAME "second_name"
#define COL_FIRST_NAME "first_name"
#define COL_FULLNAME "fullname"
#define COL_REGISTERED_COUNT "registered_count"
#define COL_NAME "name"
#define COL_START_DATE "start_date"
#define COL_STUDENTS_LIMIT "students_limit"
#define COL_BEGIN_TIME "begin_time"
#define COL_END_TIME "end_time"
#define COL_SCHEDULE "schedule"
#define COL_LESSON_COURSES "lesson_courses"
#define COL_LESSON_COURSE_ID "lesson_course_id"
#define COL_TIMING_GRID_ID "timing_grid_id"
#define COL_CITY_ID "city_id"
#define COL_GIVEN "given"
#define COL_MAX_TASK_SCORE "max_task_score"
#define COL_TOTAL_TASK_COUNT "total_task_count"
#define COL_GIVEN_TASK_SCORE "given_task_score"
#define COL_GIVEN_TASK_COUNT "given_task_count"
#define COL_USER_TASK_COUNT "user_task_count"
#define COL_USER_TASK_SCORE "user_task_score"
#define COL_UNCOMPLETED_TASKS "uncompleted_tasks"
#define COL_TEACHER_ID "teacher_id"
#define COL_NEXT_LESSON "next_lesson"
#define COL_LESSON_COUNT "lesson_count"
#define COL_MAX_LESSON_SCORE "max_lesson_score"
#define COL_DONE_LESSON_COUNT "done_lesson_count"
#define COL_DONE_LESSON_MAX_SCORE "done_lesson_max_score"
#define COL_USER_LESSON_SCORE "user_lesson_score"

//Schedule
#define COL_MONDAY_BITMASK "monday_bitmask"
#define COL_TUESDAY_BITMASK "tuesday_bitmask"
#define COL_WEDNESDAY_BITMASK "wednesday_bitmask"
#define COL_THURSDAY_BITMASK "thursday_bitmask"
#define COL_FRIDAY_BITMASK "friday_bitmask"
#define COL_SATURDAY_BITMASK "saturday_bitmask"
#define COL_SUNDAY_BITMASK "sunday_bitmask"

#define HTTP_NO_ERROR 200
#define HTTP_REQUEST_ERROR 400
#define HTTP_CLIENT_ERROR 401
#define HTTP_NOT_FOUND 404
#define HTTP_FORBIDDEN 403

#define HTTP_GET "GET"
#define HTTP_POST "POST"
#define HTTP_PUT "PUT"
#define HTTP_DELETE "DELETE"

//Типы пользовательских ошибок
#define ERR_NO_ERROR 0
#define ERR_DEFAULT_CUSTOM 100
#define ERR_DATABASE 101
#define ERR_NOT_PERMITED 102
#define ERR_LACK_OF_DATA 103

//Имена ограничений в БД
#define DB_SKILL_GROUP_SKILL_UQ "skill_group_skill_uq"
#define DB_SKILL_GROUP_SKILL_UQ2 "skill_group_skill_sort_uq"


enum UserType {
  SuperUser = 0,
  Director = 1,
  Admin = 2,
  Teacher = 3,
  Student = 4
};

//SQL - запросы
const QString SqlLogin =
    "SELECT u.id, u.city_id, u.sysuser_type_id, u.active, c.region_id FROM sysuser u "
    "LEFT JOIN city c ON c.id=u.city_id "
    "WHERE u.username=:username and u.pswd=:password";

const QString SqlStudentQuantumSummary =
    "select c.quantum_id, sum(s.lesson_score + s.task_score) score, count(distinct uc.course_id) course_count "
    "from user_course uc "
    "left join course c on uc.course_id=c.id "
    "left join course_skill cs on uc.course_id=cs.course_id "
    "left join user_skill s on cs.skill_id=s.skill_id and s.user_id=uc.user_id "
    "where c.quantum_id=%1 and uc.user_id=%2 "
    "group by c.quantum_id";

const QString SqlUserQuantumBySkill =
    "select *, lesson_score+task_score score from user_quantum uq where uq.user_id=%1 "
    "and uq.quantum_id in (select c.quantum_id from course c left join course_skill cs on c.id=cs.course_id "
    "where c.id in (select course_id from user_course where not completed and user_id=%1) and cs.skill_id=%2)";

const QString SqlRegisterLesson = "update lesson set lesson_date=%1, finished=true "
    "where skill_unit_id=%2 and lesson_course_id=%3";


//Кол-во завершенных курсов и суммарный балл
const QString SqlQuantumShortSummary =
        "select c.quantum_id, sum(lesson_score + task_score) total_score, "
        "count(case when uc.completed then 1 else null end) completed_count "
        "from course c "
        "inner join user_course uc on c.id=uc.course_id and uc.user_id=%1 "
        "where c.quantum_id=%2 "
        "group by c.quantum_id";

const QString SqlStudentSummary =
        "select u.id, u.first_name, u.last_name, u.gender, uc_sel.case_count, ut_sel.task_count, ua_sel.achievement_count "
        "from sysuser u left join"
        "(select uc.user_id, count(case when uc.passed then 1 else null end) as case_count from user_case uc group by uc.user_id) uc_sel on u.id=uc_sel.user_id "
        "left join "
        "(select ut.user_id, count(case when ut.completed then 1 else null end) as task_count from user_task ut group by ut.user_id) ut_sel on u.id=ut_sel.user_id "
        "left join "
        "(select ua.user_id, count(ua.achievement_id) as achievement_count from user_achievement ua group by ua.user_id) ua_sel "
        "on u.id=ua_sel.user_id where u.id=%1 group by u.id, uc_sel.case_count, ut_sel.task_count, ua_sel.achievement_count;";

const QString SqlDeleteLearningGroupByStudentsMin =
   "delete from learning_group lg "
   "where lg.finish_date is null and lg.optional "
   "and lg.start_date = current_date+1 "
   "and lg.min_students > (select count('x') from user_learning_group where learning_group_id = lg.id)";

const QString SqlDeleteAllInvalidAchievements =
    "delete from user_achievement where expire_date<'today'::date";

const QString SErrSkillNotFound = "Навык с таким идентификатором не найден";
const QString SErrQuantumNotFound = "Не существует квантума с таким идентификатором";
const QString SInfoUnregisteredFromLessonCourse = "Ученик успешно отписан от группы занятий";
const QString SInfoUnregisteredFromCourse = "Ученик успешно отписан от курса";
const QString SInfoRegisterOnCourse = "Ученик успешно записан на курс";
const QString SInfoEnrollOnLessonCourse = "Ученик успешно записан в группу";

//Database constraint errors
const QString SErrSkillGroupSkillUQ1 = "Навык не может дублироваться в одной группе";
const QString SErrSkillGroupSkillUQ2 = "У навыков группы не может быть совпадения порядкового номера";


#endif // APPCONST_H
