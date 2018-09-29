#ifndef STARTUPDLG_HPP
#define STARTUPDLG_HPP

#include "ui_startupdlgbase.h"

class MyPasswordSafe;

class StartupDlg: public QDialog, public Ui::StartupDlgBase
{
	Q_OBJECT;

	QString m_filter;
	QString m_filename;
	MyPasswordSafe *m_myps;
	
public:
	enum Action { OpenDefault, Browse, CreateNew };
	
	StartupDlg(MyPasswordSafe *myps);
	virtual ~StartupDlg();

	void init();

public slots:	
	Action getAction();
	void setAction(Action action);
	void okClicked();
	void actionChanged(int);
	void actionChanged(int action, bool browse);
	void setFilename(const QString &);
	const QString &getFilter();
	void setFilter(const QString &);
	const QString &getFilename();
	QString getPassPhrase();
	int getActionBoxItem();
	void setActionBoxItem(int);
	void setMyPasswordSafe(MyPasswordSafe *);
};

#endif
