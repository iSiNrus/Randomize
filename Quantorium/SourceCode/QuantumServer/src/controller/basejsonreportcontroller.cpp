#include "basejsonreportcontroller.h"
#include <QSqlQuery>
#include <QJsonArray>
#include "../core/appconst.h"
#include "../utils/qsqlqueryhelper.h"


BaseJsonReportController::BaseJsonReportController(Context &con) : BaseJsonSqlController(con)
{
  _templater = new RtfTemplater(this);
  _dataModel = new AutosizeJsonTableModel(this);
}

void BaseJsonReportController::service(HttpRequest &request, HttpResponse &response)
{
  BaseJsonSqlController::service(request, response);
  if (response.getStatusCode() != 200)
    return;
  QString path = paramString(PRM_PATH);
  int reportId = path.section("/", -1).toInt();
  if (reportId == 0) {
    qDebug() << "Report list command";
    reportList(response);
  }
  else {
    qDebug() << "Report data command id=" << reportId;
    if (initReportData(reportId)) {
      QString report = _templater->prepareDoc(_dataModel, params());
      response.setHeader("Content-Type", "text/richtext");
      response.write(report.toLocal8Bit(), true);
    }
    else {
      writeError(response, _lastErrorString);
    }
  }
}

void BaseJsonReportController::reportList(HttpResponse &response)
{
  qDebug() << "Report list for city" << _context.city_id();
  QString sql = "select * from report where city_id=%1";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(_context.city_id()), _context.uid());
  writeResultOk(response, "Список доступных отчетов", sqlResultToJson(query).array());
}

bool BaseJsonReportController::initReportModel(QString queryTemplate)
{
  QString sql = QSqlQueryHelper::fillSqlPattern(queryTemplate, params());
  QSqlQuery query = QSqlQueryHelper::execSql(sql, _context.uid());
  QJsonArray dataArr = sqlResultToJson(query).array();
  _dataModel->loadData(dataArr);
  return true;
}

bool BaseJsonReportController::initReportData(qlonglong reportId)
{
  bool resOk = true;
  QString sql = "select template_url, sql_template from report where id=%1";
  QSqlQuery query = QSqlQueryHelper::execSql(sql.arg(reportId), _context.uid());
  if (query.next()) {
    QString sqlTemplate = query.value("sql_template").toString();
    resOk = initReportModel(sqlTemplate);
    if (!resOk) {
      _lastErrorString = "Cannot load report data from DB!";
      return false;
    }
    QString templatePath = query.value("template_url").toString();
    resOk = _templater->loadTemplate(getDocRootPath() + templatePath);
    if (!resOk) {
      _lastErrorString = "Cannot load report template!";
    }
    return resOk;
  }
  else {
    qDebug() << "No report found id=" << reportId;
    return false;
  }
}

QString BaseJsonReportController::getQueryTemplate(qlonglong reportId)
{
  QString sql = "select sql_template from report where id=%1";
  QString sqlTempl = QSqlQueryHelper::getSingleValue(sql.arg(reportId), _context.uid()).toString();
  return sqlTempl;
}

QString BaseJsonReportController::getDocRootPath()
{
  QString docroot = AppSettings::strVal("docroot", "path", "docroot");
  return AppSettings::settingsPath() + docroot + "/templates/";
}
