#include "autosizejsontablemodel.h"

AutosizeJsonTableModel::AutosizeJsonTableModel(QObject *parent) : QAbstractTableModel(parent)
{
}

void AutosizeJsonTableModel::loadData(QJsonArray jArr)
{
  emit beginResetModel();
  _dataArr = jArr;
  refreshColumns();
  emit endResetModel();
}


int AutosizeJsonTableModel::rowCount(const QModelIndex &parent) const
{
  return _dataArr.count();
}

int AutosizeJsonTableModel::columnCount(const QModelIndex &parent = QModelIndex()) const
{
  return _columns.count();
}

QVariant AutosizeJsonTableModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();
  if (role == Qt::DisplayRole) {
    return _dataArr.at(index.row()).toObject().value(_columns.at(index.column())).toVariant();
  }
  else if (role >= Qt::UserRole) {
    int col = role - Qt::UserRole;
    return _dataArr.at(index.row()).toObject().value(_columns.at(col)).toVariant();
  }
  return QVariant();
}

void AutosizeJsonTableModel::refreshColumns()
{
  _columns.clear();
  if (_dataArr.isEmpty())
    return;
  QJsonObject rowObj = _dataArr.first().toObject();
  _columns = rowObj.keys();
}


QMap<int, QVariant> AutosizeJsonTableModel::itemData(const QModelIndex &index) const
{
    QMap<int, QVariant> result = QAbstractTableModel::itemData(index);
    for(int col=0; col<columnCount(); col++) {
        int colRole = Qt::UserRole + col;
        result.insert(colRole, index.data(colRole));
    }
    return result;
}


QHash<int, QByteArray> AutosizeJsonTableModel::roleNames() const
{
    QHash<int, QByteArray> result = QAbstractTableModel::roleNames();
    for(int col=0; col<_columns.count(); col++) {
        result.insert(Qt::UserRole + col, qPrintable(_columns.at(col)));
    }
    return result;
}
