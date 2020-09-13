#ifndef USERTABLECONTROLLER_H
#define USERTABLECONTROLLER_H

#include <QObject>
#include "cityrestrictedtablecontroller.h"

/**
 * Контроллер для работы со справочником пользователей
 * - ученики и учителя не могут редактировать справочник и видят только учителей и учеников
 * -

**/

class UserInsertAction : public InsertAction
{
public:
  UserInsertAction(QString table, QString name, QString description, QStringList fieldFilters = QStringList());
  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue());
};

class UserUpdateAction : public UpdateAction
{
public:
  UserUpdateAction(QString table, QString name, QString description, QStringList fieldFilters = QStringList());
  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue());
};

class UserDeleteAction : public DeleteAction
{
public:
  UserDeleteAction(QString table, QString name, QString description, QStringList fieldFilters = QStringList());
  // AbstractJsonAction interface
protected:
  virtual ActionResult doInternalAction(const Context &con, const QVariantMap &params, const QJsonValue bodyData = QJsonValue());
};

class UserTableController : public CityRestrictedTableController
{
  Q_OBJECT
  Q_DISABLE_COPY(UserTableController)
public:
  UserTableController(QString tableName, Context& con);

  // AbstractSqlController interface
protected:
  virtual bool hasAccess();

  // BaseRestTableController interface
protected:
  virtual QString filterFromParams(const QSqlRecord &infoRec);
};

#endif // USERTABLECONTROLLER_H
