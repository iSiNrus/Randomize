#ifndef TESTCONTROLLER_H
#define TESTCONTROLLER_H

#include <QObject>
#include "basejsonsqlcontroller.h"

class TestController : public BaseJsonSqlController
{
  Q_OBJECT
  Q_DISABLE_COPY(TestController)
public:
  TestController(Context &con);

  // HttpRequestHandler interface
public:
  virtual void service(HttpRequest &request, HttpResponse &response) override;
};

#endif // TESTCONTROLLER_H
