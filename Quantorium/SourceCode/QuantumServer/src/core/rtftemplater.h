#ifndef RTFTEMPLATER_H
#define RTFTEMPLATER_H

#include <QObject>
#include <QFile>
#include <QAbstractTableModel>

#define DEFAULT_FONT "Times New Roman"

//Информация о шрифте документа
struct FontInfo
{
public:
  QString familyName = DEFAULT_FONT;
  int charset = 0;
};

//Таблица шрифтов документа
class FontTable
{
public:
  FontTable();
  void loadFontTable(QString rtfDoc);
  int getFontId(int fontId, int charset);
  int getFontId(QString familyName, int charset);
  int getFontIdByCharset(int charset);
private:
  void appendFontInfo(int id, QString familyName = DEFAULT_FONT, int charset = 0);
  QHash<int, FontInfo> _fontHash;
};

//Абстрактный класс генератора значений шаблона
class AbstractValueGenerator
{
public:
  AbstractValueGenerator();
  void setDataModel(QAbstractTableModel* dataModel);
  void setVariables(QVariantMap variables);
  virtual bool match(const QStringList &keys) = 0;
  virtual QVariant value(int row = -1) = 0;
protected:
  QVariantMap _variables;
  QAbstractTableModel* _dataModel;
};

//Генератор-заглушка
class EmptyValueGenerator : public AbstractValueGenerator
{
public:
  EmptyValueGenerator();
  virtual QVariant value(int row) override;
  virtual bool match(const QStringList &keys) override;
};

//Внедрение значений модели
class ColumnValueGenerator : public AbstractValueGenerator
{
public:
  ColumnValueGenerator();
  // AbstractValueGenerator interface
public:
  virtual QVariant value(int row) override;
  virtual bool match(const QStringList &keys) override;
private:
  int _role = -1;
};

//Генератор для внедрения обычных параметров
class SimpleVariablesGenerator : public AbstractValueGenerator
{
public:
  SimpleVariablesGenerator();
  // AbstractValueGenerator interface
public:
  virtual bool match(const QStringList &keys) override;
  virtual QVariant value(int row) override;
private:
  QString _variableName;
};

//Генератор для внедрения агрегатных значений
class AggregateValueGenerator : public AbstractValueGenerator
{
public:
  AggregateValueGenerator();
  // AbstractValueGenerator interface
public:
  virtual bool match(const QStringList &keys) override;
  virtual QVariant value(int row) override;
private:
  QStringList _availableFunctions;
  QString _funcName = "";
  int _role = -1;
};

//Функции связанные с датой/временем
class DateTimeValueGenerator : public AbstractValueGenerator
{
public:
  DateTimeValueGenerator();
  // AbstractValueGenerator interface
public:
  virtual bool match(const QStringList &keys) override;
  virtual QVariant value(int row) override;
private:
  QStringList _keywords;
  QString _currentFunction;
};

//Класс шаблонизатора для RTF-документов
class RtfTemplater : public QObject
{
  Q_OBJECT
public:
  explicit RtfTemplater(QObject *parent = Q_NULLPTR);
  virtual ~RtfTemplater();
  void registerValueGenerator(AbstractValueGenerator* valueGenerator);
  bool loadTemplate(QString filename);
  QString prepareDoc(QAbstractTableModel* dataModel = Q_NULLPTR, const QVariantMap &variables = QVariantMap());
  bool prepareAndSaveDoc(QString outFile, QAbstractTableModel* dataModel = Q_NULLPTR, const QVariantMap &variables = QVariantMap());
private:
  QList<AbstractValueGenerator*> _valueGenerators;
  QAbstractTableModel* _dataModel;
  QString _templateDoc;
  FontTable _fontTable;
  void setDataModel(QAbstractTableModel* dataModel = Q_NULLPTR);
  void setVariables(const QVariantMap &variables);
  //Внедрение значений в шаблон
  QString injectValues(QString templ, int row = -1);
  //Внедрение значения по идентификатору
  QVariant injectValue(QString idStr, int row = -1);
  QVariant injectValue(QStringList idList, int row = -1);
  //Преобразование типа значения
  QVariant castValue(const QVariant &value, const QStringList &idList);
  //Заполнение данных модели по строкам
  QString processDataBand(QString templ);
  QString cyrillicStr(QString cyrStr);
  QString processChar(const QChar &chr);
  //Преобразование QVariant к строке
  QString defVarToStr(const QVariant &var);
  int roleByName(QAbstractItemModel* dataModel, QString colName);
  //Вставка правильного тега шрифта перед вставляемым значением
  QString injectFontTag(QString templ, int pos, int charset);

  QString fileToStr(QString path, bool isUtf8 = false);
  bool strToFile(QString data, QString path, bool isUtf8 = false);

};

#endif // RTFTEMPLATER_H
