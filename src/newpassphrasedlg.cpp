#include "newpassphrasedlg.hpp"

NewPassPhraseDlg::NewPassPhraseDlg(QWidget *parent)
	: QDialog(parent)
{
	setupUi(this);
}

NewPassPhraseDlg::~NewPassPhraseDlg()
{
}

void NewPassPhraseDlg::init()
{
	okButton->setEnabled(false);	
}

void NewPassPhraseDlg::okClicked()
{
	// FIXME: warn about the strength?
  QString line_one = firstLine->text(),
    line_two = secondLine->text();
	if(line_one == line_two) {
		accept();
	}
}


QString NewPassPhraseDlg::password()
{
	if(firstLine->text() == secondLine->text())
		return firstLine->text();
	else
		return QString::null;
}


void NewPassPhraseDlg::lineChanged(const QString &text)
{
	if(firstLine->text() == secondLine->text() && text.length() > 0)
		okButton->setEnabled(true);
	else
		okButton->setEnabled(false);
}
