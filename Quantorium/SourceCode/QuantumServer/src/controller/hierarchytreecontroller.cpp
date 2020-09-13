#include "hierarchytreecontroller.h"
#include "../core/appconst.h"
#include "../utils/qsqlqueryhelper.h"

#define FIELD_CHILDREN "children"
#define FIELD_TITLE "title"
#define FIELD_ID "id"
#define FIELD_DESCRIPTION "description"
#define FIELD_TYPE "type"
#define FIELD_VALID_MONTHS "valid_months"

#define TYPE_ACHIEVEMENT_GROUP "achievement_group"
#define TYPE_ACHIEVEMENT "achievement"

HierarchyTreeController::HierarchyTreeController(Context &con, QString tableName, QString idField, QString childIdField)
  : BaseJsonActionController(con)
{
  registerAction(new GetTreeAction("get_achievement_group_structure", "Структура групп ачивок", tableName, idField, childIdField));
  registerAction(new SaveTreeAction("save_achievement_group_structure", "Сохранение структуры ачивок", tableName, idField, childIdField));
}

GetTreeAction::GetTreeAction(QString name, QString desc, QString tableName, QString idField, QString childIdField)
  : AbstractJsonAction(name, desc)
{
  _tableName = tableName;
  _idField = idField;
  _childIdField = childIdField;
}

void GetTreeAction::addRequiredSkillGroups(QJsonArray &parentArr, qlonglong skillGroupId, qlonglong courseId, StudiesDictionary* dict)
{
  SkillGroup* skillGroup = dict->skillGroupById(skillGroupId);
  QJsonObject jSkillGroup = skillGroup->toJsonObject(courseId);
  QJsonArray requiredArr;
  QList<qlonglong> requiredIds = skillGroup->getRequiredList(courseId);
  foreach (qlonglong reqId, requiredIds) {
      addRequiredSkillGroups(requiredArr, reqId, courseId, dict);
  }
  jSkillGroup.insert(COL_PARENTS, requiredArr);
  parentArr.append(jSkillGroup);
}


bool GetTreeAction::checkUrlMatch(QString path, QString method)
{
  Q_UNUSED(path)
  return method == HTTP_GET;
}

ActionResult GetTreeAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(bodyData)

  if (!params.contains(PRM_ACHIEVEMENT_GROUP_ID) && !params.contains(PRM_CITY_ID)) {
    return ActionResult(SErrNotEnoughParams.arg(PRM_ACHIEVEMENT_GROUP_ID ", " PRM_CITY_ID), ERR_DEFAULT_CUSTOM);
  }
  qlonglong paramGroupId = params.value(PRM_ACHIEVEMENT_GROUP_ID).toLongLong();
  qlonglong cityId = params.value(PRM_CITY_ID).toLongLong();
  QString filterSql;
  if (paramGroupId > 0) {
    filterSql = QString("ag1.id=%1").arg(paramGroupId);
  }
  if (cityId > 0) {
    filterSql = QString("ag1.parent_id is null and city_id=%1").arg(cityId);
  }
  QString sql =
  "with recursive temp1(group_id, group_title, group_parent_id, city_id, top_parent_id, level) as ( "
  "select *, ag1.id, 1 from achievement_group ag1 "
  "where %1 "
  "UNION ALL "
  "select ag2.*, temp1.top_parent_id, level+1 as level from achievement_group ag2 "
  "inner join temp1 on (ag2.parent_id=temp1.group_id) "
  ") select * from temp1 "
  "left join achievement a on a.achievement_group_id=temp1.group_id "
  "order by 5,6,1";

  QString preparedSql = sql.arg(filterSql);
  QSqlQuery query = QSqlQueryHelper::execSql(preparedSql, con.uid());
  if (query.lastError().type() != QSqlError::NoError) {
    return handleDatabaseError(query.lastError());
  }

  QMap<int, StructureItem*> rootGroups;
  QMap<int, StructureItem*> allGroups;

  while (query.next()) {
    QSqlRecord rec = query.record();
    int groupId = rec.value("group_id").toInt();
    int parentGroupId = rec.value("group_parent_id").toInt();

    if (!allGroups.contains(groupId)) {
      StructureItem* groupItem = new StructureItem(rec, TYPE_ACHIEVEMENT_GROUP);
      allGroups.insert(groupId, groupItem);

      if (parentGroupId > 0) {
        if (allGroups.contains(parentGroupId)) {
          allGroups.value(parentGroupId)->addChild(groupItem);
        }
        else {
          qWarning() << "Probably incorrect sorting in sql query";
        }
      }
      if (groupId == paramGroupId || parentGroupId == 0) {
        rootGroups.insert(groupId, groupItem);
      }
    }

    if (!query.value(COL_ID).isNull()) {
      StructureItem* achievementItem = new StructureItem(rec, TYPE_ACHIEVEMENT);
      allGroups.value(groupId)->addChild(achievementItem);
    }
  }
  QJsonArray rootArray;
  while(!rootGroups.isEmpty()) {
    int idx = rootGroups.firstKey();
    StructureItem* rootGroup = rootGroups.take(idx);
    rootArray.append(rootGroup->jObj());
    delete rootGroup;
  }
  return ActionResult("Иерархическое содержание группы достижений", ERR_NO_ERROR, rootArray);
}

