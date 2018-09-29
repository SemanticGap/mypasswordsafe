#ifndef MYPASSWORDSAFE_HPP
#define MYPASSWORDSAFE_HPP

#include <QMainWindow>
#include <QList>
#include <QString>
#include "encryptedstring.hpp"
#include "tools/idle/idle.h"
#include "ui_mypsmainwindow.h"
#include "manualdlg.hpp"
#include "pwordeditdlg.hpp"
#include "clipboard.hpp"

class MyPasswordSafe: public QMainWindow, public Ui::MyPSMainWindow
{
	Q_OBJECT;

	ManualDlg m_manual;
	int m_name_width, m_user_width, m_notes_width, m_max_tries;
	QString m_default_safe;
	bool m_clear_clipboard;
	QString m_def_user;
	Safe *m_safe;
	unsigned int m_gen_pword_length;
	QSettings m_config;
	bool m_do_lock;
	bool m_first_time;
	Idle *m_idle;
	int m_idle_timeout;
	QList<PwordEditDlg *> m_children;
	Clipboard *m_clipboard;

signals:
	void quit();

public:
	MyPasswordSafe(QWidget *parent = NULL);
	virtual ~MyPasswordSafe();

	void init();
	void destroy();

public slots:
	void closeEvent( QCloseEvent *e );
	bool closeSafe();
	void fileNew();
	void fileExit();
	void fileMakeDefault();
	unsigned int clearTimeOut();
	void setClearTimeOut(unsigned int timeout);
	void filePreferences();
	void fileOpen();
	bool open(const char *filename, const EncryptedString &passkey);
	bool open( const char *filename, const EncryptedString &passkey, const char *type );
	bool save();
	bool saveAs();
	SafeGroup *getSelectedParent() const;
	bool createEditDialog(SafeEntry *entry);
	bool removeEditDialog(PwordEditDlg *dlg);
	void pwordAdd();
	void editDialogAccepted();
	void editDialogRejected();
	void pwordDelete();
	void deleteItem(const QModelIndex &);
	void pwordEdit();
	void pwordFetch();
	void pwordFetchUser();
	void passwordActivated(SafeItem *item);
	void passwordMenuRequested(const QPoint &point);
	void helpAbout(int page);
	void helpManual();
	Safe *getSafe();
	int getGeneratedPwordLength();
	void savingEnabled();
	void savingEnabled( bool value);
	void setDefaultSafe(const QString &path);
	const QString &getDefaultSafe() { return m_default_safe; }
	bool browseForSafe( QString &filename, QString &filter, bool saving );
	void createNewSafe(const EncryptedString &passphrase);
	void fileOpenDefault();
	void fileChangePassPhrase();
	void hideChildren();
	void showChildren();
	void lock();
	void createGroup();
	bool clearClipboardOnExit() const;
	void setClearClipboardOnExit(bool yes);
	bool lockOnMinimize() const;
	void setLockOnMinimize(bool yes);
	void setGenerateAndShow(bool yes);
	void setGenerateAndFetch(bool yes);
	void setGenerateOnNew(bool yes);
	bool isMinimized() const;
	void hideEvent(QHideEvent *);
	void showEvent(QShowEvent *);
	void readConfig();
	void readColumnWidth(int col, const char *name);
	void writeColumnWidth(int col, const char *name);
	void writeConfig();
	//void dragObjectDropped(QMimeSource *drag, SafeListViewItem *target);
	void clipboardCleared();
	void slotSecondsIdle(int secs);
	void checkMinimization();

	bool firstTime() const { return m_first_time; }
	void helpAbout();
};

#endif
