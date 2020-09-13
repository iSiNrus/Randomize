#include "fileactions.h"
#include <QFile>
#include <QDebug>
#include <QCryptographicHash>
#include <QSettings>

FileActions::FileActions(QObject *parent) :
  QObject(parent)
{
}

void FileActions::recurseScanDir(QDir dir, QList<QFileInfo> &list, const QString &filterSuffix)
{
  QStringList qsl = dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden);

  foreach (QString file, qsl) {
    QFileInfo finfo(createFilePath(dir.path(), file));

    if (finfo.isSymLink())
      return;

    if (finfo.suffix() != filterSuffix)
      list.prepend(finfo);

    if (finfo.isDir()) {
      QDir nextDir(finfo.filePath());
      recurseScanDir(nextDir, list);
    }
  }
}

QString FileActions::removeBaseFromFilePath(const QString &filePath, const QString &base)
{
  QString result = filePath;
  if (result.startsWith(base, Qt::CaseInsensitive)) {
    result = result.right(result.size() - (base.size()+1));
  }
  return result;
}

bool FileActions::copyFile(const QString &fileSource, const QString &fileDest, bool deleteIfExist, bool setFullPermissions, bool dontCopySameSize)
{
  QDir dir;
  dir.mkpath(QFileInfo(fileDest).absolutePath());
  if ( (QFile::exists(fileDest)) && deleteIfExist )
    QFile::remove(fileDest);


  bool result = false;

  QFileInfo fileInfoSource(fileSource);
  QFileInfo fileInfoDest(fileDest);

  if (( (dontCopySameSize) && (fileInfoSource.size() != fileInfoDest.size()) ) || (!dontCopySameSize) )
    result = QFile::copy(fileSource, fileDest);

  if (result && setFullPermissions)
    QFile::setPermissions(fileDest, QFile::WriteOwner|QFile::WriteUser|QFile::WriteGroup|QFile::WriteOther );

  return result;
}

void FileActions::copyDir(const QString &dirSource, const QString &dirDest, bool abortIfError, bool canCancelled,  bool dontCopySameSize)
{
  QDir dir;
  dir.mkpath(dirDest);

  QList<QFileInfo> list;
  recurseScanDir(QDir(dirSource), list);

  int counter = 0;
  foreach (QFileInfo file, list) {
    counter++;
    QString fileName = createFilePath(dirDest, removeBaseFromFilePath(file.filePath(), dirSource));
    if (file.isFile()) {
      if ( ( !copyFile(file.filePath(), fileName, true, true, dontCopySameSize)) && (abortIfError) ) {
        emit copyDirAborted();
        return;
      }
    } else if (file.isDir())
      dir.mkpath(fileName);
    emit copyDirProgress(file.filePath(), counter, list.count());
  }
  emit copyDirFinished();
}

void FileActions::deleteDir(const QString &dirSource, bool deleteDirSource)
{
  QDir dir;
  QList<QFileInfo> list;
  recurseScanDir(QDir(dirSource), list);
  int counter = 0;

  foreach (QFileInfo file, list) {
    counter++;
    if (file.isFile()) {
      QFile::setPermissions( file.filePath(), QFile::WriteOwner|QFile::WriteUser|QFile::WriteGroup|QFile::WriteOther );
      QFile::remove(file.filePath());
    }
    emit deleteDirProgress(file.filePath(), counter, list.count());
  }

  foreach (QFileInfo file, list) {
    counter++;
    if (file.isDir()) {
      dir.rmdir(file.filePath());
    }
    emit deleteDirProgress(file.filePath(), counter, list.count());
  }

  if (deleteDirSource)
    dir.rmpath(dirSource);

  emit deleteDirFinished();
}

QString FileActions::calcMd5(const QString &fileName)
{
  QString result;
  QFile file(fileName);
  if (file.open(QIODevice::ReadOnly)) {
    result = QCryptographicHash::hash(file.readAll()
                                      , QCryptographicHash::Md5).toHex().toLower();
  }
  return result;
}

QString FileActions::createFilePath(const QString &dirName, const QString &fileName)
{
  QString result;
  QString dir = dirName;
  QString file = fileName;

  result = dir.append(QDir::separator()).append(file).replace("\\", "/");

  while (result.contains("//"))
    result = result.replace("//", "/");

  return result;
}

QString FileActions::extractFileName(const QString &fileName)
{
  QFileInfo f(fileName);
  return f.fileName();
}

QString FileActions::extractFileExt(const QString &fileName)
{
  QFileInfo f(fileName);
  return f.suffix();
}

bool FileActions::mkPath(const QString &pathName)
{
  QDir d;
  return d.mkpath(pathName);
}

QString FileActions::getConfigPath(const QString &organizationName, const QString &applicationCodeName)
{
  QString result;

  QSettings settings( QSettings::IniFormat, QSettings::SystemScope, organizationName, applicationCodeName );

  QString fileName = settings.fileName();
  if( !fileName.isEmpty() )
    result = QFileInfo( fileName ).dir().path() + "/" + applicationCodeName;

  return result;
}

void FileActions::deleteFile(const QString &fileName)
{
  QFile::setPermissions(fileName, QFile::WriteOwner|QFile::WriteUser|QFile::WriteGroup|QFile::WriteOther );
  QFile::remove(fileName);
}

qint64 FileActions::calcDirSize(const QString &dir)
{
  QList<QFileInfo> list;
  qint64 result = 0;

  recurseScanDir(QDir(dir), list);

  foreach (QFileInfo file, list) {
    if (file.isFile()) {
      result += file.size();
      emit calculatedDirSize(result);
    }
  }

  return result;
}

void FileActions::extractResFile(const QString &fileName, const QString &extractToFileName, bool setPermissionToWrite)
{
  QFile resfile(fileName);
  resfile.open(QIODevice::ReadWrite);
  resfile.copy(fileName, extractToFileName); // сохраняем
  resfile.close();

  if (setPermissionToWrite)
    QFile::setPermissions(extractToFileName, QFile::ReadOwner|QFile::WriteOwner|QFile::ReadUser|QFile::WriteUser );
}
