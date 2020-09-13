#include "fileattachmentcontroller.h"
#include "../core/appconst.h"
#include "../core/api.h"
#include "../utils/qsqlqueryhelper.h"
#include "../utils/qfileutils.h"
#include "../core/appsettings.h"
#include <QJsonArray>
#include <QDir>
#include <QUrl>
#include <QDebug>


FileAttachmentController::FileAttachmentController(Context &con) : BaseJsonSqlController(con)
{

}

void FileAttachmentController::service(HttpRequest &request, HttpResponse &response)
{
  BaseJsonSqlController::service(request, response);
  if (response.getStatusCode() != 200)
    return;
  QString method = paramString(PRM_HTTP_METHOD);
  QString path = paramString(PRM_PATH);
  QString filepath = path.section(DELIMITER CTRL_ATTACHMENTS DELIMITER, 1, 1);
  if (method == HTTP_POST) {
    QTemporaryFile* tempFile = request.getUploadedFile(QString(REQUEST_FILE_ID).toLatin1());
    tempFile->open();
    QString tempFilePath = tempFile->fileName();
    tempFile->close();
    saveFile(filepath, tempFilePath, response);
  }
  else if (method == HTTP_GET) {
    bool hasExtension = (filepath.indexOf('.') >= 0);

    if (hasExtension)
      loadFile(filepath, response);
    else
      loadDirInfo(filepath, response);
  }
  else if (method == HTTP_DELETE) {
    deleteFileOrDir(filepath, response);
  }
  else {
    writeError(response, "Unsupported HTTP method");
  }
}

QString FileAttachmentController::getAttachmentRootPath()
{
  QString docroot = AppSettings::strVal("docroot", "path", "docroot");
  return AppSettings::settingsPath() + docroot + DELIMITER CTRL_ATTACHMENTS DELIMITER;
}

void FileAttachmentController::saveFile(QString relativePath, const QString &tempFilePath, HttpResponse &response)
{
  QString newFilePath = getAttachmentRootPath() + relativePath; 

  qDebug() << "!!!" << getAttachmentRootPath() << relativePath << QFilePath::folderPath(relativePath);
  QDir dir(getAttachmentRootPath());
  dir.mkpath(QFilePath::folderPath(relativePath));
  qDebug() << "Copying" << tempFilePath << "to" << newFilePath;
  QFile::remove(newFilePath);
  bool resOk = QFile::copy(tempFilePath, newFilePath);
  if (resOk) {
    writeResultOk(response, "Файл сохранен");
  }
  else {
    writeError(response, "Ошибка при загрузке файла");
  }
}

void FileAttachmentController::loadFile(QString filePath, HttpResponse &response)
{  
  QString absolutePath = getAttachmentRootPath() + filePath;
  if (QFile::exists(absolutePath)) {
    QString mime = QFileUtils::mimeFromExtension(filePath);
    response.setHeader("Content-Type", mime.toLatin1());
    QFile file(absolutePath);
    file.open(QFile::ReadOnly);
    response.setHeader("Content-length", file.size());
    response.write(file.readAll());
    file.close();
  }
  else {
    writeError(response, "File not found: " + absolutePath, 404);
  }
}

void FileAttachmentController::loadDirInfo(QString folder, HttpResponse &response)
{
  QString folderPath = getAttachmentRootPath() + folder;
  QDir dir(folderPath);
  QFileInfoList infoList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
  QJsonArray jArr;
  foreach (QFileInfo fileInfo, infoList) {
    QJsonObject jObj;
    jObj.insert("filename", fileInfo.fileName());
    jObj.insert("extension", fileInfo.suffix());
    jArr.append(jObj);
  }
  writeResultOk(response, "Список файлов в папке", jArr);
}

void FileAttachmentController::deleteFileOrDir(QString filePath, HttpResponse &response)
{
  QUrl url(getAttachmentRootPath() + filePath);
  bool isDir = filePath.indexOf(".") < 0;
  QDir dir(isDir ? url.toString() : url.toString(QUrl::RemoveFilename));
  bool resOk = false;
  if (isDir) {
    qDebug() << "Deleting dir:" << dir.absolutePath();
    resOk = dir.removeRecursively();
  }
  else {
    qDebug() << "Deleting file:" << dir.absoluteFilePath(url.fileName());
    resOk = dir.remove(url.fileName());
  }
  if (resOk)
    writeResultOk(response, "Файл или папка успешно удалены");
  else
    writeError(response, "Не удалось удалить файл или папку");
}
