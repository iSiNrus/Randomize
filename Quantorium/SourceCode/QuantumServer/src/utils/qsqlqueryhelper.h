#ifndef QSQLQUERYHELPER_H
#define QSQLQUERYHELPER_H

#include <QSqlDatabase>
#include <QSqlQuery>

#include <QSqlError>
#include <QDebug>

#define DEBUG_SQL

class QSqlQueryHelper
{
public:
  static QSqlQuery execSql(QString sql, QString connectionName = QSqlDatabase::defaultConnection);
  static QSqlQuery execSql(QString sql, QSqlDatabase &db);
  static qlonglong getCurrentSequenceValue(QString sequenceName, QString connectionName = QSqlDatabase::defaultConnection);
  static QVariant getSingleValue(QString sql, QString connectionName = QSqlDatabase::defaultConnection);
  static QVariant getSingleValue(QString sql, QSqlDatabase &db);
  static QSqlRecord getSingleRow(QString sql, QString connectionName = QSqlDatabase::defaultConnection);
  static QString databaseName(QString connection);
  static QString driverName(QString connection);
  static QSqlRecord tableRowInfo(QString table, QString connectionName);
  static QString fillSqlPattern(QString pattern, QObject* object);
  static QString fillSqlPattern(QString pattern, const QVariantMap &params);
  static void fillObjectFromRecord(QObject* object, QSqlRecord& rec);
  static QString encaseValue(QString strVal, QVariant::Type type);
  static void prepareInsertQuery(QSqlQuery &query, QString tableName, const QJsonObject &jObj);
  static void prepareUpdateQuery(QSqlQuery &query, QString tableName, const QJsonObject &jObj);
  static void prepareDeleteQuery(QSqlQuery &query, QString tableName, const QVariant &idVal);
  static void setLogging(bool enable);
private:
  static bool _logging;
};

#endif // QSQLQUERYHELPER_H
