#ifndef MANUALDLG_HPP
#define MANUALDLG_HPP

#include "ui_manualdlg.h"

class ManualDlg: public QDialog, public Ui::ManualDlg
{
	Q_OBJECT;

public:
	ManualDlg(QWidget *);
	virtual ~ManualDlg();
};

#endif