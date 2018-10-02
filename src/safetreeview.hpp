#ifndef SAFETREEVIEW_HPP
#define SAFETREEVIEW_HPP

#include <QTreeView>
#include <QSortFilterProxyModel>
#include "safetreemodel.hpp"

class Safe;
class SafeItem;

class SafeTreeView: public QTreeView
{
	Q_OBJECT;

	Safe *m_safe;
	SafeTreeModel *m_tree;
	QSortFilterProxyModel *m_proxy;

public:
	SafeTreeView(QWidget *parent);
	virtual ~SafeTreeView();

public slots:
	void setSafe(Safe *);
	inline Safe *safe() { return m_tree->safe(); }
	inline SafeTreeModel *safeModel() { return m_tree; }

	QModelIndex getSelectedIndex() const;
	SafeItem *getSelectedItem();
	void itemAdded(SafeItem *, bool);
	void itemChanged(SafeItem *);
	void itemDeleted(SafeItem *);
	void promoteActivated(const QModelIndex &);
	void setFilter(QString);

signals:
	void activated(SafeItem *);
};

#endif