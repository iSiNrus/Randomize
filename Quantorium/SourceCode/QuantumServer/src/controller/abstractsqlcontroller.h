#ifndef ABSTRACTSQLCONTROLLER_H
#define ABSTRACTSQLCONTROLLER_H

#include "httprequest.h"
#include "httpresponse.h"
#include "httprequesthandler.h"
#include <QJsonDocument>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include "../httpsessionmanager.h"

const QString SErrorMessageTemplate = "Ошибка %1: %2";
const QString SErrForbidden = "Пользователь не имеет доступа к данному ресурсу";

class AbstractSqlController : public HttpRequestHandler
{
  Q_OBJECT
  Q_DISABLE_COPY(AbstractSqlController)
public:
  AbstractSqlController();
  AbstractSqlController(Context &con);
  virtual ~AbstractSqlController();
  virtual void service(HttpRequest &request, HttpResponse &response);
  //Регистрация дополнительного фильтра на возвращаемые поля
  void addFieldFilter(QString fieldName);
protected:
  Context _context;
  QStringList _fieldFilters;
  virtual bool hasAccess() = 0;
  virtual void writeError(HttpResponse &response, QString errorMsg, int code = 100);
private:

};

#endif // ABSTRACTSQLCONTROLLER_H
