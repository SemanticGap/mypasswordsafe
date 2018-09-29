#ifndef PWORDEDITDLG_HPP
#define PWORDEDITDLG_HPP

#include <QDialog>
#include "ui_pwordeditdlg.h"
#include "safe.hpp"

class PwordEditDlg: public QDialog, public Ui::PwordEditDlg
{
	Q_OBJECT;

	SafeEntry *m_item;
	SafeGroup *m_future_group;
	static bool generate_and_show;
	static bool generate_and_fetch;
	static bool generate_on_new;

public:
	static int gen_pword_length;
	static QString default_user;

	PwordEditDlg(QWidget *);
	void init();

	SafeEntry *getItem() const;
	void setItem(SafeEntry *item, SafeGroup *future_group = NULL);
	void updateItem(SafeGroup *future_group);

	static bool generateAndFetch();
	static void setGenerateAndFetch(bool yes);
	static bool generateAndShow();
	static void setGenerateAndShow(bool yes);
	static bool generateOnNew();
	static void setGenerateOnNew(bool yes);

public slots:
	void accept();
	void reject();
	void showPassword();
	void hidePassword();
	void togglePassword();
	void genPassword();
	void genPassword(bool fetch);
	void fetchPassword();
	QString getItemName() const;
	QString getUser() const;
	QString getPassword() const;
	QString getNotes() const;
	void setItemName( const QString &text );
	void setUser( const QString &text );
	void setPassword( const QString &text );
	void setNotes( const QString &text );
	void setGenPwordLength( int value );
	void setCreatedOn(const QDateTime &time);
	void setAccessedOn(const QDateTime &time);
	void setModifiedOn(const QDateTime &time);
	void setLifetime(const QTime &time);
	void setUUID(const QString &uuid);
	void showDetails(bool yes);
	bool detailsShown() const;
	bool isNew() const;
};

#endif