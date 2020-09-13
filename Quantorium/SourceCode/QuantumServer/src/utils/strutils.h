#ifndef STRUTILS_H
#define STRUTILS_H

#include <QString>
#include <QMap>
#include <QHash>
#include "math.h"

class StrUtils
{
public:
    static QString fillValues(QString templ, QMap<QString, QString> valuesMap);
    static QString intToBitmask(int intVal);
    static QList<qlonglong> longToList(qlonglong longVal);
    static QString replaceTag(QString inStr, QString tagName, QString templ);
    static QHash<QString, QString> attrsToHash(QString attrs);
    static QStringList dashValues(QString inStr);
    static QString uuid();
private:
    StrUtils();
};

bool lessThanByLength(QString &str1, QString &str2);
bool greaterThanByLength(QString &str1, QString &str2);

#endif // STRUTILS_H
