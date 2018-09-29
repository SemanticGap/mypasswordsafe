#include <assert.h>
#include "myutil.hpp"
#include "safetreeview.hpp"

SafeTreeView::SafeTreeView(QWidget *parent)
	: QTreeView(parent), m_tree(NULL)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(activated(const QModelIndex &)), this, SLOT(promoteActivated(const QModelIndex &)));
}

SafeTreeView::~SafeTreeView()
{
	if(m_tree)
		delete m_tree;
}

void SafeTreeView::setSafe(Safe *safe)
{
	if(m_tree)
		delete m_tree;
	m_tree = new SafeTreeModel(safe, this);
	setModel(m_tree);
}

SafeItem *SafeTreeView::getSelectedItem()
{
	QModelIndex index = currentIndex();
	if(index.isValid()) {
		return (SafeItem *)index.internalPointer();
	} else {
		return NULL;
	}
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
	if(index.isValid()) {
		SafeItem *item = (SafeItem *)index.internalPointer();
		emit activated(item);
	}
}
