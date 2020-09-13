#ifndef LOGFILECONTROLLER_H
#define LOGFILECONTROLLER_H

#include <QObject>
#include "abstractsqlcontroller.h"

//Контроллер получения логов за день
class LogFileController : public AbstractSqlController
{
  Q_OBJECT
  Q_DISABLE_COPY(LogFileController)
public:
  LogFileController(Context &con);

  // HttpRequestHandler interface
public:
  virtual void service(HttpRequest &request, HttpResponse &response) override;

  // AbstractSqlController interface
protected:
  virtual bool hasAccess() override;
private:
  QString logFileName(const QDate &date);
};

#endif // LOGFILECONTROLLER_H
