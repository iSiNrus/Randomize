#ifndef TIMINGGRIDDICT_H
#define TIMINGGRIDDICT_H

#include <QHash>
#include <QJsonValue>
#include <QJsonObject>
#include <QTime>
#include <QSqlDatabase>

struct TimingIndex
{
public:
  TimingIndex(int d, int n);
  int dayType;
  int orderNum;
};

inline bool operator==(const TimingIndex &idx1, const TimingIndex &idx2)
{
  return (idx1.dayType == idx2.dayType) && (idx1.orderNum == idx2.orderNum);
}

inline uint qHash(const TimingIndex &key, uint seed = 0) {
  return qHash(key.dayType, seed) ^ qHash(key.orderNum, seed);
}

class TimingGrid
{
public:
  TimingGrid();
  TimingGrid(QString name, int duration, qlonglong cityId, qlonglong version);
  QString name() const;
  int duration() const;
  int version() const;
  void setVersion(int version);
  QJsonObject toJsonObj();
  void addTime(int dayType, int lessonNum, QTime startTime);
  QTime startTime(int dow, int num) const;
  QTime endTime(int dow, int num) const;
  qlonglong cityId() const;

private:
  int _version;
  QString _name;
  int _duration;
  qlonglong _cityId;
  QHash<TimingIndex, QTime> _timingsMap;
};

class TimingGridDict
{
public:
  static TimingGridDict* instance();
  TimingGrid getById(qlonglong id, QSqlDatabase &db);
  TimingGrid getDefaultByCity(qlonglong cityId, QSqlDatabase &db);
private:
  TimingGridDict();
  bool isOutDated(qlonglong id, QSqlDatabase &db);
  void updateTimingGrid(qlonglong id, QSqlDatabase &db);
  qlonglong versionById(qlonglong id);
  static TimingGridDict* _instance;
  QHash<qlonglong, TimingGrid> _dataMap;
};

#endif // TIMINGGRIDDICT_H
