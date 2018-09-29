#include "config.h"
#include "aboutdlg.hpp"

AboutDlg::AboutDlg(QWidget *parent)
	: QDialog(parent)
{
	setupUi(this);
	init();
}

AboutDlg::~AboutDlg()
{
}

void AboutDlg::init()
{
	QString text(infoText->text());
	text.replace("{VERSION}", VERSION);
	text.replace("{DATE}", COMP_DATE);
	text.replace("{USER}", COMP_USER);
	text.replace("{HOST}", COMP_HOST);
	text.replace("{DATE}", COMP_DATE);
	infoText->setText(text);
}

void AboutDlg::setCurrentPage(int page)
{
	tabWidget2->setCurrentPage(page);
}

int AboutDlg::currentPage() const
{
	return tabWidget2->currentPageIndex();
}
