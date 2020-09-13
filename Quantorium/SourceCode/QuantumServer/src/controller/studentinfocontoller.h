#ifndef STUDENTCOURSESINFOCONTOLLER_H
#define STUDENTCOURSESINFOCONTOLLER_H

#include <QObject>
#include <QBitArray>
#include "basejsonsqlcontroller.h"
#include "../dicts/timinggriddict.h"
#include "../core/studiesdictionary.h"

struct SkillStudentStatus {
  qlonglong skill_id;
  bool ready = false;
  bool completed = false;
  bool inStudy = false;
};

struct SkillInCourse {
  qlonglong skillId;
  qlonglong courseId;
  qlonglong skillGroupId;
};

class StudentInfoContoller : public BaseJsonSqlController
{
  Q_OBJECT
  Q_DISABLE_COPY(StudentInfoContoller)
public:
  StudentInfoContoller(Context &con);

  // HttpRequestHandler interface
public:
  virtual void service(HttpRequest &request, HttpResponse &response);

  // AbstractSqlController interface
protected:
  virtual bool hasAccess();
private:
  QJsonArray scheduleArrayFromRec(QSqlRecord rec, qlonglong timingGridId, qlonglong cityId);

  QJsonArray scheduleBitmaskArr(int dayOfWeek, int bitmask, const TimingGrid &timingGrid);
};

template < class T >
QBitArray toQBit ( T &obj ) {
    int len = sizeof(obj) * 8;
    qint8 *data = (qint8*)(&obj) ;
    QBitArray result(len);
    for ( int i=0; i< sizeof(data); ++i ) {
        for ( int j=0; j<8; ++j ) {
            result.setBit ( i*8 + j, data[i] & (1<<j) ) ;
        }
    }
    return result;
}

#endif // STUDENTCOURSESINFOCONTOLLER_H
