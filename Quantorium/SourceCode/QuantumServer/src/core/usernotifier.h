#ifndef USERNOTIFIER_H
#define USERNOTIFIER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QHash>
#include <QVariantMap>

class AbstractNotification
{
public:
  AbstractNotification(QString msgTemplate);
  virtual QString message(QVariant receivers, QVariantMap params) = 0;
  virtual QString receiverClause(QVariant receivers, QVariantMap params) = 0;
protected:
  QString _msgTemplate;
};

class UserMessageNotification : public AbstractNotification
{
public:
  UserMessageNotification();

  // AbstractNotification interface
public:
  virtual QString message(QVariant receivers, QVariantMap params) override;
  virtual QString receiverClause(QVariant receivers, QVariantMap params) override;
};

class SimpleNotification : public AbstractNotification
{
public:
  SimpleNotification(QString msgTemplate);

  // AbstractNotification interface
public:
  virtual QString message(QVariant receivers, QVariantMap params) override;
  virtual QString receiverClause(QVariant receivers, QVariantMap params) override;
};

class UserTypeNotification : public SimpleNotification
{
public:
  UserTypeNotification(QString msgTemplate);

  // AbstractNotification interface
public:
  virtual QString receiverClause(QVariant receivers, QVariantMap params) override;
};

class LessonGroupNotification : public SimpleNotification
{
public:
  LessonGroupNotification(QString msgTemplate);

  // AbstractNotification interface
public:
  virtual QString receiverClause(QVariant receivers, QVariantMap params) override;
};

class UserNotifier : public QObject
{
  Q_OBJECT
public:
  explicit UserNotifier(QObject *parent = Q_NULLPTR);
  void loadTypes(QSqlDatabase &db);
  void registerNotification(int code, AbstractNotification* notification);
  bool notify(QSqlDatabase &db, int code, QVariant receivers, QVariantMap params = QVariantMap(), int senderId = -1);
signals:

public slots:

private:
  QHash<int, AbstractNotification*> _notifications;
};

#endif // USERNOTIFIER_H
