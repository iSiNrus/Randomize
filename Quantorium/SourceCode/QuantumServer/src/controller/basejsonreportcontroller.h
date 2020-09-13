#ifndef BASEJSONREPORTCONTROLLER_H
#define BASEJSONREPORTCONTROLLER_H

#include <QObject>
#include "basejsonsqlcontroller.h"
#include "../core/rtftemplater.h"
#include "../models/autosizejsontablemodel.h"
#include "../core/appsettings.h"

class BaseJsonReportController : public BaseJsonSqlController
{
  Q_OBJECT
  Q_DISABLE_COPY(BaseJsonReportController)
public:
  BaseJsonReportController(Context &con);

  // HttpRequestHandler interface
public:
  virtual void service(HttpRequest &request, HttpResponse &response) override;
private:
  QString _lastErrorString;
  RtfTemplater* _templater;
  AutosizeJsonTableModel* _dataModel;
  void reportList(HttpResponse &response);
  bool initReportModel(QString queryTemplate);
  bool initReportData(qlonglong reportId);
  QString getQueryTemplate(qlonglong reportId);
  QString getDocRootPath();
};

#endif // BASEJSONREPORTCONTROLLER_H
