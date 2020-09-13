#include "abstractsqlcontroller.h"
#include <QSqlError>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

//Фильтруемые по умолчанию поля (связаны с паролем)
#define DEFAULT_FIELD_FILTERS "password" << "pswd" << "pass"

AbstractSqlController::AbstractSqlController() : HttpRequestHandler()
{
  _fieldFilters << DEFAULT_FIELD_FILTERS;
}

AbstractSqlController::AbstractSqlController(Context &con) : _context(con)
{
  _fieldFilters << DEFAULT_FIELD_FILTERS;
}

AbstractSqlController::~AbstractSqlController()
{
}

void AbstractSqlController::service(HttpRequest &request, HttpResponse &response)
{
  if (!hasAccess()) {
    //Forbidden
    response.setStatus(403);
    writeError(response, SErrForbidden, 403);
  }
}

void AbstractSqlController::addFieldFilter(QString fieldName)
{
  _fieldFilters.append(fieldName);
}

void AbstractSqlController::writeError(HttpResponse &response, QString errorMsg, int code)
{
  response.setHeader("Content-Type", "text/html; charset=UTF-8");
  response.write(SErrorMessageTemplate.arg(QString::number(code)).arg(errorMsg).toUtf8());
}
