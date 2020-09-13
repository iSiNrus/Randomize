#include "slogger.h"
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <stdio.h>
#include <QMutex>
#include <QFileInfo>
#include <QRegExp>

QMutex mutex;

bool SLogger::_logDebug = true;
bool SLogger::_logWarning = true;
bool SLogger::_logCritical = true;
bool SLogger::_logFatal = true;
bool SLogger::_logInfo = true;
bool SLogger::_logStart = false;
bool SLogger::_logEventTime = true;
QString SLogger::_logFile = "";
QString SLogger::_logDir = "";
QString SLogger::_filter = "";

SLogger::SLogger()
{
}

void SLogger::logOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    mutex.lock();

    QString logStr = createLogStr(type, msg);

    if (!_filter.isEmpty()) {
        QRegExp rx(_filter);
        if (rx.indexIn(logStr)<0) {
            mutex.unlock();
            return;
        }
    }

    if (logStr.isEmpty()) {
        mutex.unlock();
        return;
    }

    if (!_logFile.isEmpty())
        logToFile(logStr);

    QChar mtype = logStr.at(0);
    if ((mtype == 'I') || (mtype == 'D'))
        logToStdOut(logStr);
    else
        logToStdErr(logStr);
    mutex.unlock();
}

void SLogger::setLogToFile(const QString &value)
{
    QFileInfo fileInfo(value);    
    _logFile = fileInfo.fileName();
    _logDir = fileInfo.path();
}

void SLogger::setFilter(const QString &value)
{
    _filter = value;
}

QString SLogger::currentLogFileName()
{
   return QDateTime::currentDateTime().toString("yyyy-MM-dd-") + _logFile;
}

QString SLogger::fullLogFileName()
{
   return _logDir + "/" + currentLogFileName();
}

void SLogger::logToFile(const QString &msg)
{
    if (msg.isEmpty())
        return;
    QFile file;
    QString logFileName = fullLogFileName();
    file.setFileName(logFileName);
    if (file.open(QIODevice::Append)) {
        file.write(msg.toLocal8Bit());
        file.close();
    }
}

void SLogger::logToStdOut(const QString &msg)
{
    if (msg.isEmpty())
        return;

    QFile file;
    if (file.open(stdout, QIODevice::WriteOnly)) {
        file.write(msg.toLocal8Bit());
        file.close();
    }
}

void SLogger::logToStdErr(const QString &msg)
{
    if (msg.isEmpty())
        return;

    QFile file;
    if (file.open(stderr, QIODevice::WriteOnly)) {
        file.write(msg.toLocal8Bit());
        file.close();
    }
}

void SLogger::start()
{
    bool tmp = _logDebug;
    _logDebug = true;
    qInstallMessageHandler(logOutput);
    qDebug() << "";
    qDebug() << "";
    qDebug() << "";
    qDebug() << "Build:" << __TIMESTAMP__;
    qDebug() << "Log started:" << QDateTime::currentDateTime().toString("yyyy-dd-MM");

    if (!_logFile.isEmpty()) {
        qDebug() << "Write to file:" << fullLogFileName();
    }

    qDebug() << "==========================================================================================";
    _logDebug = tmp;
}

QString SLogger::createLogStr(QtMsgType type, const QString &msg)
{
    QString dateStamp = "";
    QString logStr = "";

    if (_logEventTime)
        dateStamp = QDateTime::currentDateTime().toString("hh:mm:ss:zzz ");

    switch (type) {
    case QtDebugMsg:
    {
        QString s = QString(msg).left(6);
        if (s.left(2) == "I:") {
            if (_logInfo)
                logStr = QString("I %1| %2").arg(dateStamp).arg(QString(msg));
        } else if (_logDebug)
            logStr = QString("D %1| %2").arg(dateStamp).arg(QString(msg));
        break;
    }
    case QtInfoMsg:
    {
        if (_logInfo)
            logStr = QString("I %1| %2").arg(dateStamp).arg(QString(msg));
        break;
    }
    case QtWarningMsg:
        if (_logWarning)
            logStr = QString("W %1| %2").arg(dateStamp).arg(QString(msg));
        break;
    case QtCriticalMsg:
        if (_logCritical)
            logStr = QString("C %1| %2").arg(dateStamp).arg(QString(msg));
        break;
    case QtFatalMsg:
        if (_logFatal)
            logStr = QString("F %1| %2").arg(dateStamp).arg(QString(msg));
        break;
    default: ;
    }

    if (logStr.isEmpty())
        return "";

//    logStr = logStr.replace("\n", " \\n ");
//    logStr = logStr.replace("\r", " \\r ");
//    logStr = logStr.replace("\t", " \\t ");
    return logStr+"\r\n";
}




