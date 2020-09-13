#include "studiesdictionary.h"
#include <QVariant>
#include <QDebug>
#include <QSqlError>
#include <QTime>
#include "../databaseopenhelper.h"

#define COL_ID "id"
#define COL_NAME "name"
#define COL_ICON "icon"
#define COL_ACTUAL "actual"
#define COL_LOCKED "locked"
#define COL_VERSION "version"
#define COL_ORIGINAL_ID "original_id"
#define COL_MIN_PERCENTAGE "min_percentage"
#define COL_QUANTUM_ID "quantum_id"
#define COL_MIN_AGE "min_age"
#define COL_MAX_SCORE "max_score"
#define COL_TOTAL_MAX_SCORE "total_max_score"
#define COL_CITY "city"
#define COL_CITY_ID "city_id"
#define COL_TIMING_GRID_ID "timing_grid_id"
#define COL_DESCRIPTION "description"
#define COL_COURSE_ID "course_id"
#define COL_SORT_ID "sort_id"
#define COL_SKILL_UNIT_ID "skill_unit_id"
#define COL_SKILL_ID "skill_id"
#define COL_SKILL_GROUP_ID "skill_group_id"
#define COL_UNIT_COUNT "unit_count"
#define COL_TASK_COUNT "task_count"
#define COL_COURSE_COUNT "course_count"

#define COL_SKILLS "skills"
#define COL_REQUIRED "required"

#define CON_REFRESH "StudiesDict"
#define CON_GET_VERSION "GetVersionConn"


StudiesDictionary::StudiesDictionary(qlonglong cityId, QObject *parent) : QObject(parent)
{
  _cityId = cityId;
}

