#include "rtftemplater.h"
#include <QDebug>
#include <QDate>
#include <QDateTime>
#include <QStringList>
#include <QString>

#define RTF_PARAGRAPH_START "\\pard"
#define RTF_PARAGRAPH_END "\\par"

#define FORMAT_DATE "dd.MM.yyyy"
#define FORMAT_TIME "hh:mm:ss"
#define FORMAT_SHORT_TIME "hh:mm"
#define FORMAT_DATE_TIME "dd.MM.yyyy hh:mm"

#define TAG_BEGIN_BAND "BAND.BEGIN"
#define TAG_END_BAND "BAND.END"
#define DATASET_PREFIX "data"
#define VAR_DELIMITER "."

#define KEYWORD_ROW "row"
#define ROW_ROLE 1000

#define AGGR_FUNC_MIN "min"
#define AGGR_FUNC_MAX "max"
#define AGGR_FUNC_SUM "sum"
#define AGGR_FUNC_AVG "avg"

//Casting types
#define TYPE_INTEGER "int"
#define TYPE_REAL "real"
#define TYPE_SHORT "short"
#define TYPE_FULL "full"

//Дополнительные функции
#define EMPTY_VALUE ""
#define FUNC_TIME "f_time"
#define FUNC_TODAY "f_today"
#define FUNC_DATETIME "f_datetime"


RtfTemplater::RtfTemplater(QObject *parent) : QObject(parent)
{
  registerValueGenerator(new SimpleVariablesGenerator());
  registerValueGenerator(new ColumnValueGenerator());
  registerValueGenerator(new AggregateValueGenerator());
  //TEST
  registerValueGenerator(new DateTimeValueGenerator());
}

RtfTemplater::~RtfTemplater()
{
  while (!_valueGenerators.isEmpty())
    delete _valueGenerators.takeFirst();
}

void RtfTemplater::registerValueGenerator(AbstractValueGenerator *valueGenerator)
{
  _valueGenerators.append(valueGenerator);
}

bool RtfTemplater::loadTemplate(QString filename)
{
  qDebug() << "Loading template:" << filename;
  _templateDoc = fileToStr(filename, false);
  if (_templateDoc.isEmpty())
    return false;
  //Убираем тег языка из тегов переменных
  _templateDoc.replace(QRegExp("\\[\\\\lang[0-9]+ ([A-Za-z\\.]+)\\]"), "[\\1]");
//  qDebug() << "Template loaded:" << _templateDoc;
  _fontTable.loadFontTable(_templateDoc);
  return true;
}

void RtfTemplater::setDataModel(QAbstractTableModel *dataModel)
{
  _dataModel = dataModel;
  foreach (AbstractValueGenerator* item, _valueGenerators) {
    item->setDataModel(dataModel);
  }
}

void RtfTemplater::setVariables(const QVariantMap &variables)
{
  foreach (AbstractValueGenerator* item, _valueGenerators) {
    item->setVariables(variables);
  }
}

QString RtfTemplater::prepareDoc(QAbstractTableModel* dataModel, const QVariantMap &variables)
{
  setDataModel(dataModel);
  setVariables(variables);
  QString newDoc = processDataBand(_templateDoc);
  newDoc = injectValues(newDoc);
  return newDoc;
}

bool RtfTemplater::prepareAndSaveDoc(QString outFile, QAbstractTableModel *dataModel, const QVariantMap &variables)
{
  QString doc = prepareDoc();
  return strToFile(doc, outFile, false);
}

QString RtfTemplater::injectValues(QString templ, int row)
{
  QRegExp rx("\\[([A-Za-z\\._]+)\\]");
  while (rx.indexIn(templ) >= 0) {
    QString idStr = rx.cap(1);
    QString fontTag = injectFontTag(templ, rx.pos(), 204);
    QString resStr = defVarToStr(injectValue(idStr, row));
    templ = templ.replace(rx.cap(), fontTag + resStr);
  }
  return templ;
}

