#ifndef NEWPASSPHRASEDLG_HPP
#define NEWPASSPHRASEDLG_HPP

#include "ui_newpassphrasedlg.h"

class NewPassPhraseDlg: public QDialog, public Ui::NewPassPhraseDlg
{
	Q_OBJECT;
public:
	NewPassPhraseDlg(QWidget *);
	~NewPassPhraseDlg();

	void init();

public slots:
	void okClicked();
	QString password();
	void lineChanged(const QString &text);
};

#endif