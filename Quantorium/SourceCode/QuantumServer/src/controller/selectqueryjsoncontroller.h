#ifndef SELECTQUERYJSONCONTROLLER_H
#define SELECTQUERYJSONCONTROLLER_H

#include "httprequest.h"
#include "httpresponse.h"
#include "httprequesthandler.h"
#include <QJsonDocument>
#include <QSqlQuery>
#include <QSqlRecord>
#include "../httpsessionmanager.h"
#include "basejsonsqlcontroller.h"

const QString SErrMissingParams = "В запросе не хватает одного или нескольких обязательных параметров: %1";


/*!
    Класс контроллера позволяющего получить набор данных произвольного
    sql-запроса. Sql-запрос задается в конструкторе. Контроллер поддерживает
    наличие именованных параметров (:paramName) в тексте запроса. Значения
    параметров берутся из одноименных параметров запроса. Также поддерживается объявление
    опциональных кусков запроса, которые удаляются из финального запроса при отсутствии
    хотя бы одного значения параметра в запросе от клиента.

    Пример: select * from users {where id = :id}
    Если параметр id не будет найден в запросе от клиента фрагмент запроса в {}
    будет исключен
 */
class SelectQueryJsonController : public BaseJsonSqlController
{
  Q_OBJECT
  Q_DISABLE_COPY(SelectQueryJsonController)
public:
  SelectQueryJsonController(QString sql, Context& con);
  virtual void service(HttpRequest &request, HttpResponse &response);
  virtual bool hasAccess();
protected:
  virtual void modifyResultRecord(QJsonObject& jObj);
private:
  QStringList _sql;
  QStringList _missingParams;
  QVariant toVariant(QString str);
  bool prepareQuery(QSqlQuery &query, HttpRequest &request);
};

#endif // SELECTQUERYJSONCONTROLLER_H