QVariant RtfTemplater::injectValue(QString idStr, int row)
{
  return injectValue(idStr.split("."), row);
}

QVariant RtfTemplater::injectValue(QStringList idList, int row)
{
  AbstractValueGenerator* generator = 0;
  foreach (AbstractValueGenerator* valGen, _valueGenerators) {
    if (valGen->match(idList)) {
      generator = valGen;
    }
  }
  if (!generator)
    generator = new EmptyValueGenerator();
  return castValue(generator->value(row), idList);
}

QVariant RtfTemplater::castValue(const QVariant &value, const QStringList &idList)
{
  QString type = idList.last();
  if (type == TYPE_INTEGER) {
    return value.toLongLong();
  }
  else if (type == TYPE_REAL) {
    return value.toReal();
  }
  else if (type == TYPE_SHORT && value.type() == QVariant::Time) {
    return value.toTime().toString(FORMAT_SHORT_TIME);
  }
  else {
    return value;
  }
}

QString RtfTemplater::processDataBand(QString templ)
{
  QString paragraphStartTag = RTF_PARAGRAPH_START;
  QString paragraphEndTag = RTF_PARAGRAPH_END;
  QString beginBand = TAG_BEGIN_BAND;
  QString endBand = TAG_END_BAND;
  int pos = templ.indexOf(beginBand);
  if (pos < 0) {
    qWarning() << "Band begin tag not found";
    return templ;
  }
  int beginBandLastPos = templ.indexOf(paragraphEndTag, pos) + paragraphEndTag.length();
  int beginBandFirstPos = templ.lastIndexOf(paragraphStartTag, beginBandLastPos);

  pos = templ.indexOf(endBand);
  if (pos < 0) {
    qWarning() << "Band end tag not found!";
    return templ;
  }
  int endBandLastPos = templ.indexOf(paragraphEndTag, pos) + paragraphEndTag.length();
  int endBandFirstPos = templ.lastIndexOf(paragraphStartTag, endBandLastPos);

  QString bandPattern = templ.mid(beginBandLastPos, endBandFirstPos - beginBandLastPos);
//  qDebug() << "Band pattern:" << bandPattern;
  QString bandBody;
  if (_dataModel) {
    for(int row=0; row<_dataModel->rowCount(); row++) {
      bandBody.append(injectValues(bandPattern, row));
    }
  }

  templ.remove(beginBandFirstPos, endBandLastPos - beginBandFirstPos);
  templ.insert(beginBandFirstPos, bandBody);
  return templ;
}

QString RtfTemplater::cyrillicStr(QString cyrStr)
{
  QString result;
  foreach (QChar chr, cyrStr) {
    result.append(processChar(chr));
  }
  return result;
}

QString RtfTemplater::processChar(const QChar &chr)
{
  ushort ucode = chr.unicode();
  if ((ucode < 1040) || (ucode > 1103))
    return QString(chr);
  ushort code1251 = ucode - 848;
  return "\\'" + QString::number(code1251, 16);
}

QString RtfTemplater::defVarToStr(const QVariant &var)
{
  switch (var.type()) {  
  case QVariant::Int:
  case QVariant::LongLong:
    return " " + var.toString();
  case QVariant::Double:
    return " " + QString::number(var.toReal(), 'f', 2);
  case QVariant::String:
    return " " + cyrillicStr(var.toString());
  case QVariant::Date:
    return " " + var.toDate().toString(FORMAT_DATE);
  case QVariant::DateTime:
    return " " + var.toDateTime().toString(FORMAT_DATE_TIME);
  case QVariant::Time:
    return " " + var.toTime().toString(FORMAT_TIME);
  default:
    return var.toString();
  }
}

int RtfTemplater::roleByName(QAbstractItemModel* dataModel, QString colName)
{
  return dataModel->roleNames().key(colName.toLatin1());
}

