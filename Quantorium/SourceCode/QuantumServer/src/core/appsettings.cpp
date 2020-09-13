#include "appsettings.h"
#include <QCoreApplication>
#include <QDebug>

QSettings* AppSettings::_settings = 0;
QString AppSettings::_filePath = "";

QVariant AppSettings::val(QString key, QVariant defValue)
{
  if (!_settings)
    init();
  QVariant res = _settings->value(key, defValue);
  qDebug() << "Load:" << key << res;
  return res;
}

QVariant AppSettings::val(QString section, QString param, QVariant defValue)
{
  return val(section + "/" + param, defValue);
}

void AppSettings::setVal(QString key, QVariant val)
{
  if (!_settings)
    init();
  _settings->setValue(key, val);
  qDebug() << "Save:" << key << val;
}

void AppSettings::setVal(QString section, QString param, QVariant val)
{
  setVal(section + "/" + param, val);
}

QString AppSettings::strVal(QString section, QString name, QVariant defValue)
{
    QString fullName = name;
    if (!section.isEmpty())
        fullName = fullName.prepend(section + "/");
    return val(fullName, defValue).toString();
}

int AppSettings::intVal(QString section, QString name, QVariant defValue)
{
    QString fullName = name;
    if (!section.isEmpty())
        fullName = fullName.prepend(section + "/");
    return val(fullName, defValue).toInt();
}

bool AppSettings::boolVal(QString section, QString name, QVariant defValue)
{
    QString fullName = name;
    if (!section.isEmpty())
        fullName = fullName.prepend(section + "/");
    return val(fullName, defValue).toBool();
}

void AppSettings::remove(const QString &key)
{
  if (!_settings)
    init();
  _settings->remove(key);
}

QString AppSettings::applicationPath()
{
  return QCoreApplication::applicationDirPath() + "/";
}

QString AppSettings::applicationName()
{
  return QCoreApplication::applicationName();
}

QString AppSettings::settingsPath()
{
  if (!_settings)
    init();
  return _settings->fileName().section("/", 0, -2) + "/";
}

void AppSettings::sync()
{
    if (!_settings)
      init();
    _settings->sync();
}

void AppSettings::init()
{
  if (_filePath.isEmpty())
    setSettingsFile(applicationPath() + applicationName() + ".ini");
  qDebug() << "Loading settings: " + _filePath;
  _settings = new QSettings(_filePath, QSettings::IniFormat);
}

AppSettings::AppSettings(){}

void AppSettings::setSettingsFile(QString filePath)
{
  _filePath = filePath;
}

bool AppSettings::contains(QString section, QString name)
{
    if (!_settings)
      init();
    QString fullName = name;
    if (!section.isEmpty())
        fullName.prepend(section + "/");
    return _settings->contains(fullName);
}
