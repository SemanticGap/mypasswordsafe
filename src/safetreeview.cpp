#include <assert.h>
#include <QSortFilterProxyModel>
#include "myutil.hpp"
#include "safetreeview.hpp"

#define SORTING 1

SafeTreeView::SafeTreeView(QWidget *parent)
	: QTreeView(parent), m_tree(NULL), m_proxy(NULL)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(activated(const QModelIndex &)), this, SLOT(promoteActivated(const QModelIndex &)));
}

SafeTreeView::~SafeTreeView()
{
	if(m_tree)
		delete m_tree;
	if(m_proxy)
		delete m_proxy;
}

void SafeTreeView::setSafe(Safe *safe)
{
	if(safe == NULL) {
		setModel(NULL);
		if(m_tree) {
			delete m_tree;
			m_tree = NULL;
		}
		if(m_proxy) {
			delete m_proxy;
			m_proxy = NULL;
		}
	} else {
		SafeTreeModel *tree = new SafeTreeModel(safe, this);
#if(SORTING == 1)
		QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
		proxy->setSourceModel(tree);
		proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
#if QT_VERSION >= 0x051000
		proxy->setRecursiveFilteringEnabled(true);
#endif
		setModel(proxy);
		setSortingEnabled(true);
		setFilter("");
#else
		setModel(tree);
#endif

		if(m_tree)
			delete m_tree;
		m_tree = tree;
#if(SORTING == 1)
		if(m_proxy)
			delete m_proxy;
		m_proxy = proxy;
#endif
	}
}

QModelIndex SafeTreeView::getSelectedIndex() const
{
	QItemSelectionModel *selection = selectionModel();
	if(selection->hasSelection()) {
		QModelIndexList items = selection->selectedRows();
		return items.first();
	}

	return QModelIndex();
}

SafeItem *SafeTreeView::getSelectedItem()
{
	QModelIndex index = getSelectedIndex();
	if(m_proxy != NULL)
		index = m_proxy->mapToSource(index);

	if(index.isValid()) {
		return (SafeItem *)index.internalPointer();
	}

	return NULL;
}

void SafeTreeView::itemAdded(SafeItem *item, bool)
{
}

void SafeTreeView::itemChanged(SafeItem *)
{
}

void SafeTreeView::itemDeleted(SafeItem *)
{
}

void SafeTreeView::promoteActivated(const QModelIndex &index)
{
	QModelIndex i = index;
	if(m_proxy != NULL)
		i = m_proxy->mapToSource(index);

	if(i.isValid()) {
		SafeItem *item = (SafeItem *)i.internalPointer();
		emit activated(item);
	}
}

void SafeTreeView::setFilter(QString f)
{
	if(m_proxy) {
		m_proxy->setFilterWildcard(f);
		sortByColumn(0, Qt::AscendingOrder);
	}
}
