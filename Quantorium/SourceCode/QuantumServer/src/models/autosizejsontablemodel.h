#ifndef AUTOSIZEJSONTABLEMODEL_H
#define AUTOSIZEJSONTABLEMODEL_H

#include <QObject>
#include <QAbstractTableModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

class AutosizeJsonTableModel : public QAbstractTableModel
{
public:
  AutosizeJsonTableModel(QObject *parent = Q_NULLPTR);
  void loadData(QJsonArray jArr);
  // QAbstractItemModel interface
public:
  virtual int rowCount(const QModelIndex &parent) const override;
  virtual int columnCount(const QModelIndex &parent) const override;
  virtual QVariant data(const QModelIndex &index, int role) const override;
  virtual QMap<int, QVariant> itemData(const QModelIndex &index) const override;
  virtual QHash<int, QByteArray> roleNames() const override;
private:
  QJsonArray _dataArr;
  QStringList _columns;
  void refreshColumns();  
};

#endif // AUTOSIZEJSONTABLEMODEL_H
