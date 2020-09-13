#ifndef COURSEADMININFOCONTROLLER_H
#define COURSEADMININFOCONTROLLER_H

#include <QObject>
#include "basejsonsqlcontroller.h"

class CourseAdminInfoController : public BaseJsonSqlController
{
public:
  CourseAdminInfoController(Context &con);

  // HttpRequestHandler interface
public:
  virtual void service(HttpRequest &request, HttpResponse &response) override;

  // AbstractSqlController interface
protected:
  virtual bool hasAccess() override;
};

#endif // COURSEADMININFOCONTROLLER_H
