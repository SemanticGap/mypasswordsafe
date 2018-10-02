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
	if(index.isValid()) {
		Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
		//SafeItem *i = (SafeItem *)index.internalPointer();
		f |= Qt::ItemIsDropEnabled;
		return f;
	}
	else
		return QAbstractItemModel::flags(index);
}

QVariant SafeTreeModel::data(const QModelIndex &index, int role) const
{
	if(role == Qt::SizeHintRole)
		return QVariant(QSize(100, 20));

	SafeItem *i = (SafeItem *)index.internalPointer();
	if(!index.isValid()) i = m_safe;

	if(i == NULL)
		return QVariant();

	if(role == Qt::DecorationRole && index.column() == 0) {
		switch(i->rtti()) {
		case SafeEntry::RTTI:
			return QIcon(":/images/password.png");
		case SafeGroup::RTTI:
			return QIcon(":/images/folder_blue.png");
		default:
			break;
		}		
	} else if(role == Qt::DisplayRole) {
		if(i == m_safe) {
			switch(index.column()) {
			case 0:
				return QVariant(m_safe->getPath());
			}
		} else if(i->rtti() == SafeEntry::RTTI) {
			SafeEntry *e = (SafeEntry *)i;
			//DBGOUT("data entry " << role << " " << e->name().toStdString() << " " << index.row() << " " << index.column());
			switch(index.column()) {
			case 0:
				return QVariant(e->name());
			case 1: return QVariant(QString("%1 %2").arg(index.row()).arg(index.column()));
			case 2: return QVariant(parent(index).row());
			case 3: return QVariant(flags(index));
			}
		} else if(i->rtti() == SafeGroup::RTTI) {
			SafeGroup *e = (SafeGroup *)i;
			//DBGOUT("data group " << role << " " << e->name().toStdString() << " " << index.row() << " " << index.column());
			switch(index.column()) {
			case 0:
				return QVariant(e->name());
			case 1: return QVariant(QString("%1 %2").arg(index.row()).arg(index.column()));
			case 2: return QVariant(parent(index).row());
			case 3: return QVariant(flags(index));
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
		switch(section) {
		case 0: return QVariant(tr("Name"));
		case 1: return QVariant(tr("Position"));
		case 2: return QVariant(tr("Parent"));
		case 3: return QVariant(tr("Flags"));
		}
	}

	return QVariant();
}

int SafeTreeModel::rowCount(const QModelIndex &parent) const
{
	DBGOUT("row count " << parent.row() << " " << parent.column() << " " << (size_t)parent.internalPointer() << " " << parent.isValid() << " " << parent.parent().row());
	if(parent.isValid()) {
		SafeItem *item = (SafeItem *)parent.internalPointer();
		if(item != NULL && item->rtti() == SafeGroup::RTTI) {
			SafeGroup *grp = (SafeGroup *)item;
			DBGOUT("row count group " << (size_t)grp << " " << grp->count());
			return grp->count();
		} else {
			DBGOUT("row count item " << (size_t)item);
			return 0;
		}
	} else {
		DBGOUT("row count invalid " << m_safe->count());
		return m_safe->count();
	}
}

int SafeTreeModel::columnCount(const QModelIndex &index) const
{
	return 4;
}

QModelIndex SafeTreeModel::index(SafeItem *item, int column) const
{
	if(item != NULL && item != m_safe) {
		SafeGroup *parent = item->parent();
		if(parent != NULL) {
			return createIndex(parent->index(item), column, item);
		}
	}

	return QModelIndex();
}

QModelIndex SafeTreeModel::index(int row, int col, const QModelIndex &parent) const
{
	DBGOUT("Index " << row << " " << col << "\tparent " << parent.isValid() << " " << parent.row() << " " << parent.column() << " " << (size_t)parent.internalPointer());
	SafeItem *parent_item;
	if(parent.isValid())
		parent_item = static_cast<SafeItem *>(parent.internalPointer());
	else
		parent_item = m_safe;

	if(parent_item == NULL || parent_item->rtti() != SafeGroup::RTTI) return QModelIndex();

	SafeGroup *grp = static_cast<SafeGroup *>(parent_item);
	SafeItem *item = grp->at(row);
	DBGOUT("   group index " << row << " " << (size_t)item);
	if(item != NULL)
		return createIndex(row, col, item);
	else
		return QModelIndex();
}

QModelIndex SafeTreeModel::parent(const QModelIndex &index) const
{
	DBGOUT("parent " << index.row() << " " << index.column());
	if(index.isValid()) {
		SafeItem *item = static_cast<SafeItem *>(index.internalPointer());
		if(item != NULL && item != m_safe) {
			return this->index(item->parent());
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
	//int i = parent->index(item);
	//emit rowsAboutToBeInserted(index(parent), i, i);
}

void SafeTreeModel::itemAdded(SafeItem *item, SafeGroup *parent)
{
#if QT_VERSION >= 0x050000
	//QList<QPersistentModelIndex> parents;
	//parents << index(parent);
	//parents << index(item);
	//emit layoutChanged(parents, VerticalSortHint);
	emit layoutChanged();
#else
	emit layoutChanged();
	//QModelIndex i = index(item);
	//emit dataChanged(i, i);
#endif
}

void SafeTreeModel::itemChanged(SafeItem *item)
{
  DBGOUT("item changed " << (size_t)item);
  QModelIndex index = this->index(item);
  emit dataChanged(index, this->index(item, columnCount(index)));
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