bool StudiesDictionary::refreshData()
{
  qDebug() << "Refresh studies cache";
  //TODO: Защитить мьютексом
  clear();

  {
    QSqlDatabase db = DatabaseOpenHelper::newConnection(CON_REFRESH);
    QSqlQuery query = QSqlQuery(db);
    QString sql = "SELECT * FROM task "
                  "WHERE skill_unit_id IN (select id from skill_unit where skill_id in (select id from skill where city_id=%1)) "
                  "ORDER BY sort_id";
    if (!execSql(query, sql.arg(QString::number(_cityId))))
      return false;
    while(query.next()) {
      Task* task = new Task(query.record());
      _taskMap.insert(task->id(), task);
    }

    sql = "SELECT * FROM skill_unit "
          "WHERE skill_id IN (select id from skill where city_id=%1) "
          "ORDER BY sort_id";
    if (!execSql(query, sql.arg(QString::number(_cityId))))
      return false;
    while(query.next()) {
      SkillUnit* skillUnit = new SkillUnit(query.record());
      _skillUnitMap.insert(skillUnit->id(), skillUnit);
    }
    //Skill
    sql = "SELECT * FROM skill "
          "WHERE city_id=%1";
    if (!execSql(query, sql.arg(QString::number(_cityId))))
      return false;
    while(query.next()) {
      Skill* skill = new Skill(query.record());
      _skillMap.insert(skill->id(), skill);
    }
    //SkillGroup
    sql = "SELECT * FROM skill_group "
          "WHERE city_id=%1";
    if (!execSql(query, sql.arg(QString::number(_cityId))))
      return false;
    while(query.next()) {
      SkillGroup* skillGroup = new SkillGroup(query.record());
      _skillGroupMap.insert(skillGroup->id(), skillGroup);
    }
    //Course
    sql = "SELECT * FROM course "
          "WHERE quantum_id in (select id from quantum where city_id=%1)";
    if (!execSql(query, sql.arg(QString::number(_cityId))))
      return false;
    while(query.next()) {
      Course* course = new Course(query.record());
      _courseMap.insert(course->id(), course);
    }
    //Quantum
    sql = "SELECT q.*, (SELECT name FROM city WHERE id=q.city_id) city FROM quantum q "
          "WHERE city_id=%1";
    if (!execSql(query, sql.arg(QString::number(_cityId))))
      return false;
    while(query.next()) {
      Quantum* quantum = new Quantum(query.record());
      _quantumMap.insert(quantum->id(), quantum);
    }
    sql =   "SELECT csg.course_id, csg.skill_group_id, rsg.required_skill_group_id "
            "FROM course_skill_group csg "
            "LEFT JOIN required_skill_group rsg on csg.course_id=rsg.course_id and csg.skill_group_id=rsg.skill_group_id "
            "WHERE csg.city_id=%1 "
            "ORDER BY csg.course_id, csg.skill_group_id";
    if (!execSql(query, sql.arg(QString::number(_cityId))))
      return false;
    qlonglong lastCourseId = 0;
    qlonglong lastSkillGroupId = 0;
    while(query.next()) {
      qlonglong courseId = query.value("course_id").toLongLong();
      qlonglong skillGroupId = query.value("skill_group_id").toLongLong();
      qlonglong requiredGroupId = query.value("required_skill_group_id").toLongLong();
      if (courseId != lastCourseId)
        lastSkillGroupId = 0;
      if (skillGroupId != lastSkillGroupId) {
        //Добавление группы навыков в курс
        _courseMap.value(courseId)->appendSkillGroup(_skillGroupMap.value(skillGroupId));
      }
      if (requiredGroupId != 0) {
        //Добавление зависимости группы в группу с ключом courseId
        _skillGroupMap.value(skillGroupId)->appendRequired(courseId, requiredGroupId);
      }
      lastCourseId = courseId;
      lastSkillGroupId = skillGroupId;
    }

    sql = "SELECT * FROM skill_group_skill_act "
          "WHERE city_id=%1";
    if (!execSql(query, sql.arg(QString::number(_cityId))))
      return false;
    while(query.next()) {
      qlonglong skillGroupId = query.value("skill_group_id").toLongLong();
      qlonglong skillId = query.value("skill_id").toLongLong();
      //Группа включает неутвержденный навык (должно быть запрещено на клиенте)
      if (skillId <= 0) {
        qWarning() << "Skill group" << skillGroupId << "contains draft skills. It should be forbidden on client side!";
        continue;
      }
      int sortId = query.value("sort_id").toInt();
      Skill* skill = _skillMap.value(skillId);
      _skillGroupMap.value(skillGroupId)->appendSkill(skill);
      skill->setSortIdByGroup(skillGroupId, sortId);
    }

    //Заполнение списка задач в объектах юнитов
    foreach (Task* task, _taskMap.values()) {
      _skillUnitMap.value(task->getSkillUnitId())->appendTask(task);
    }
    //Заполнение списка юнитов в объектах навыков
    foreach (SkillUnit* skillUnit, _skillUnitMap.values()) {
      _skillMap.value(skillUnit->getSkillId())->appendSkillUnit(skillUnit);
    }
    //Заплнение списка курсов в объектах квантумов
    foreach (Course* course, _courseMap.values()) {
      _quantumMap.value(course->quantumId())->appendCourse(course);
    }
  }
  QSqlDatabase::removeDatabase(CON_REFRESH);
  return true;
}

QList<Course *> StudiesDictionary::coursesByIdList(const QList<qlonglong> &idList)
{
  QList<Course*> resultList;
  foreach (qlonglong id, idList) {
    resultList.append(_courseMap.value(id));
  }
  return resultList;
}

Course *StudiesDictionary::courseById(const qlonglong id)
{
  return _courseMap.value(id);
}

QList<Skill *> StudiesDictionary::skillsByIdList(const QList<qlonglong> &idList)
{
  QList<Skill*> resultList;
  foreach (qlonglong id, idList) {
    resultList.append(_skillMap.value(id));
  }
  return resultList;
}

QList<SkillGroup *> StudiesDictionary::skillGroupsByCourse(qlonglong courseId)
{
  QList<SkillGroup*> resultList;
  Course* course = _courseMap.value(courseId);
  resultList = course->skillGroups();
  return resultList;
}

SkillGroup *StudiesDictionary::skillGroupById(const qlonglong id)
{
  return _skillGroupMap.value(id);
}

Quantum *StudiesDictionary::quantumById(const qlonglong id)
{
  return _quantumMap.value(id);
}

Skill *StudiesDictionary::skillById(const qlonglong id)
{
  return _skillMap.value(id);
}

bool StudiesDictionary::studentCompletedPreviousSkillsInGroup(qlonglong skillId, qlonglong skillGroupId, const QList<qlonglong> &completedSkills)
{
  int sortId = 0;
  if (skillId > 0) {
    sortId = _skillMap.value(skillId)->sortIdByGroup(skillGroupId);
  }
  SkillGroup* skillGroup = _skillGroupMap.value(skillGroupId);
  foreach (Skill* skill, skillGroup->skills()) {
    if ((skill->sortIdByGroup(skillGroupId) >= sortId) && (sortId > 0))
      continue;
    if (!completedSkills.contains(skill->id()))
      return false;
  }
  return true;
}

