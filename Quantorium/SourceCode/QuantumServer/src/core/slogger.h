#ifndef SLOGGER_H
#define SLOGGER_H

#include <QString>
#include <QtGlobal>
#include <QDebug>

#define FATAL qFatal() << __FILE__ << "-" << __FUNCTION__ << "(" << __LINE__ << ") |"
#define CRITICAL qCritical().noquote() << __FILE__ << "-" << __FUNCTION__ << "(" << __LINE__ << ") |"
#define WARNING qWarning().noquote() << __FILE__ << "-" << __FUNCTION__ << "(" << __LINE__ << ") |"
#define LOG qDebug().noquote() << __FILE__ << "-" << __FUNCTION__ << "(" << __LINE__ << ") |"
#define INFO qDebug().noquote() << __FILE__ << "-" << __FUNCTION__ << "(" << __LINE__ << ") |"

class SLogger
{
public:         
    SLogger();

    static void logOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    void start();

    static bool logDebug() {return _logDebug;}
    static bool logWarning() {return _logWarning;}
    static bool logCritical() {return _logCritical;}
    static bool logFatal() {return _logFatal;}
    static bool logInfo() {return _logInfo;}
    static bool logEventTime() {return _logEventTime;}

    static QString logFile() {return _logFile;}
    static QString logDir() {return _logDir;}

    static QString filter() {return _filter;}

    void setLogDebug(bool value) {_logDebug = value;}
    void setLogWarning(bool value) {_logWarning = value;}
    void setLogCritical(bool value) {_logCritical = value;}
    void setLogFatal(bool value) {_logFatal = value;}
    void setLogInfo(bool value) {_logInfo = value;}
    void setLogStart(bool value) {_logStart = value;}
    void setLogEventTime(bool value) {_logEventTime = value;}

    void setLogToFile(const QString &fileName);

    void setFilter(const QString &value);

    static QString fullLogFileName();
    static QString currentLogFileName();

private:
    static bool _logInfo;
    static bool _logStart;
    static bool _logDebug;
    static bool _logWarning;
    static bool _logCritical;
    static bool _logFatal;
    static bool _logEventTime;
    static QString _logFile;
    static QString _logDir;
    static QString _filter;

    static void logToFile(const QString &msg);
    static void logToStdOut(const QString &msg);
    static void logToStdErr(const QString &msg);
    static QString createLogStr(QtMsgType type, const QString &msg);
};

#endif // SLOGGER_H
