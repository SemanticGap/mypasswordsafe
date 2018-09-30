#include <QString>
#include <QSize>
#include <QIcon>
#include <assert.h>
#include "myutil.hpp"
#include "safetreemodel.hpp"

SafeTreeModel::SafeTreeModel(Safe *safe, QObject *parent)
	: QAbstractItemModel(parent), m_safe(safe)
{
	connect(m_safe, SIGNAL(itemAdded(SafeItem *, SafeGroup *)), this, SLOT(itemAdded(SafeItem *, SafeGroup *)));
	connect(m_safe, SIGNAL(itemPreAdd(SafeItem *, SafeGroup *)), this, SLOT(itemPreAdd(SafeItem *, SafeGroup *)));
	connect(m_safe, SIGNAL(itemChanged(SafeItem *)), this, SLOT(itemChanged(SafeItem *)));
}

SafeTreeModel::~SafeTreeModel()
{
}

Qt::ItemFlags SafeTreeModel::flags(const QModelIndex &index) const
{
	if(index.isValid())
		//return QAbstractItemModel::flags(index);
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	else
		return 0;
}

QVariant SafeTreeModel::data(const QModelIndex &index, int role) const
{
	if(role == Qt::SizeHintRole)
		return QVariant(QSize(100, 20));

	if(!index.isValid())
		return QVariant();

	SafeItem *i = (SafeItem *)index.internalPointer();

	if(role == Qt::DecorationRole) {
		switch(i->rtti()) {
		case SafeEntry::RTTI:
			return QIcon(":/images/password.png");
		case SafeGroup::RTTI:
			return QIcon(":/images/folder_blue.png");
		default:
			break;
		}		
	} else if(role == Qt::DisplayRole) {
		if(i->rtti() == SafeEntry::RTTI) {
			SafeEntry *e = (SafeEntry *)i;
			//DBGOUT("data entry " << role << " " << e->name().toStdString() << " " << index.row() << " " << index.column());
			if(index.column() == 0) {
				return QVariant(e->name());
			}
		} else if(i->rtti() == SafeGroup::RTTI) {
			SafeGroup *e = (SafeGroup *)i;
			//DBGOUT("data group " << role << " " << e->name().toStdString() << " " << index.row() << " " << index.column());
			if(index.column() == 0) {
				return QVariant(e->name());
			}
		}
	}

	return QVariant();
}

QVariant SafeTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role != Qt::DisplayRole) {
		return QVariant();
	}

	//DBGOUT("Header " << orientation << " " << section);

	if(orientation == Qt::Horizontal) {
		if(section == 0) {
			return QVariant(tr("Name"));
		}
	}

	return QVariant();
}

int SafeTreeModel::rowCount(const QModelIndex &parent) const
{
	if(parent.isValid()) {
		SafeGroup *item = (SafeGroup *)parent.internalPointer();
		if(item->rtti() == SafeGroup::RTTI) {
			DBGOUT("row count group " << (size_t)item << " " << item->count());
			return item->count();
		} else {
			DBGOUT("row count item " << (size_t)item << " " << item->count());
			return 0;
		}
	} else {
		DBGOUT("row count invalid " << m_safe->count());
		return m_safe->count();
	}
}

int SafeTreeModel::columnCount(const QModelIndex &index) const
{
	return 1;
}

QModelIndex SafeTreeModel::index(SafeItem *item) const
{
	if(item != NULL) {
		SafeGroup *parent = item->parent();
		if(parent != NULL) {
			return createIndex(parent->index(item), 0, item);
		}
	}

	return QModelIndex();
}

QModelIndex SafeTreeModel::index(int row, int col, const QModelIndex &parent) const
{
	DBGOUT("Index " << row << " " << col << " " << parent.isValid() << " " << parent.row() << " " << parent.column() << " " << (size_t)parent.internalPointer());
	SafeGroup *grp = (SafeGroup *)parent.internalPointer();
	if(!parent.isValid() || grp == NULL) grp = m_safe;

	DBGOUT("   group index " << row << " " << (size_t)grp->at(row));
	return createIndex(row, col, grp->at(row));
}

QModelIndex SafeTreeModel::parent(const QModelIndex &index) const
{
	DBGOUT("parent " << index.row() << " " << index.column());
	if(index.isValid()) {
		SafeItem *item = static_cast<SafeItem *>(index.internalPointer());
		if(item != NULL && item->parent() != NULL) {
			return this->index(item->parent());
		} else {
			return this->index(m_safe);
		}
	}

	return QModelIndex();
}

/*
bool SafeTreeModel::hasChildren(const QModelIndex &parent) const
{
	DBGOUT("has children " << parent.row());
	if(parent.isValid()) {
		SafeItem *i = (SafeItem *)parent.internalPointer();
		return i->rtti() == SafeGroup::RTTI;
	} else {
		return true;
	}
}
*/

void SafeTreeModel::itemPreAdd(SafeItem *item, SafeGroup *parent)
{
	emit layoutAboutToBeChanged();
}

void SafeTreeModel::itemAdded(SafeItem *item, SafeGroup *parent)
{
	emit layoutChanged();
}

void SafeTreeModel::itemChanged(SafeItem *item)
{
  DBGOUT("item changed " << (size_t)item);
  QModelIndex index = this->index(item);
  emit dataChanged(index, index);
}

bool SafeTreeModel::removeRows(int position, int rows, const QModelIndex &parent)
{
  SafeGroup *grp;
  if(parent.isValid())
    grp = static_cast<SafeGroup *>(parent.internalPointer());
  else
    grp = m_safe;

  DBGOUT("beginRemove " << grp->name().toStdString() << " " << position << " " << rows);
  beginRemoveRows(parent, position, position + rows - 1);
  QList<SafeItem *> items;

  for(int i = 0; i < rows; i++) {
    SafeItem *it = grp->at(position + i);
    if(it) {
      DBGOUT("removing " << it->name().toStdString());
      items << it;
    }
  }

  for(QList<SafeItem *>::iterator iter = items.begin();
      iter != items.end();
      iter++) {
    delete (*iter);
  }

  endRemoveRows();

  return true;
}