bool StudiesDictionary::studentHasRequiredKnowledge(qlonglong skillId, qlonglong skillGroupId, qlonglong courseId, const QList<qlonglong> &completedSkills)
{
  bool result = studentCompletedPreviousSkillsInGroup(skillId, skillGroupId, completedSkills);
  if (!result)
    return false;

  SkillGroup* skillGroup = _skillGroupMap.value(skillGroupId);
  foreach (qlonglong requiredGroupId, skillGroup->getRequiredList(courseId)) {
    result = studentHasRequiredKnowledge(0, requiredGroupId, courseId, completedSkills);
    if (!result)
      return false;
  }
  return true;
}

qlonglong StudiesDictionary::version()
{
  return _version;
}

void StudiesDictionary::update()
{
  qlonglong actualVersion = getActualVersion();
  qDebug() << "Current version:" << _version << "\r\nActual version:" << actualVersion;
  if (actualVersion == 0 || actualVersion > _version) {
    _version = actualVersion;
    refreshData();
  }
}

qlonglong StudiesDictionary::getActualVersion()
{
  int result = 0;
  {
    QSqlDatabase db = DatabaseOpenHelper::newConnection(CON_GET_VERSION);
    QSqlQuery query(db);
    query.prepare("select version from structure_versions where city_id=:cityId");
    query.bindValue(":cityId", _cityId);
    bool result = query.exec();
    if (result && query.next()) {
      qlonglong actualVersion = query.value(0).toLongLong();
      result = actualVersion;
    }
    else {
      qWarning() << "Error while getting actual structure version!";
      qCritical() << query.lastError().databaseText();
      result = 0;
    }
  }
  QSqlDatabase::removeDatabase(CON_GET_VERSION);
  return result;
}

bool StudiesDictionary::execSql(QSqlQuery &query, const QString &sql)
{
  qDebug() << "SQL:" << sql;
  bool result = query.exec(sql);
  if (!result)
    qWarning() << "Error:" << query.lastError().databaseText();
  return result;
}

void StudiesDictionary::clear()
{
  foreach (Task* task, _taskMap.values()) {
    delete task;
  }
  _taskMap.clear();

  foreach (SkillUnit* skillUnit, _skillUnitMap.values()) {
    delete skillUnit;
  }
  _skillUnitMap.clear();

  foreach (Skill* skill, _skillMap.values()) {
    delete skill;
  }
  _skillMap.clear();

  foreach (SkillGroup* skillGroup, _skillGroupMap.values()) {
    delete skillGroup;
  }
  _skillGroupMap.clear();

  foreach (Course* course, _courseMap.values()) {
    delete course;
  }
  _courseMap.clear();

  foreach (Quantum* quantum, _quantumMap.values()) {
    delete quantum;
  }
  _quantumMap.clear();
}



StudyItem::StudyItem()
{
}

StudyItem::StudyItem(const QSqlRecord rec)
{
  _id = rec.value(COL_ID).toLongLong();
  _name = rec.value(COL_NAME).toString();
}

StudyItem::~StudyItem()
{
}

QJsonObject StudyItem::toJsonObject(qlonglong param)
{
  Q_UNUSED(param)
  QJsonObject jObj;
  jObj.insert(COL_ID, id());
  jObj.insert(COL_NAME, name());
  return jObj;
}

qlonglong StudyItem::id() const
{
  return _id;
}

void StudyItem::setId(const qlonglong &id)
{
  _id = id;
}

QString StudyItem::name() const
{
  return _name;
}

void StudyItem::setName(const QString &name)
{
  _name = name;
}

Course::Course()
{
}

Course::Course(const QSqlRecord rec) : StudyItem(rec)
{
  _icon = rec.value(COL_ICON).toString();
  _minPercentage = rec.value(COL_MIN_PERCENTAGE).toReal();
  _quantumId = rec.value(COL_QUANTUM_ID).toLongLong();
  _minAge = rec.value(COL_MIN_AGE).toInt();
  _actual = rec.value(COL_ACTUAL).toBool();
  _locked = rec.value(COL_LOCKED).toBool();
  _version = rec.value(COL_VERSION).toInt();
  _originalId = rec.value(COL_ORIGINAL_ID).toLongLong();
}

