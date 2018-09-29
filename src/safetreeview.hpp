#ifndef SAFETREEVIEW_HPP
#define SAFETREEVIEW_HPP

#include <QTreeView>
#include "safetreemodel.hpp"

class Safe;
class SafeItem;

class SafeTreeView: public QTreeView
{
	Q_OBJECT;

	Safe *m_safe;
	SafeTreeModel *m_tree;

public:
	SafeTreeView(QWidget *parent);
	virtual ~SafeTreeView();

public slots:
	void setSafe(Safe *);
	SafeItem *getSelectedItem();
	void itemAdded(SafeItem *, bool);
	void itemChanged(SafeItem *);
	void itemDeleted(SafeItem *);
	void promoteActivated(const QModelIndex &);

signals:
	void activated(SafeItem *);
};

#endif