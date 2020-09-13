#ifndef FILEACTIONS_H
#define FILEACTIONS_H

#include <QObject>
#include <QDir>
#include <QFileInfo>

class FileActions : public QObject
{
    Q_OBJECT
public:
    explicit FileActions(QObject *parent = 0);

    static void recurseScanDir(QDir dir, QList<QFileInfo> & list, const QString &filterSuffix = "");
    static QString removeBaseFromFilePath(const QString &filePath, const QString &base);
    static bool copyFile(const QString &fileSource, const QString &fileDest, bool deleteIfExist = true, bool setFullPermissions = true, bool dontCopySameSize = false);
    static QString calcMd5(const QString &fileName);
    static QString createFilePath(const QString &dirName, const QString &fileName);
    static QString extractFileName( const QString &fileName);
    static QString extractFileExt( const QString &fileName);
    static bool mkPath(const QString &pathName);
    static QString getConfigPath(const QString &organizationName, const QString &applicationCodeName);

    static void extractResFile(const QString &fileName, const QString &extractToFileName, bool setPermissionToWrite = false);

    static void deleteFile(const QString &fileName);

    qint64 calcDirSize(const QString &dir);

    void copyDir(const QString &dirSource, const QString &dirDest, bool abortIfError = false, bool canCancelled = true, bool dontCopySameSize = false);
    void deleteDir(const QString &dirSource, bool deleteDirSource = false);

signals:
    void calculatedDirSize(qint64 size);
    void copyDirProgress(const QString &fileName, int number, int count);
    void copyDirFinished();
    void copyDirAborted();

    void deleteDirProgress(const QString &fileName, int number, int count);
    void deleteDirFinished();
    void deleteDirAborted();
    
public slots:
    
};

#endif // FILEACTIONS_H