void Course::appendSkillGroup(SkillGroup *skillGroup)
{
  _skillGroups.append(skillGroup);
}

QString Course::icon() const
{
  return _icon;
}

void Course::setIcon(const QString &icon)
{
  _icon = icon;
}

qreal Course::minPercentage() const
{
  return _minPercentage;
}

void Course::setMinPercentage(qreal minPercentage)
{
  _minPercentage = minPercentage;
}

qlonglong Course::quantumId() const
{
  return _quantumId;
}

void Course::setQuantumId(const qlonglong &quantumId)
{
  _quantumId = quantumId;
}

QList<SkillGroup*> Course::skillGroups() const
{
  return _skillGroups;
}

void Course::setSkillGroups(const QList<SkillGroup*> &skillGroups)
{
  _skillGroups = skillGroups;
}

int Course::minAge() const
{
  return _minAge;
}

void Course::setMinAge(int minAge)
{
  _minAge = minAge;
}

int Course::maxScore()
{
  int result = 0;
  foreach (SkillGroup* skillGr, _skillGroups) {
    foreach (Skill* skill, skillGr->skills()) {
      foreach (SkillUnit* unit, skill->skillUnits()) {
        result += unit->getMaxScore();
        foreach (Task* task, unit->getTasks()) {
          result += task->getMaxScore();
        }
      }
    }
  }
  return result;
}

bool Course::locked() const
{
  return _locked;
}

int Course::version() const
{
  return _version;
}

qlonglong Course::originalId() const
{
  return _originalId;
}

bool Course::isOriginal() const
{
  return id() == _originalId;
}

bool Course::actual() const
{
  return _actual;
}

QJsonObject Course::toJsonObject(qlonglong param)
{
  QJsonObject jObj = StudyItem::toJsonObject(param);
  jObj.insert(COL_ICON, QJsonValue::fromVariant(icon()));
  jObj.insert(COL_MIN_PERCENTAGE, QJsonValue::fromVariant(minPercentage()));
  jObj.insert(COL_MIN_AGE, QJsonValue::fromVariant(minAge()));
  jObj.insert(COL_MAX_SCORE, QJsonValue::fromVariant(maxScore()));
  jObj.insert(COL_VERSION, QJsonValue::fromVariant(version()));
  return jObj;
}

Quantum::Quantum()
{
}

Quantum::Quantum(const QSqlRecord rec) : StudyItem(rec)
{
  _icon = rec.value(COL_ICON).toString();
  _city = rec.value(COL_CITY).toString();
  _description = rec.value(COL_DESCRIPTION).toString();
}

void Quantum::appendCourse(Course *course)
{
  _courses.append(course);
}

QString Quantum::icon() const
{
  return _icon;
}

void Quantum::setIcon(const QString &icon)
{
  _icon = icon;
}

QString Quantum::city() const
{
  return _city;
}

void Quantum::setCity(const QString &city)
{
  _city = city;
}

QString Quantum::description() const
{
  return _description;
}

void Quantum::setDescription(const QString &description)
{
  _description = description;
}

QJsonObject Quantum::toJsonObject(qlonglong param)
{
  QJsonObject jObj = StudyItem::toJsonObject(param);
  jObj.insert(COL_ICON, QJsonValue::fromVariant(icon()));
  jObj.insert(COL_CITY, QJsonValue::fromVariant(city()));
  jObj.insert(COL_DESCRIPTION, QJsonValue::fromVariant(description()));
  jObj.insert(COL_MAX_SCORE, QJsonValue::fromVariant(maxScore()));
  jObj.insert(COL_COURSE_COUNT, QJsonValue::fromVariant(courseCount()));
  return jObj;
}

QList<Course*> Quantum::courses() const
{
  return _courses;
}

void Quantum::setCourses(const QList<Course*> &courses)
{
  _courses = courses;
}

int Quantum::courseCount()
{
  int count = 0;
  foreach(Course* course, _courses) {
    if (course->actual())
      count++;
  }
  return count;
}

int Quantum::maxScore()
{
  int score = 0;
  foreach (Course* course, _courses) {
    if (course->actual())
      score += course->maxScore();
  }
  return score;
}

