#ifndef PREFERENCESDLG_HPP
#define PREFERENCESDLG_HPP

#include "ui_preferencesdlg.h"

class PreferencesDlg: public QDialog, public Ui::PreferencesDlg
{
	Q_OBJECT;

public:
	PreferencesDlg(QWidget *);
	virtual ~PreferencesDlg();

public slots:
	void onBrowse();
	void setDefaultSafe( const QString &safe );
	void setMaxTries( int tries );
	void setIdleTime(int);
	void setClearTime(int);
	void onHelp();
	QString getDefUser();
	QString getDefaultSafe();
	void setDefUser( const QString &user );
	int getGenPwordLength();
	void setGenPwordLength( int length );
	int getMaxTries();
	int getIdleTime();
	int getClearTime();
};

#endif