QString RtfTemplater::injectFontTag(QString templ, int pos, int charset)
{
  QRegExp fontRx("\\\\f([0-9]+)");
  int prevFontId = -1;
  int valFontId = -1;
  if (fontRx.lastIndexIn(templ, pos) < 0) {
    qWarning() << "No font for injecting value!";
    valFontId = _fontTable.getFontIdByCharset(charset);
  }
  else {
    prevFontId = fontRx.cap(1).toInt();
    valFontId = _fontTable.getFontId(prevFontId, charset);
  }
  QString fontTag = prevFontId == valFontId ? "" : ("\\f" + QString::number(valFontId));
  return fontTag;
}

QString RtfTemplater::fileToStr(QString path, bool isUtf8)
{
  QString resStr = "";
  QFile file(path);
  bool resOk = file.open(QFile::ReadOnly);
  if (!resOk) {
    qWarning() << "Cannot read file:" << path;
    return resStr;
  }
  if (isUtf8)
    resStr = QString::fromLocal8Bit(file.readAll());
  else
    resStr = QString::fromUtf8((file.readAll()));
  file.close();
  return resStr;
}

bool RtfTemplater::strToFile(QString data, QString path, bool isUtf8)
{
  QFile file(path);
  bool resOk = file.open(QFile::WriteOnly);
  if (!resOk)
    return false;
  file.write(isUtf8 ? data.toUtf8() : data.toLocal8Bit());
  file.close();
  return true;
}

FontTable::FontTable()
{
}

void FontTable::loadFontTable(QString rtfDoc)
{
  _fontHash.clear();

  QRegExp rx("\\{\\\\fonttbl\\{(.*)\\}\\}");
  int matchPos = rx.indexIn(rtfDoc);
  if (matchPos < 0) {
    qWarning() << "No font table data found!";
    return;
  }
  QString fontsStr = rx.cap(1);
  rx.setMinimal(true);
  rx.setPattern("f([0-9]+)[\\S]+fcharset([0-9]+) ([A-Za-z\\s]+);");
  int pos = 0;
  while(rx.indexIn(fontsStr, pos) >= 0) {
    appendFontInfo(rx.cap(1).toInt(), rx.cap(3), rx.cap(2).toInt());
    pos = rx.pos() + rx.cap().length();
  }
}

int FontTable::getFontId(int fontId, int charset)
{
  FontInfo info = _fontHash.value(fontId);
  if (info.charset == charset)
    return fontId;
  return getFontId(info.familyName, charset);
}

int FontTable::getFontId(QString familyName, int charset)
{
  foreach (int id, _fontHash.keys()) {
    FontInfo info = _fontHash.value(id);
    if ((info.familyName == familyName) && (info.charset == charset))
      return id;
  }
  return getFontIdByCharset(charset);
}

int FontTable::getFontIdByCharset(int charset)
{
  foreach (int id, _fontHash.keys()) {
    FontInfo info = _fontHash.value(id);
    if (info.charset == charset)
      return id;
  }
  return -1;
}

void FontTable::appendFontInfo(int id, QString familyName, int charset)
{
//  qDebug() << "Font added:" << id << familyName << charset;
  FontInfo info;
  info.familyName = familyName;
  info.charset = charset;
  _fontHash.insert(id, info);
}

AbstractValueGenerator::AbstractValueGenerator()
{
}

void AbstractValueGenerator::setDataModel(QAbstractTableModel *dataModel)
{
  _dataModel = dataModel;
}

void AbstractValueGenerator::setVariables(QVariantMap variables)
{
  _variables = variables;
}

EmptyValueGenerator::EmptyValueGenerator() : AbstractValueGenerator()
{
}


QVariant EmptyValueGenerator::value(int row)
{
  Q_UNUSED(row)
  qWarning() << "Empty value generator warning!";
  return "";
}

bool EmptyValueGenerator::match(const QStringList &keys)
{
  Q_UNUSED(keys)
  return true;
}

ColumnValueGenerator::ColumnValueGenerator()
{

}

