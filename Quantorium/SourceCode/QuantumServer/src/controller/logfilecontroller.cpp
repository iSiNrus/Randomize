#include "logfilecontroller.h"
#include "../core/api.h"
#include "../core/appconst.h"
#include "../core/appsettings.h"
#include <QDate>
#include <QFile>


LogFileController::LogFileController(Context &con) : AbstractSqlController(con)
{
}

void LogFileController::service(HttpRequest &request, HttpResponse &response)
{
  AbstractSqlController::service(request, response);
  if (response.getStatusCode() != HTTP_NO_ERROR)
    return;

  if (!request.parameters.contains("date")) {
    response.setStatus(HTTP_NOT_FOUND);
    return;
  }

  QDate date = QDate::fromString(QString::fromLatin1(request.getParameter("date")), DATE_FORMAT);
  QString fullPath = logFileName(date);
  QFile logFile(fullPath);
  if (logFile.open(QIODevice::ReadOnly)) {
    response.setHeader("Content-Type", "application/octet-stream");
    response.write(logFile.readAll(), true);
    logFile.close();
  }
  else {
    QString msg404 = "No file at %1";
    response.setStatus(HTTP_NOT_FOUND, msg404.arg(fullPath).toLatin1());
  }

}

bool LogFileController::hasAccess()
{
  return _context.userType() == SuperUser;
}

QString LogFileController::logFileName(const QDate &date)
{
  QString pattern = AppSettings::applicationPath() + DELIMITER + AppSettings::strVal(SECTION_LOGGER, PRM_LOG_FILE);
  QStringList urlSections = pattern.split(DELIMITER, QString::SkipEmptyParts);
  QString filenamePattern = urlSections.last();
  urlSections.removeLast();
  urlSections.append(date.toString(DATE_FORMAT) + "-" + filenamePattern);
  return urlSections.join(DELIMITER);
}