SkillGroup::SkillGroup()
{
}

SkillGroup::SkillGroup(const QSqlRecord rec) : StudyItem(rec)
{

}

void SkillGroup::appendSkill(Skill *skill)
{
  _skills.append(skill);
}

void SkillGroup::appendRequired(qlonglong courseId, qlonglong skillGroupId)
{
  _requiredByCourseMap.insertMulti(courseId, skillGroupId);
}

QList<qlonglong> SkillGroup::getRequiredList(qlonglong courseId)
{
  return _requiredByCourseMap.values(courseId);
}

QJsonObject SkillGroup::toJsonObject(qlonglong param)
{
  //param - должен быть course_id
  QJsonObject jObj = StudyItem::toJsonObject(param);
  QJsonArray jSkills;
  foreach (Skill* skill, _skills) {
    jSkills.append(skill->toJsonObject(id()));
  }
  jObj.insert(COL_SKILLS, jSkills);

  QJsonArray jRequired;
  foreach (qlonglong requiredId, getRequiredList(param)) {
    jRequired.append(QJsonValue::fromVariant(requiredId));
  }
  jObj.insert(COL_REQUIRED, jRequired);
  return jObj;
}

QList<Skill*> SkillGroup::skills() const
{
  return _skills;
}

void SkillGroup::setSkills(const QList<Skill*> &skills)
{
  _skills = skills;
}

Task::Task()
{
}

Task::Task(const QSqlRecord rec)  : StudyItem(rec)
{
  _maxScore = rec.value(COL_MAX_SCORE).toInt();
  _skillUnitId = rec.value(COL_SKILL_UNIT_ID).toLongLong();
  _sortId = rec.value(COL_SORT_ID).toInt();
}

int Task::getMaxScore() const
{
  return _maxScore;
}

void Task::setMaxScore(int value)
{
  _maxScore = value;
}

qlonglong Task::getSkillUnitId() const
{
  return _skillUnitId;
}

void Task::setSkillUnitId(const qlonglong &skillUnitId)
{
  _skillUnitId = skillUnitId;
}

int Task::getSortId() const
{
  return _sortId;
}

void Task::setSortId(int sortId)
{
  _sortId = sortId;
}

QJsonObject Task::toJsonObject(qlonglong param)
{
  QJsonObject jObj = StudyItem::toJsonObject(param);
  jObj.insert(COL_MAX_SCORE, getMaxScore());
  jObj.insert(COL_SKILL_UNIT_ID, getSkillUnitId());
  jObj.insert(COL_SORT_ID, getSortId());
  return jObj;
}

SkillUnit::SkillUnit()
{
}

SkillUnit::SkillUnit(const QSqlRecord rec) : StudyItem(rec)
{
  _maxScore = rec.value(COL_MAX_SCORE).toInt();
  _skillId = rec.value(COL_SKILL_ID).toLongLong();
  _sortId = rec.value(COL_SORT_ID).toInt();
}

void SkillUnit::appendTask(Task *task)
{
  _tasks.append(task);
}

int SkillUnit::getMaxScore() const
{
  return _maxScore;
}

void SkillUnit::setMaxScore(int value)
{
  _maxScore = value;
}

int SkillUnit::getTotalMaxScore() const
{
  int totalScore = getMaxScore();
  foreach (Task* task, _tasks) {
    totalScore += task->getMaxScore();
  }
  return totalScore;
}

int SkillUnit::getTaskCount() const
{
  return _tasks.count();
}

qlonglong SkillUnit::getSkillId() const
{
  return _skillId;
}

void SkillUnit::setSkillId(const qlonglong &skillId)
{
  _skillId = skillId;
}

int SkillUnit::getSortId() const
{
  return _sortId;
}

void SkillUnit::setSortId(int sortId)
{
  _sortId = sortId;
}

QJsonObject SkillUnit::toJsonObject(qlonglong param)
{
  QJsonObject jObj = StudyItem::toJsonObject(param);
  jObj.insert(COL_MAX_SCORE, getMaxScore());
  jObj.insert(COL_SKILL_ID, getSkillId());
  jObj.insert(COL_SORT_ID, getSortId());
  jObj.insert(COL_TOTAL_MAX_SCORE, getTotalMaxScore());
  jObj.insert(COL_TASK_COUNT, getTaskCount());
  return jObj;
}

