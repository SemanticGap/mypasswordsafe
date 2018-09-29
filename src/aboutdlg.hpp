#ifndef ABOUTDLG_HPP
#define ABOUTDLG_HPP

#include "ui_aboutdlg.h"

class AboutDlg: public QDialog, public Ui::AboutDlg
{
	Q_OBJECT;

public:
	AboutDlg(QWidget *);
	virtual ~AboutDlg();

	void init();

public slots:
	void setCurrentPage( int page );
	int currentPage() const;
};

#endif