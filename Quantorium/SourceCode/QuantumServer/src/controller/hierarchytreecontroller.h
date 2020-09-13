#ifndef HIERARCHYTREECONTROLLER_H
#define HIERARCHYTREECONTROLLER_H

#include <QObject>
#include "basejsonactioncontroller.h"
#include "../core/studiesdictionary.h"

class StructureItem
{
public:
  StructureItem(const QSqlRecord &rec, QString type);
  virtual ~StructureItem();
  void addChild(StructureItem* child);
  virtual QJsonObject jObj();
private:
  QString _type;
  QVariantMap _fields;
  QList<StructureItem*> _children;
};

class GetTreeAction : public AbstractJsonAction
{
public:
  GetTreeAction(QString name, QString desc, QString tableName, QString idField, QString childIdField);
private:
  QString _tableName;
  QString _idField;
  QString _childIdField;
  void addRequiredSkillGroups(QJsonArray &parentArr, qlonglong skillGroupId, qlonglong courseId, StudiesDictionary* dict);
  QList<qlonglong> getRootSkillGroupsByCourse(const Context &con, qlonglong courseId);
  // AbstractJsonAction interface
public:
  virtual bool checkUrlMatch(QString path, QString method) override;

protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
  virtual ActionResult handleDatabaseError(const QSqlError &error) override;

};

class SaveTreeAction : public AbstractJsonAction
{
public:
  SaveTreeAction(QString name, QString desc, QString tableName, QString idField, QString childIdField);
private:
  QString _tableName;
  QString _idField;
  QString _childIdField;
  // AbstractJsonAction interface
public:
  virtual bool checkUrlMatch(QString path, QString method) override;

  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData) override;
};


class HierarchyTreeController : public BaseJsonActionController
{
  Q_OBJECT
  Q_DISABLE_COPY(HierarchyTreeController)
public:
  HierarchyTreeController(Context &con, QString tableName, QString idField, QString childIdField);
};

#endif // HIERARCHYTREECONTROLLER_H