QList<Task*> SkillUnit::getTasks() const
{
  return _tasks;
}

void SkillUnit::setTasks(const QList<Task*> &tasks)
{
  _tasks = tasks;
}

Skill::Skill()
{
}

Skill::Skill(const QSqlRecord rec) : StudyItem(rec)
{
  _icon = rec.value(COL_ICON).toString();
  _description = rec.value(COL_DESCRIPTION).toString();
  _minPercentage = rec.value(COL_MIN_PERCENTAGE).toReal();
  _actual = rec.value(COL_ACTUAL).toBool();
  _locked = rec.value(COL_LOCKED).toBool();
  _originalId = rec.value(COL_ORIGINAL_ID).toLongLong();
  _cityId = rec.value(COL_CITY_ID).toLongLong();
}

void Skill::appendSkillUnit(SkillUnit* skillUnit)
{
  _skillUnits.append(skillUnit);
}

void Skill::setSortIdByGroup(qlonglong groupId, int sortId)
{
  _sortIdByGroupMap.insert(groupId, sortId);
}

int Skill::sortIdByGroup(qlonglong groupId)
{
  return _sortIdByGroupMap.value(groupId);
}

int Skill::unitCount() const
{
  return _skillUnits.count();
}

int Skill::getTotalMaxScore() const
{
  int sum = 0;
  foreach (SkillUnit* skillUnit, _skillUnits) {
    sum += skillUnit->getTotalMaxScore();
  }
  return sum;
}

int Skill::getTaskCount() const
{
  int count = 0;
  foreach (SkillUnit* skillUnit, _skillUnits) {
    count += skillUnit->getTaskCount();
  }
  return count;
}

QString Skill::icon() const
{
  return _icon;
}

void Skill::setIcon(const QString &icon)
{
  _icon = icon;
}

QString Skill::description() const
{
  return _description;
}

void Skill::setDescription(const QString &description)
{
  _description = description;
}

qreal Skill::minPercentage() const
{
  return _minPercentage;
}

void Skill::setMinPercentage(const qreal &minPercentage)
{
  _minPercentage = minPercentage;
}

QJsonObject Skill::toJsonObject(qlonglong param)
{
  QJsonObject jObj = StudyItem::toJsonObject(param);
  jObj.insert(COL_UNIT_COUNT, QJsonValue::fromVariant(unitCount()));
  jObj.insert(COL_ICON, QJsonValue::fromVariant(icon()));
  jObj.insert(COL_MIN_PERCENTAGE, QJsonValue::fromVariant(minPercentage()));
  jObj.insert(COL_DESCRIPTION, QJsonValue::fromVariant(description()));
  jObj.insert(COL_TOTAL_MAX_SCORE, QJsonValue::fromVariant(getTotalMaxScore()));
  jObj.insert(COL_TASK_COUNT, QJsonValue::fromVariant(getTaskCount()));
  jObj.insert(COL_SORT_ID, QJsonValue::fromVariant(_sortIdByGroupMap.value(param)));
  jObj.insert(COL_CITY_ID, QJsonValue::fromVariant(getCityId()));
  return jObj;
}

QList<SkillUnit*> Skill::skillUnits() const
{
  return _skillUnits;
}

void Skill::setSkillUnits(const QList<SkillUnit*> &skillUnits)
{
  _skillUnits = skillUnits;
}

bool Skill::getActual() const
{
  return _actual;
}

bool Skill::getLocked() const
{
  return _locked;
}

qlonglong Skill::originalId() const
{
  return _originalId;
}

bool Skill::isOriginal() const
{
  return _originalId == id();
}

qlonglong Skill::getCityId() const
{
  return _cityId;
}

void Skill::setCityId(const qlonglong &cityId)
{
  _cityId = cityId;
}

StudyStructureCache::StudyStructureCache(QObject *parent) : QObject(parent)
{
}

StudiesDictionary *StudyStructureCache::dictByCity(qlonglong cityId)
{
  StudiesDictionary* dict;
  if (_cacheItems.contains(cityId)) {
    dict = _cacheItems.value(cityId);
  }
  else {
    dict = new StudiesDictionary(cityId);
    _cacheItems.insert(cityId, dict);
  }
  qDebug() << "Cache items count:" << _cacheItems.count();
  dict->update();
  return dict;
}