ActionResult GetTreeAction::handleDatabaseError(const QSqlError &error)
{
  return AbstractJsonAction::handleDatabaseError(error);
}

SaveTreeAction::SaveTreeAction(QString name, QString desc, QString tableName, QString idField, QString childIdField)
  : AbstractJsonAction(name, desc)
{
  _tableName = tableName;
  _idField = idField;
  _childIdField = childIdField;
}

bool SaveTreeAction::checkUrlMatch(QString path, QString method)
{
  Q_UNUSED(path)
  return method == HTTP_POST;
}

ActionResult SaveTreeAction::doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData)
{
  Q_UNUSED(con)
  Q_UNUSED(params)
  Q_UNUSED(bodyData)
  return ActionResult("Сохранение изменений пока еще не реализовано", ERR_DEFAULT_CUSTOM);
}

StructureItem::StructureItem(const QSqlRecord &rec, QString type)
{
  _type = type;
  if (_type == TYPE_ACHIEVEMENT_GROUP) {
    _fields.insert(FIELD_ID, rec.value("group_id").toInt());
    _fields.insert(FIELD_TYPE, "achievement_group");
    _fields.insert(FIELD_TITLE, rec.value("group_title").toString());
  }
  else if (_type == TYPE_ACHIEVEMENT) {
    _fields.insert(FIELD_ID, rec.value(FIELD_ID).toInt());
    _fields.insert(FIELD_TYPE, TYPE_ACHIEVEMENT);
    _fields.insert(FIELD_TITLE, rec.value(FIELD_TITLE).toString());
    _fields.insert(FIELD_DESCRIPTION, rec.value(FIELD_DESCRIPTION).toString());
    _fields.insert(FIELD_VALID_MONTHS, rec.value(FIELD_VALID_MONTHS).toInt());
  }
}

StructureItem::~StructureItem()
{
//  qDebug() << "Destructor:" << _fields.value("title").toString();
  while(!_children.isEmpty()) {
    delete _children.takeFirst();
  }
}

void StructureItem::addChild(StructureItem* child)
{
  if (_type == TYPE_ACHIEVEMENT) {
    qWarning() << "Child items not allowed for achievement type";
    return;
  }
  else if (_type == TYPE_ACHIEVEMENT_GROUP) {
    _children.append(child);
  }
  else {
    qCritical() << "Unknown item type";
  }
}

QJsonObject StructureItem::jObj()
{
  QJsonObject obj = QJsonObject::fromVariantMap(_fields);
  QJsonArray childrenArr;
  if (_type == TYPE_ACHIEVEMENT_GROUP) {
    foreach (StructureItem* child, _children) {
      QJsonObject childObj = child->jObj();
      childrenArr.append(childObj);
    }
    obj.insert(FIELD_CHILDREN, childrenArr);
  }
  else if (_type == TYPE_ACHIEVEMENT) {
    //Нет дочерних элементов
  }
  else {
    qCritical() << "Unknown item type";
  }
  return obj;
}
