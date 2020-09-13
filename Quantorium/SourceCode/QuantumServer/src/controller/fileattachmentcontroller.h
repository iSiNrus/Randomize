#ifndef FILEATTACHMENTCONTROLLER_H
#define FILEATTACHMENTCONTROLLER_H

#include <QObject>
#include "basejsonsqlcontroller.h"

class FileAttachmentController : public BaseJsonSqlController
{
  Q_OBJECT
  Q_DISABLE_COPY(FileAttachmentController)
public:
  FileAttachmentController(Context &con);
  // HttpRequestHandler interface
public:
  virtual void service(HttpRequest &request, HttpResponse &response) override;
private:
  QString getAttachmentRootPath();
  void saveFile(QString relativePath, const QString &tempFilePath, HttpResponse &response);
  void loadFile(QString filePath, HttpResponse &response);
  void loadDirInfo(QString folder, HttpResponse &response);

  void deleteFileOrDir(QString filePath, HttpResponse &response);
};

#endif // FILEATTACHMENTCONTROLLER_H
