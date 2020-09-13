#ifndef SUPERUSERACTIONCONTROLLER_H
#define SUPERUSERACTIONCONTROLLER_H

#include <QObject>
#include "basejsonactioncontroller.h"

class SuperUserActionController : public BaseJsonActionController
{
  Q_OBJECT
public:
  SuperUserActionController(Context &con);

  // HttpRequestHandler interface
public:
  virtual void service(HttpRequest &request, HttpResponse &response) override;
};

#endif // SUPERUSERACTIONCONTROLLER_H