QVariant ColumnValueGenerator::value(int row)
{
  if (_role == ROW_ROLE) {
    return row + 1;
  }
  QModelIndex idx = _dataModel->index(row, 0);
  if (idx.isValid()) {
    return idx.data(_role);
  }
  else if (_dataModel->rowCount() > 0) {
    return _dataModel->index(0, 0).data(_role);
  }
  else {
    return EMPTY_VALUE;
  }
}

bool ColumnValueGenerator::match(const QStringList &keys)
{
  if (keys.at(0) != "data")
    return false;
  if (keys.count() < 2) {
    qWarning() << "Incorrect dataset column identifier";
    return false;
  }

  QString colName = keys.at(1);

  if (colName == KEYWORD_ROW) {
    _role = ROW_ROLE;
    return true;
  }

  _role = _dataModel->roleNames().key(colName.toLatin1(), -1);
  if (_role < 0) {
    qWarning() << "Unknown dataset column";
    return false;
  }
  return true;
}


SimpleVariablesGenerator::SimpleVariablesGenerator() : AbstractValueGenerator()
{
}

bool SimpleVariablesGenerator::match(const QStringList &keys)
{
  if (_variables.contains(keys.first())) {
    _variableName = keys.first();
    return true;
  }
  else {
    return false;
  }
}

QVariant SimpleVariablesGenerator::value(int row)
{
  Q_UNUSED(row);
  return _variables.value(_variableName);
}


AggregateValueGenerator::AggregateValueGenerator() : AbstractValueGenerator()
{
  _availableFunctions << AGGR_FUNC_MIN << AGGR_FUNC_MAX << AGGR_FUNC_SUM << AGGR_FUNC_AVG;
}

bool AggregateValueGenerator::match(const QStringList &keys)
{
  if (!_availableFunctions.contains(keys.at(0)))
    return false;
  _funcName = keys.at(0);

  if (keys.count() < 2) {
    qWarning() << "Incorrect dataset column identifier";
    return false;
  }
  QString colName = keys.at(1);
  qDebug() << "Colname:" << colName;
  _role = _dataModel->roleNames().key(colName.toLatin1(), -1);
  qDebug() << "Colname role:" << _role;
  return _role >= 0;
}

QVariant AggregateValueGenerator::value(int row)
{
  Q_UNUSED(row)

  qreal resValue = 0;
  for(int row=0; row<_dataModel->rowCount(); row++) {
    if (_funcName == AGGR_FUNC_MIN) {
      if (row == 0)
        resValue = _dataModel->index(row, 0).data(_role).toReal();
      else
        resValue = qMin(resValue, _dataModel->index(row, 0).data(_role).toReal());
    }
    else if (_funcName == AGGR_FUNC_MAX) {
      if (row == 0)
        resValue = _dataModel->index(row, 0).data(_role).toReal();
      else
        resValue = qMax(resValue, _dataModel->index(row, 0).data(_role).toReal());
    }
    else if (_funcName == AGGR_FUNC_SUM || _funcName == AGGR_FUNC_AVG) {
      resValue += _dataModel->index(row, 0).data(_role).toReal();
    }
  }
  if (_funcName == AGGR_FUNC_AVG) {
    resValue = resValue / _dataModel->rowCount();
  }
  return resValue;
}


DateTimeValueGenerator::DateTimeValueGenerator() : AbstractValueGenerator()
{
  _keywords << FUNC_TIME << FUNC_TODAY << FUNC_DATETIME;
}

bool DateTimeValueGenerator::match(const QStringList &keys)
{
  if (_keywords.contains(keys.first())) {
    _currentFunction = keys.first();
    return true;
  }
  return false;
}

QVariant DateTimeValueGenerator::value(int row)
{
  Q_UNUSED(row)
  if (_currentFunction == FUNC_TIME) {
    return QTime::currentTime();
  }
  else if (_currentFunction == FUNC_TODAY) {
    return QDate::currentDate();
  }
  else if (_currentFunction == FUNC_DATETIME) {
    return QDateTime::currentDateTime();
  }
  return EMPTY_VALUE;
}
