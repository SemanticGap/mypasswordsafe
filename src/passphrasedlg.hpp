#ifndef PASSPHRASEDLG_HPP
#define PASSPHRASEDLG_HPP

#include <QDialog>
#include "ui_passphrasedlg.h"

class PassPhraseDlg: public QDialog, public Ui::PassPhraseDlg
{
	Q_OBJECT;

	QString m_passkey;

public:
	PassPhraseDlg(QWidget *);

	const QString getText() const;
	void pwordBox_textChanged( const QString &text );
	void hideCancel(bool yes);
};

#endif