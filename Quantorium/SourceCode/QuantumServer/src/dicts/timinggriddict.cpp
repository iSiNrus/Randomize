#include "timinggriddict.h"
#include "../utils/qsqlqueryhelper.h"
#include "../databaseopenhelper.h"
#include <QDebug>

TimingGridDict* TimingGridDict::_instance = 0;


TimingGridDict *TimingGridDict::instance()
{
  if (!_instance) {
    _instance = new TimingGridDict();
  }
  return _instance;
}

TimingGrid TimingGridDict::getById(qlonglong id, QSqlDatabase &db)
{
  if (isOutDated(id, db)) {
    updateTimingGrid(id, db);
  }
  return _dataMap.value(id);
}

TimingGrid TimingGridDict::getDefaultByCity(qlonglong cityId, QSqlDatabase &db)
{
  QString sql = "select id from timing_grid where city_default=true and city_id=%1";
  qlonglong timingGridId =
      QSqlQueryHelper::getSingleValue(sql.arg(QString::number(cityId)), db).toLongLong();
  return getById(timingGridId, db);
}

TimingGridDict::TimingGridDict()
{
}

bool TimingGridDict::isOutDated(qlonglong id, QSqlDatabase &db)
{
  if (!_dataMap.contains(id))
    return true;
  QString sql = "select version from timing_grid where id=%1";
  int actualVersion =
      QSqlQueryHelper::getSingleValue(sql.arg(QString::number(id)), db).toLongLong();
  return actualVersion > versionById(id);
}

void TimingGridDict::updateTimingGrid(qlonglong id, QSqlDatabase &db)
{
  QString sql = "select * from timing_grid where id=%1";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(QString::number(id)), db);
  TimingGrid timingGrid;
  if (query.next()) {
    timingGrid = TimingGrid(query.value("name").toString(),
                            query.value("duration").toInt(),
                            query.value("city_id").toLongLong(),
                            query.value("version").toLongLong());
  }
  sql = "select * from timing_grid_unit where timing_grid_id=%1";
  query = QSqlQueryHelper::execSql(sql.arg(QString::number(id)), db);
  while (query.next()) {
    int dayType = query.value("day_type").toInt();
    int orderNum = query.value("order_num").toInt();
    QTime startTime = query.value("begin_time").toTime();
    timingGrid.addTime(dayType, orderNum, startTime);
  }
  _dataMap.insert(id, timingGrid);
}

qlonglong TimingGridDict::versionById(qlonglong id)
{
  if (_dataMap.contains(id)) {
    return _dataMap.value(id).version();
  }
  else {
    return 0;
  }
}

TimingGrid::TimingGrid()
{
  _name = "";
  _duration = 40;
  _cityId = 0;
  _version = 0;
}

TimingGrid::TimingGrid(QString name, int duration, qlonglong cityId, qlonglong version)
{
  _name = name;
  _duration = duration;
  _cityId = cityId;
  _version = version;
}

QString TimingGrid::name() const
{
  return _name;
}

int TimingGrid::duration() const
{
  return _duration;
}

int TimingGrid::version() const
{
  return _version;
}

void TimingGrid::setVersion(int version)
{
  _version = version;
}

QJsonObject TimingGrid::toJsonObj()
{
  QJsonObject jObj;
  jObj.insert("version", version());
  jObj.insert("name", name());
  jObj.insert("city_id", cityId());
  jObj.insert("duration", duration());
  return jObj;
}

void TimingGrid::addTime(int dayType, int lessonNum, QTime startTime)
{
  qDebug() << "Add time:" << dayType << lessonNum << startTime;
  _timingsMap.insert(TimingIndex(dayType, lessonNum), startTime);
}

QTime TimingGrid::startTime(int dow, int num) const
{
  TimingIndex idx(dow, num);
  if (_timingsMap.contains(idx)) {
    return _timingsMap.value(idx);
  }
  idx = TimingIndex(0, num);
  if (_timingsMap.contains(idx)) {
    return _timingsMap.value(idx);
  }
  return QTime();
}

QTime TimingGrid::endTime(int dow, int num) const
{
  QTime start = startTime(dow, num);
  if (!start.isValid())
    return QTime();
  return start.addSecs(duration()*60);
}

qlonglong TimingGrid::cityId() const
{
    return _cityId;
}

TimingIndex::TimingIndex(int d, int n)
{
    dayType = d;
    orderNum = n;
}
