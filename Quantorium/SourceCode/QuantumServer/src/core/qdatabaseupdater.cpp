#include "qdatabaseupdater.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>

bool QDatabaseUpdater::updateDatabase(QSqlDatabase &db, QString versionTable, QString versionField)
{
  if (!db.isOpen() || db.isOpenError()) {
    qFatal(qUtf8Printable(SErrDatabaseConnection.arg(db.lastError().databaseText())));
  }
  log(SMsgUpdateIsStarting);
  int currentVersion = getDbVersion(db, versionTable, versionField);
  log(SMsgCurrentDbVersion.arg(QString::number(currentVersion)));
  QStringList queryList = initializeUpdateScript();
  while (currentVersion < queryList.size()){
    QSqlQuery query = db.exec(queryList.at(currentVersion));
    if (query.lastError().type() != QSqlError::NoError){
      log(SErrCantUpdateToVer.arg(QString::number(currentVersion + 1)));
      return false;
    }
    setDbVersion(db, versionTable, versionField, ++currentVersion);
    log(SMsgDbUpdatedToVersion.arg(QString::number(currentVersion)));
  }
  log(SMsgUpdateCompleted);
  return true;
}

int QDatabaseUpdater::getDbVersion(QSqlDatabase &db, QString versionTable, QString versionField)
{
  QString sql = SQL_GET_VERSION;
  QSqlQuery query = db.exec(sql.arg(versionField, versionTable));
  if (query.lastError().type() != QSqlError::NoError){
    qDebug() << query.lastError().databaseText();
    return -1;
  }
  if (!query.next()){
    log("No record in version table " + versionTable);
    return -1;
  }
  return query.value(versionField).toInt();
}

void QDatabaseUpdater::setDbVersion(QSqlDatabase &db, QString versionTable, QString versionField, int version)
{  
  db.exec(QString(SQL_SET_VERSION).arg(versionTable, versionField, QString::number(version)));
}

QStringList QDatabaseUpdater::initializeUpdateScript()
{
  QStringList scriptList;
  //1
  QString sql1 = "CREATE OR REPLACE FUNCTION public.clone_case(\r\n"
      "source_case_id bigint, \r\n"
      "user_id bigint) \r\n"
    "RETURNS bigint AS \r\n"
  "$BODY$DECLARE \r\n"
  " new_case_id bigint; \r\n"
  "BEGIN \r\n"
  "  SELECT id FROM \"case\" WHERE id = source_case_id INTO new_case_id; \r\n"
  "\r\n"
  " IF new_case_id IS NULL THEN \r\n"
  "   RAISE EXCEPTION 'Попытка клонирования несуществующего кейса (id=%)', source_case_id; \r\n"
  " END IF; \r\n"
  "\r\n"
  " INSERT INTO \"case\"(title, icon, description, min_percentage, min_age, author_id) \r\n"
  "   SELECT title, icon, description, min_percentage, min_age, user_id FROM \"case\" \r\n"
  "     WHERE id = source_case_id RETURNING id INTO new_case_id; \r\n"
  "\r\n"
  " INSERT INTO case_required_achievement(case_id, achievement_id) \r\n"
  "   SELECT new_case_id, achievement_id FROM case_required_achievement \r\n"
  "     WHERE case_id = source_case_id; \r\n"
  "\r\n"
  " INSERT INTO case_achievement(case_id, achievement_id) \r\n"
  "   SELECT new_case_id, achievement_id FROM case_achievement \r\n"
  "     WHERE case_id = source_case_id; \r\n"
  "\r\n"
  " INSERT INTO case_unit(title, sort_id, description, case_id) \r\n"
  "   SELECT title, sort_id, description, new_case_id FROM case_unit \r\n"
  "     WHERE case_id = source_case_id ORDER BY sort_id; \r\n"
  "\r\n"
  " INSERT INTO task(name, max_score, sort_id, description, case_id) \r\n"
  "   SELECT t.name, t.max_score, t.sort_id, t.description, new_case_id \r\n"
  "   FROM task t \r\n"
  "   WHERE t.case_id = source_case_id \r\n"
  "   ORDER BY t.sort_id; \r\n"
  "\r\n"
  " RETURN new_case_id; \r\n"
  "END$BODY$ \r\n"
  " LANGUAGE plpgsql VOLATILE \r\n"
  " COST 100";
  scriptList.append(sql1);
  //2
  QString sql2 = "ALTER TABLE \"case\" DROP COLUMN coins";
  scriptList.append(sql2);

  return scriptList;
}

void QDatabaseUpdater::log(QString msg)
{
  qDebug() << msg;
}
