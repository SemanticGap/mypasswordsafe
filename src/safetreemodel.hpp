#ifndef SAFETREEMODEL_HPP
#define SAFETREEMODEL_HPP

#include <QAbstractItemModel>
#include "safe.hpp"

class SafeTreeModel: public QAbstractItemModel
{
	Q_OBJECT;

	Safe *m_safe;

public:
	SafeTreeModel(Safe *, QObject *);
	virtual ~SafeTreeModel();

	inline Safe *safe() { return m_safe; }

	virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	virtual QModelIndex index(SafeItem *item, int column = 0) const;
	virtual QModelIndex index(int row, int col, const QModelIndex &parent = QModelIndex()) const override;
	virtual QModelIndex parent(const QModelIndex &parent = QModelIndex()) const override;
	virtual bool removeRows(int position, int rows, const QModelIndex &parent);

public slots:
	void itemAdded(SafeItem *, SafeGroup *);
	void itemPreAdd(SafeItem *, SafeGroup *);
	void itemChanged(SafeItem *);
};

#endif
