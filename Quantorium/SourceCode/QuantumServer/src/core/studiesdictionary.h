#ifndef STUDIESDICTIONARY_H
#define STUDIESDICTIONARY_H

#include <QObject>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QHash>
#include <QList>

class StudyItem
{
public:
    StudyItem();
    StudyItem(const QSqlRecord rec);
    virtual ~StudyItem();

    virtual QJsonObject toJsonObject(qlonglong param = 0);

    qlonglong id() const;
    void setId(const qlonglong &id);

    QString name() const;
    void setName(const QString &name);

private:
    qlonglong _id;
    QString _name;
};

class Task : public StudyItem
{
public:
    Task();
    Task(const QSqlRecord rec);

    int getMaxScore() const;
    void setMaxScore(int value);

    qlonglong getSkillUnitId() const;
    void setSkillUnitId(const qlonglong &skillUnitId);

    int getSortId() const;
    void setSortId(int sortId);

private:
    int _maxScore = 0;
    qlonglong _skillUnitId;
    int _sortId;

    // StudyItem interface
public:
    virtual QJsonObject toJsonObject(qlonglong param = 0);
};

class SkillUnit : public StudyItem
{
public:
    SkillUnit();
    SkillUnit(const QSqlRecord rec);
    void appendTask(Task* task);

    int getMaxScore() const;
    void setMaxScore(int value);

    int getTotalMaxScore() const;
    int getTaskCount() const;

    qlonglong getSkillId() const;
    void setSkillId(const qlonglong &skillId);

    int getSortId() const;
    void setSortId(int sortId);

    QList<Task*> getTasks() const;
    void setTasks(const QList<Task*> &tasks);

private:
    int _maxScore = 0;
    qlonglong _skillId;
    int _sortId;
    QList<Task*> _tasks;

    // StudyItem interface
public:
    virtual QJsonObject toJsonObject(qlonglong param = 0);
};

class Skill : public StudyItem
{
public:
    Skill();
    Skill(const QSqlRecord rec);
    void appendSkillUnit(SkillUnit* skillUnit);
    void setSortIdByGroup(qlonglong groupId, int sortId);
    int sortIdByGroup(qlonglong groupId);

    int unitCount() const;
    int getTotalMaxScore() const;
    int getTaskCount() const;

    QString icon() const;
    void setIcon(const QString &icon);

    QString description() const;
    void setDescription(const QString &description);

    qreal minPercentage() const;
    void setMinPercentage(const qreal &minPercentage);

    QList<SkillUnit*> skillUnits() const;
    void setSkillUnits(const QList<SkillUnit*> &skillUnits);

    bool getActual() const;
    bool getLocked() const;

    qlonglong originalId() const;
    bool isOriginal() const;

    qlonglong getCityId() const;
    void setCityId(const qlonglong &cityId);
private:
    bool _actual;
    bool _locked;
    qlonglong _originalId;
    QString _icon;
    QString _description;
    qreal _minPercentage = 0;
    qlonglong _cityId;
    QHash<qlonglong, qlonglong> _sortIdByGroupMap;
    QList<SkillUnit*> _skillUnits;

    // StudyItem interface
public:
    virtual QJsonObject toJsonObject(qlonglong param);
};

class SkillGroup : public StudyItem
{
public:
  SkillGroup();
  SkillGroup(const QSqlRecord rec);
    void appendSkill(Skill* skill);
    void appendRequired(qlonglong courseId, qlonglong skillGroupId);

    QList<qlonglong> getRequiredList(qlonglong courseId);

    QList<Skill*> skills() const;
    void setSkills(const QList<Skill*> &skills);

private:
    QList<Skill*> _skills;
    QHash<qlonglong, qlonglong> _requiredByCourseMap;
    // StudyItem interface
public:
    virtual QJsonObject toJsonObject(qlonglong param);
};

class Course : public StudyItem
{
public:
    Course();
    Course(const QSqlRecord rec);
    void appendSkillGroup(SkillGroup* skillGroup);

    QString icon() const;
    void setIcon(const QString &icon);

    qreal minPercentage() const;
    void setMinPercentage(qreal minPercentage);

    qlonglong quantumId() const;
    void setQuantumId(const qlonglong &quantumId);

    QList<SkillGroup*> skillGroups() const;
    void setSkillGroups(const QList<SkillGroup*> &skillGroups);

    int minAge() const;
    void setMinAge(int minAge);

    int maxScore();

    bool actual() const;
    bool locked() const;
    int version() const;

    qlonglong originalId() const;
    bool isOriginal() const;
private:
    bool _actual;
    bool _locked;
    int _version;
    qlonglong _originalId;
    QString _icon;
    qreal _minPercentage = 0;
    qlonglong _quantumId = 0;
    int _minAge = 5;
    QList<qlonglong> _requiredCourses;
    QList<SkillGroup*> _skillGroups;

    // StudyItem interface
public:
    virtual QJsonObject toJsonObject(qlonglong param = 0);
};

class Quantum : public StudyItem
{
public:
    Quantum();
    Quantum(const QSqlRecord rec);
    void appendCourse(Course* course);

    QString icon() const;
    void setIcon(const QString &icon);

    QString city() const;
    void setCity(const QString &city);

    QString description() const;
    void setDescription(const QString &description);

    QList<Course*> courses() const;
    void setCourses(const QList<Course*> &courses);

    int courseCount();
    int maxScore();
private:
    QString _icon;
    QString _city;
    QString _description;
    QList<Course*> _courses;

    // StudyItem interface
public:
    virtual QJsonObject toJsonObject(qlonglong param = 0);
};

class StudiesDictionary : public QObject
{
    Q_OBJECT
public:
    explicit StudiesDictionary(qlonglong cityId, QObject *parent = 0);
    //Перезагрузка кэша
    bool refreshData();    

    QList<Course*> coursesByIdList(const QList<qlonglong> &idList);
    Course* courseById(const qlonglong id);
    QList<Skill*> skillsByIdList(const QList<qlonglong> &idList);
    QList<SkillGroup*> skillGroupsByCourse(qlonglong courseId);
    SkillGroup* skillGroupById(const qlonglong id);
    Quantum* quantumById(const qlonglong id);
    Skill* skillById(const qlonglong id);
    bool studentHasRequiredKnowledge(qlonglong skillId, qlonglong skillGroupId, qlonglong courseId, const QList<qlonglong> &completedSkills);
    qlonglong version();
    void update();
    qlonglong getActualVersion();
signals:

public slots:

private:
    qlonglong _cityId;
    qlonglong _version = -1;
    QHash<qlonglong, Task*> _taskMap;
    QHash<qlonglong, SkillUnit*> _skillUnitMap;
    QHash<qlonglong, Skill*> _skillMap;
    QHash<qlonglong, SkillGroup*> _skillGroupMap;
    QHash<qlonglong, Course*> _courseMap;
    QHash<qlonglong, Quantum*> _quantumMap;
    bool execSql(QSqlQuery &query, const QString &sql);
    void clear();
    bool studentCompletedPreviousSkillsInGroup(qlonglong skillId, qlonglong skillGroupId, const QList<qlonglong> &completedSkills);
};

class StudyStructureCache : public QObject
{
  Q_OBJECT
public:
  StudyStructureCache(QObject *parent = 0);
  StudiesDictionary* dictByCity(qlonglong cityId);
private:
  QHash<qlonglong, StudiesDictionary*> _cacheItems;
};
#endif // STUDIESDICTIONARY_H
