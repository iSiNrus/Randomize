#include "usernotifier.h"
#include "notifications.h"
#include "api.h"
#include <QDebug>

UserNotifier::UserNotifier(QObject *parent) : QObject(parent)
{  

}

void UserNotifier::loadTypes(QSqlDatabase &db)
{
  QSqlQuery query = db.exec("select * from notice_type");
  while (query.next()) {
    int code = query.value(PRM_ID).toLongLong();
    QString pattern = query.value(PRM_PATTERN).toString();
    QString className = query.value(PRM_CLASS_NAME).toString();
    AbstractNotification* notification;
    if (className == "UserMessageNotification") {
      notification = new UserMessageNotification();
    }
    else {
      notification = new SimpleNotification(pattern);
    }
    registerNotification(code, notification);
  }
}

void UserNotifier::registerNotification(int code, AbstractNotification *notification)
{
  qDebug() << "Registering notification type:" << code;
  _notifications.insert(code, notification);
}

bool UserNotifier::notify(QSqlDatabase &db, int code, QVariant receivers, QVariantMap params, int senderId)
{
  if (!_notifications.contains(code)) {
    qWarning() << "Unknown notification type!";
    return false;
  }
  QString sql =
      "insert into notice(code, receiver_id, description, sender_id) "
      "select %1, id, '%2', %4 from sysuser "
      "where %3";
  QString msg = _notifications.value(code)->message(receivers, params);
  QString whereClause = _notifications.value(code)->receiverClause(receivers, params);
  if (whereClause.isEmpty())
    whereClause = "1=1";
  QString sender = (senderId < 0) ? "null" : QString::number(senderId);
  QSqlQuery query = db.exec(sql.arg(code).arg(msg).arg(whereClause).arg(sender));
  qDebug() << "Notifier sql:" << query.lastQuery();
  if (query.lastError().type() != QSqlError::NoError) {
    qWarning() << "Error:" << query.lastError().databaseText();
    return false;
  }
  return true;
}


SimpleNotification::SimpleNotification(QString msgTemplate) : AbstractNotification(msgTemplate)
{
}

QString SimpleNotification::message(QVariant receivers, QVariantMap params)
{
  Q_UNUSED(receivers)
  QString msg = _msgTemplate;
  foreach (QString paramName, params.keys()) {
    msg.replace("#" + paramName + "#", params.value(paramName).toString());
  }
  return msg;
}

AbstractNotification::AbstractNotification(QString msgTemplate)
{
  _msgTemplate = msgTemplate;
}

UserMessageNotification::UserMessageNotification() : AbstractNotification("")
{
}

QString UserMessageNotification::message(QVariant receivers, QVariantMap params)
{
  Q_UNUSED(receivers)
  QString msg = params.value(PRM_MESSAGE).toString();
  return msg;
}

QString SimpleNotification::receiverClause(QVariant receivers, QVariantMap params)
{
  Q_UNUSED(params)
  QString clause = "id=%1";
  return clause.arg(receivers.toLongLong());
}


QString UserMessageNotification::receiverClause(QVariant receivers, QVariantMap params)
{
  Q_UNUSED(params)
  QString clause = "id=%1";
  return clause.arg(receivers.toLongLong());
}

UserTypeNotification::UserTypeNotification(QString msgTemplate) : SimpleNotification(msgTemplate)
{
}


QString UserTypeNotification::receiverClause(QVariant receivers, QVariantMap params)
{
  Q_UNUSED(params)
  QString clause = "user_type_id=%1";
  return clause.arg(receivers.toLongLong());
}

LessonGroupNotification::LessonGroupNotification(QString msgTemplate) : SimpleNotification(msgTemplate)
{
}

QString LessonGroupNotification::receiverClause(QVariant receivers, QVariantMap params)
{
  Q_UNUSED(params)
  QString clause = "id=(select user_id from user_learning_group where learning_group_id=%1)";
  return clause.arg(receivers.toLongLong());
}
