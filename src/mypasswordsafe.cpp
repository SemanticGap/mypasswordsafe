/* $Header: /home/cvsroot/MyPasswordSafe/src/mypasswordsafe.ui.h,v 1.44 2006/09/25 07:04:31 nolan Exp $
 * Copyright (c) 2004, Semantic Gap (TM)
 * http://www.semanticgap.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <cstdlib>
#include <iostream>
#include <string>
#include <qapplication.h>
#include <qfiledialog.h>
// #include <kfiledialog.h>
#include <qstatusbar.h>
#include <qclipboard.h>
#include <qmessagebox.h>
#include <qinputdialog.h>
#include <qsettings.h>
#include <qdom.h>
#include <qtimer.h>
#include <QWhatsThis>
#include <QHideEvent>
#include <QShowEvent>
#include <QCloseEvent>
#include "tools/idle/idle.h"
#include "passphrasedlg.hpp"
#include "pwordeditdlg.hpp"
#include "manualdlg.hpp"
#include "newpassphrasedlg.hpp"
#include "myutil.hpp"
#include "xmlserializer.hpp"
#include "clipboard.hpp"
#include "mypasswordsafe.hpp"
#include "preferencesdlg.hpp"
#include "aboutdlg.hpp"

using namespace std;

typedef QFileDialog MyFileDialog;

MyPasswordSafe::MyPasswordSafe(QWidget *parent)
	: QMainWindow(parent),
	m_manual(this),
	m_safe(NULL),
	m_config(QSettings::UserScope, ORGNAME, APPNAME),
	m_idle(NULL),
	m_clipboard(NULL)
{
  setupUi(this);
  init();
}

MyPasswordSafe::~MyPasswordSafe()
{
  destroy();
}

void MyPasswordSafe::init()
{
#ifdef __APPLE__
  setAttribute(Qt::WA_MacMetalStyle);
#endif /* __APPLE__ */

  m_clipboard = Clipboard::instance();
  connect(editClearClipboardAction, SIGNAL(activated()), m_clipboard, SLOT(clear()));
  connect(m_clipboard, SIGNAL(cleared()), this, SLOT(clipboardCleared()));

  connect(pwordTreeView, SIGNAL(activated(SafeItem *)), this, SLOT(passwordActivated(SafeItem *)));
  connect(pwordTreeView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(passwordMenuRequested(const QPoint &)));

  m_do_lock = false;
  savingEnabled(false);

  readConfig();

  m_safe = NULL; // m_safe gets setup by the Startup dialog

  m_idle = new Idle;
  m_idle->start();
  connect(m_idle, SIGNAL(secondsIdle(int)), this, SLOT(slotSecondsIdle(int)));

  settingsGenerateAndShow->setChecked(PwordEditDlg::generateAndShow());
  settingsGenerateAndFetch->setChecked(PwordEditDlg::generateAndFetch());
  settingsGenerateOnNew->setChecked(PwordEditDlg::generateOnNew());
}

void MyPasswordSafe::destroy()
{
  delete m_idle;

  if(clearClipboardOnExit())
    m_clipboard->copy("");

  writeConfig();

  if(m_safe)
    delete m_safe;
}


void MyPasswordSafe::closeEvent( QCloseEvent *e )
{
  // ask to save file if needed
  if(closeSafe()) {
    e->accept();
    emit quit();
  }
}


bool MyPasswordSafe::closeSafe()
{
  if(m_safe) {
    if(m_safe->hasChanged()) {
      int result = QMessageBox::warning(this, tr("Save safe?"),
					tr("Do you want to save the safe before it is closed?"),
					QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
      switch(result) {
      case QMessageBox::Yes:
	if(!save())
		return false;
	break;
      case QMessageBox::Cancel:
	return false;
      default:
	break;
      }
    }
    delete m_safe;
    m_safe = NULL;
    savingEnabled(false);
  }
  return true;
}


void MyPasswordSafe::fileNew()
{
  NewPassPhraseDlg dlg(this);
  // NOTE:  the dialog doesn't trash the memory
  if(dlg.exec() == QDialog::Rejected) {
    statusBar()->showMessage(tr("No pass-phrase entered"));
    return;
  }

  createNewSafe(EncryptedString(dlg.password().toUtf8()));
}


void MyPasswordSafe::fileExit()
{
  close();
}


void MyPasswordSafe::fileMakeDefault()
{
  if(m_safe->hasChanged()) {
    if(!save()) {
      statusBar()->showMessage(tr("The safe must be saved before it can be made the default."));
      return;
    }
  }
  setDefaultSafe(m_safe->getPath());
  fileOpenDefaultAction->setEnabled(true);
}

unsigned int MyPasswordSafe::clearTimeOut()
{
  return Clipboard::timeOut() / 1000;
}

void MyPasswordSafe::setClearTimeOut(unsigned int timeout)
{
  Clipboard::setTimeOut(timeout * 1000);
}

void MyPasswordSafe::filePreferences()
{
  PreferencesDlg dlg(this);
  dlg.setDefUser(PwordEditDlg::default_user);
  dlg.setDefaultSafe(m_default_safe);
  dlg.setGenPwordLength(PwordEditDlg::gen_pword_length);
  dlg.setMaxTries(m_max_tries);
  dlg.setIdleTime(m_idle_timeout);
  dlg.setClearTime(clearTimeOut());

  if(dlg.exec() == QDialog::Accepted) {
    PwordEditDlg::default_user = dlg.getDefUser();
    setDefaultSafe(dlg.getDefaultSafe());
    PwordEditDlg::gen_pword_length = dlg.getGenPwordLength();
    m_max_tries = dlg.getMaxTries();
    m_idle_timeout = dlg.getIdleTime();
    setClearTimeOut(dlg.getClearTime());

    if(m_default_safe.isEmpty() == false)
      fileOpenDefaultAction->setEnabled(true);
    else
      fileOpenDefaultAction->setEnabled(false);
  }
}


void MyPasswordSafe::fileOpen()
{
  QString filename, filter;

  if(browseForSafe(filename, filter, false)) {
    if(!filename.isEmpty()) {
      QString file_ext = filename.right(filename.lastIndexOf('.'));
      Safe::Error open_ret = Safe::Failed;

      // check the password
      PassPhraseDlg passphrase_dlg(this);
      SecuredString pword;
      int num_tries = 0;
      while(num_tries < m_max_tries) {
	if(passphrase_dlg.exec() == PassPhraseDlg::Accepted) {
	  pword.set(passphrase_dlg.getText().toUtf8());

	  open_ret = Safe::checkPassword(filename, filter, pword);

	  if(open_ret == Safe::Success)
	    break;
	  else {
	    statusBar()->showMessage(Safe::errorToString(open_ret));
	  }

	  num_tries++;
	}
	else {
	  statusBar()->showMessage(tr("No pass-phrase entered"));
	  return;
	}
      }

      if(open(filename.toUtf8(), pword, filter.toUtf8())) {
	statusBar()->showMessage(tr("Opened %1 entries in %2 groups").arg(m_safe->totalNumEntries()).arg(m_safe->totalNumGroups()));
      }
    }
    else {
      statusBar()->showMessage(tr("No filename specified"));
    }
  }
  else {
    statusBar()->showMessage(tr("Open file cancelled"));
  }
}


bool MyPasswordSafe::open(const char *filename, const EncryptedString &passkey)
{
  return open(filename, passkey, NULL);
}


bool MyPasswordSafe::open( const char *filename, const EncryptedString &passkey, const char *type )
{
  Safe::Error ret = Safe::Failed;
  DBGOUT("Checking password");
  ret = Safe::checkPassword((const char *)filename,
			    (const char *)type, passkey);

  if(ret == Safe::Success) {
    // this is when we ask the user if they want to save a
    // modified safe or not
    if(!closeSafe())
      return false;
    Safe *s = new Safe;
    ret = s->load((const char *)filename,
			      (const char *)type,
			      passkey,
			      (const char *)PwordEditDlg::default_user.toUtf8());
    if(ret == Safe::Success) {
      DBGOUT(filename << " has " << s->count() << " entries");

      Safe *current_safe = m_safe;

      m_safe = s;
      pwordTreeView->setSafe(m_safe);

      if(current_safe != NULL)
	delete current_safe;

      setWindowTitle(tr("MyPasswordSafe: ") + filename);
      savingEnabled(false);
      fileSaveAsAction->setEnabled(true);
      connect(m_safe, SIGNAL(changed()), this, SLOT(savingEnabled()));
      return true;
    }
    else {
      statusBar()->showMessage(Safe::errorToString(ret));
      delete s;
    }
  }
  else if(ret == Safe::Failed) {
    statusBar()->showMessage(tr("Wrong pass phrase"));
  }
  else {
    statusBar()->showMessage(Safe::errorToString(ret));
  }

  return false;
}


bool MyPasswordSafe::save()
{
  if(m_safe != NULL) {
    QString path(m_safe->getPath());
    if(path.isEmpty())
      return saveAs();
    else {
      Safe::Error error = m_safe->save((const char *)PwordEditDlg::default_user.toUtf8());
      if(error == Safe::Success) {
	savingEnabled(false);
	statusBar()->showMessage(tr("Safe saved"));
	return true;
      }
      else {
	statusBar()->showMessage(Safe::errorToString(error));
      }
    }
  }
  return false;
}


bool MyPasswordSafe::saveAs()
{
  QString filename, filter;

  if(browseForSafe(filename, filter, true)) {
    //QString filter = file_dlg.selectedFilter();
    if(!filename.isEmpty()) {
      Safe::Error error = m_safe->save(filename,
				       filter,
				       PwordEditDlg::default_user);
      if(error == Safe::Success) {
	setWindowTitle(tr("MyPasswordSafe: ") + m_safe->getPath());
	statusBar()->showMessage(tr("Safe saved"));
	savingEnabled(false);
	return true;
      }
      else {
	DBGOUT("Error: " << error);
	statusBar()->showMessage(tr("Error saving file"));
      }
    }
  }
  else {
    statusBar()->showMessage(tr("Save file cancelled"));
  }
  return false;
}


SafeGroup *MyPasswordSafe::getSelectedParent() const
{
    SafeItem *selection = pwordTreeView->getSelectedItem();
    SafeGroup *parent = m_safe;

    if(selection != NULL) {
      if(selection->rtti() == SafeEntry::RTTI) {
	parent = selection->parent();
      }
      else if(selection->rtti() == SafeGroup::RTTI) {
	parent = (SafeGroup *)selection;
      }
    }

    return parent;
}

bool MyPasswordSafe::createEditDialog(SafeEntry *entry)
{
  // return false if we're already editing this item
  if(entry != NULL) {
    for(QList<PwordEditDlg *>::iterator iter = m_children.begin();
	iter != m_children.end();
	iter = iter++) {
      PwordEditDlg *item = *iter;
      if(item->getItem() == entry) {
	item->raise();
	item->activateWindow();
	return false;
      }
    }
  }

  // create the dialog
  PwordEditDlg *dlg = new PwordEditDlg(this);
  dlg->setItem(entry, getSelectedParent());

  // connect the dialog to the close slot
  connect(dlg, SIGNAL(accepted()), this, SLOT(editDialogAccepted()));
  connect(dlg, SIGNAL(rejected()), this, SLOT(editDialogRejected()));

  // add the dialog to the list
  m_children.append(dlg);

  // show the dialog
  dlg->show();

  return true;
}

bool MyPasswordSafe::removeEditDialog(PwordEditDlg *dlg)
{
  // find and remove the dialog
  return m_children.removeOne(dlg);
}

void MyPasswordSafe::pwordAdd()
/* Shows the password edit dialog for the user to
 * fill out the values that the new entry will have,
 * and then adds the item to safe and view.
 */
{
  if(createEditDialog(NULL) == false) {
    statusBar()->showMessage(tr("Failed to create password entry dialog"));
  }
}

void MyPasswordSafe::editDialogAccepted()
{
  PwordEditDlg *dlg = dynamic_cast<PwordEditDlg *>(sender());
  DBGOUT("editDialogClosed: " << dlg->result());

  if(dlg->isNew()) {
    pwordTreeView->itemAdded(dlg->getItem(), true);
    statusBar()->showMessage(tr("Item added"));
  }
  else {
    pwordTreeView->itemChanged(dlg->getItem());
    statusBar()->showMessage(tr("Password updated"));
  }

  savingEnabled(true);
  removeEditDialog(dlg);
}

void MyPasswordSafe::editDialogRejected()
{
  PwordEditDlg *dlg = dynamic_cast<PwordEditDlg *>(sender());

  if(dlg->isNew()) {
    statusBar()->showMessage(tr("Add canceled"));
  }
  else {
    statusBar()->showMessage(tr("Edit password cancelled"));
    savingEnabled(true); // updated the access time
  }

  removeEditDialog(dlg);
}


void MyPasswordSafe::pwordDelete()
{
  //SafeItem *item(pwordTreeView->getSelectedItem());
  QModelIndex index = pwordTreeView->currentIndex();
  if(index.isValid()) {
    int result = QMessageBox::warning(this, tr("Are you sure?"),
				      tr("Are you sure you want to delete this password?"),
				      QMessageBox::Yes, QMessageBox::No);
    switch(result) {
    case QMessageBox::Yes:
      deleteItem(index);
      statusBar()->showMessage(tr("Password deleted"));
      break;
    default:
      statusBar()->showMessage(tr("Delete cancelled"));
      break;
    }
  }
  else {
    statusBar()->showMessage(tr("No item selected"));
  }
}


void MyPasswordSafe::deleteItem(const QModelIndex &index)
{
  DBGOUT("deleteItem");
  //m_safe->deleteItem(item);
  pwordTreeView->model()->removeRow(index.row(), index.parent());
  savingEnabled(true);
}

void MyPasswordSafe::pwordEdit()
{
  SafeItem *item = pwordTreeView->getSelectedItem();
  if(item != NULL) {
    if(item->rtti() == SafeEntry::RTTI) {
      SafeEntry *entry = (SafeEntry *)item;

      if(createEditDialog(entry) == false) {
	statusBar()->showMessage(tr("You are already editing this item."));
      }
    }
  }
  else {
    statusBar()->showMessage(tr("No item selected"));
  }
}

void MyPasswordSafe::pwordFetch()
{
  SafeItem *item = pwordTreeView->getSelectedItem();
  if(item && item->rtti() == SafeEntry::RTTI) {
    SafeEntry *entry = (SafeEntry *)item;
    entry->updateAccessTime();

    // NOTE: password decrypted
    SecuredString pword(entry->password().get());
    m_clipboard->copy(QString::fromUtf8(pword.get()), true);

    statusBar()->showMessage(tr("Password copied to clipboard"));
  }
  else {
    statusBar()->showMessage(tr("No item selected"));
  }
}


void MyPasswordSafe::pwordFetchUser()
{
  SafeItem *item = pwordTreeView->getSelectedItem();
  if(item && item->rtti() == SafeEntry::RTTI) {
    SafeEntry *entry = (SafeEntry *)item;
    entry->updateAccessTime();

    m_clipboard->copy(entry->user(), true);
    statusBar()->showMessage(tr("Username copied to clipboard"));
  }
  else {
    statusBar()->showMessage(tr("No item selected"));
  }
}

void MyPasswordSafe::passwordActivated(SafeItem *item)
{
  if(item->rtti() == SafeEntry::RTTI)
    pwordFetch();
}

void MyPasswordSafe::passwordMenuRequested(const QPoint &point)
{
  PopupMenu->popup(pwordTreeView->mapToGlobal(point));
}


void MyPasswordSafe::helpAbout(int page)
{
  AboutDlg dlg(this);
  dlg.setCurrentPage(page);
  dlg.exec();
}


void MyPasswordSafe::helpManual()
{
  if(m_manual.isVisible()) {
    m_manual.hide();
  }
  else {
    m_manual.show();
  }
}


Safe *MyPasswordSafe::getSafe()
{
  return m_safe;
}


int MyPasswordSafe::getGeneratedPwordLength()
{
  return PwordEditDlg::gen_pword_length;
}

void MyPasswordSafe::savingEnabled()
{
  savingEnabled(true);
}

void MyPasswordSafe::savingEnabled( bool value)
{
  if(value) {
    fileSaveAction->setEnabled(true);
    fileSaveAsAction->setEnabled(true);
  }
  else {
    fileSaveAction->setEnabled(false);
  }
}

void MyPasswordSafe::setDefaultSafe(const QString &path)
{
  m_default_safe = path;
}

bool MyPasswordSafe::browseForSafe( QString &filename, QString &filter, bool saving )
{
  QString all_safes(tr("All Safes (%1)").arg(Safe::getExtensions()));
  QString all_files(tr("All Files (*)"));

  QString types(QString("%1\n%2\n%3").arg(all_safes).arg(Safe::getTypes()).arg(all_files));
  QString f;
  bool ret = false;

  static QString path = QString::null;
  if(path == QString::null) {
    char *home = getenv("HOME");
    if(home != NULL) {
      path = QString(home);
    } else {
      path = QString("/");
    }
  }

  do {
    if(saving) {
      f = MyFileDialog::getSaveFileName(this,
				       tr("Enter a file to save to"),
					path,
				       types,
					&filter);
    }
    else {
      f = MyFileDialog::getOpenFileName(this,
				       tr("Choose a file to open"),
                                        path,
				       types,					
					&filter);
    }
  
    if(!f.isEmpty()) {
      if(saving) {
	QFileInfo info(f);
	if(info.exists()) {
	  if(QMessageBox::warning(this, tr("File exists"),
				  tr("Are you sure you want to overwrite \"%1\"?").arg(f),
				  QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
	    continue;
	  }
	}
      }

      filename = f;
      if(filter == all_safes)
	filter = QString::null;

      path = QFileInfo(f).absolutePath();

      ret = true;
    }
    else {
      break;
    }
  } while(ret == false);

  return ret;
}


void MyPasswordSafe::createNewSafe(const EncryptedString &passphrase)
{
  if(closeSafe()) {
    m_safe = new Safe();
    m_safe->setPassPhrase(passphrase);
    pwordTreeView->setSafe(m_safe);
    m_safe->setChanged(false);
    setWindowTitle(tr("MyPasswordSafe: <untitled>"));
    savingEnabled(false);
    fileSaveAsAction->setEnabled(false);
    statusBar()->showMessage(tr("Created new safe"));
  }
}


void MyPasswordSafe::fileOpenDefault()
{
  PassPhraseDlg dlg(this);
  if(dlg.exec() == PassPhraseDlg::Accepted) {
    if(open((const char *)getDefaultSafe().toUtf8(), (const char *)dlg.getText().toUtf8()))
      statusBar()->showMessage(tr("Opened %1 entries in %2 groups").arg(m_safe->totalNumEntries()).arg(m_safe->totalNumGroups()));
    else
      statusBar()->showMessage(tr("Unable to open the default safe"));
  }
  else {
    statusBar()->showMessage(tr("No pass-phrase entered"));
  }
}


void MyPasswordSafe::fileChangePassPhrase()
{
  // prompt the user for a new password
  NewPassPhraseDlg dlg(this);
  if(dlg.exec() == NewPassPhraseDlg::Accepted) {
    // set the new password
    m_safe->setPassPhrase(EncryptedString((const char *)dlg.password().toUtf8()));
    savingEnabled(true);
    statusBar()->showMessage(tr("Pass-phrase changed"));
  }
  else {
    statusBar()->showMessage(tr("Canceled"));
  }
}

void MyPasswordSafe::hideChildren()
{
    for(QList<PwordEditDlg *>::iterator iter = m_children.begin();
	iter != m_children.end();
	iter = iter++) {
      (*iter)->hide();
    }
}

void MyPasswordSafe::showChildren()
{
    for(QList<PwordEditDlg *>::iterator iter = m_children.begin();
	iter != m_children.end();
	iter = iter++) {
      (*iter)->show();
    }
}

void MyPasswordSafe::whatsThis()
{
  QWhatsThis::enterWhatsThisMode();
}

void MyPasswordSafe::lock()
{
  PassPhraseDlg dlg(this);
  dlg.hideCancel(true);

  hide();
  hideChildren();
  //closeChild(); // NOTE: hiding would cause a crash on show
  // FIXME: hiding the edit dialog should work with non-modal dialogs

  m_clipboard->clear();

  do {
    dlg.exec(); // will only accept
  } while(m_safe->getPassPhrase() != EncryptedString(dlg.getText().toUtf8()));
  
  showNormal();
  showChildren();

  statusBar()->showMessage(tr("MyPasswordSafe unlocked"));
}


void MyPasswordSafe::createGroup()
{
  // prompt the user for the groups name
  bool ok = false;
  QString group_name = QInputDialog::getText(this,
                                             tr("MyPasswordSafe"),
					     tr("What would you like to name the group?"),
					     QLineEdit::Normal,
					     QString::null, &ok);

  // if the user cancels, set the status bar's message and return
  if(ok == false) {
    statusBar()->showMessage(tr("Create group canceled"));
    return;
  }

  // get the selected item
  SafeItem *item = pwordTreeView->getSelectedItem();
  SafeGroup *parent = m_safe;

  // if there's an item selected
  if(item != NULL) {
    //    get its group
    if(item->rtti() == SafeEntry::RTTI) {
      parent = item->parent();
    }
    else if(item->rtti() == SafeGroup::RTTI) {
      parent = (SafeGroup *)item;
    }
    else {
      DBGOUT("Selected item not a valid item type");
      statusBar()->showMessage(tr("The selected item is invalid"));
      return;
    }
  }

  //    create the group
  SafeGroup *group = new SafeGroup(parent, group_name);
  if(group == NULL) {
    statusBar()->showMessage(tr("Unable to create the group"));
  }
  else {
    m_safe->setChanged(true);
    pwordTreeView->itemAdded(group, true);

    savingEnabled(true);
    statusBar()->showMessage(tr("Created the group \"%1\"").arg(group_name));
  }
}


bool MyPasswordSafe::clearClipboardOnExit() const
{
  return clearClipboardOnExitAction->isChecked();
}


void MyPasswordSafe::setClearClipboardOnExit(bool yes)
{
  clearClipboardOnExitAction->setChecked(yes);
}


bool MyPasswordSafe::lockOnMinimize() const
{
  return lockOnMinimizeAction->isChecked();
}


void MyPasswordSafe::setLockOnMinimize(bool yes)
{
  lockOnMinimizeAction->setChecked(yes);
}

void MyPasswordSafe::setGenerateAndShow(bool yes) 
{
  PwordEditDlg::setGenerateAndShow(yes);
}

void MyPasswordSafe::setGenerateAndFetch(bool yes) 
{
  PwordEditDlg::setGenerateAndFetch(yes);
}

void MyPasswordSafe::setGenerateOnNew(bool yes) 
{
  PwordEditDlg::setGenerateOnNew(yes);
}

#ifdef Q_WS_X11
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <QX11Info>

bool MyPasswordSafe::isMinimized() const
{
  //DBGOUT("isMinimized: " << QMainWindow::isMinimized());

  Display *display = QX11Info::display();
  Atom atom;
  static const int prop_buffer_lengh = 1024 * 1024;
  Atom type_returned = 0;
  int format_returned = 0;
  unsigned long n_clients;
  unsigned long bytes_after_return = 0;
  unsigned char *data = NULL;

  atom = XInternAtom (display, "WM_STATE", False);
  XGetWindowProperty (display, (Window)winId(),
		      atom, 0, prop_buffer_lengh, False, atom,
		      &type_returned, &format_returned, &n_clients,
		      &bytes_after_return, &data);

  /*#ifdef DEBUG
  for(int i = 0; i < n_clients; i++) {
    DBGOUT("data[" << i << "] = " << data[i] << "\t" << (data[i] == IconicState));
  }
  #endif*/
  return (n_clients > 0 && data[0] == IconicState);
}
#else
bool MyPasswordSafe::isMinimized() const
{
  return QMainWindow::isMinimized();
}
#endif

void MyPasswordSafe::hideEvent(QHideEvent *)
{
  DBGOUT("Hide event: " << isMinimized());

  // Fire a timer to see if we need to lock due
  // to a minimization after 5 seconds
  if(lockOnMinimize()) {
    QTimer::singleShot(5*1000, this, SLOT(checkMinimization()));
  }
}

void MyPasswordSafe::showEvent(QShowEvent *)
{
  DBGOUT("Show event: " << isMinimized());

  if(m_do_lock && lockOnMinimize()) {
    m_do_lock = false;
    lock();
  }
}

static const char *MyPS_Column_Fields[] = {
  "/MainWindow/name_width",
  "/MainWindow/user_width",
  "/MainWindow/notes_width",
  "/MainWindow/modtime_width",
  "/MainWindow/accessed_width",
  "/MainWindow/created_width",
  "/MainWindow/lifetime_width",
  NULL
};

void MyPasswordSafe::readConfig()
{
  m_config.beginGroup("MyPasswordSafe");

  m_first_time = m_config.value("prefs/first_time", true).toBool();
  PwordEditDlg::gen_pword_length = m_config.value("prefs/password_length", 8).toInt();
  
  m_default_safe = m_config.value("prefs/default_safe", "").toString();
  if(m_default_safe.isEmpty())
    fileOpenDefaultAction->setEnabled(false);
  
  PwordEditDlg::default_user = m_config.value("prefs/default_username", "").toString();
  setClearClipboardOnExit(m_config.value("prefs/clear_clipboard", true).toBool());
  setLockOnMinimize(m_config.value("prefs/lock_on_minimize", true).toBool());
  PwordEditDlg::setGenerateAndShow(m_config.value("prefs/generate/show", true).toBool());
  PwordEditDlg::setGenerateAndFetch(m_config.value("prefs/generate/fetch", true).toBool());
  PwordEditDlg::setGenerateOnNew(m_config.value("prefs/generate/on_new", true).toBool());
  
  m_max_tries = m_config.value("prefs/max_tries", 3).toInt();
  if(m_max_tries > 10)
    m_max_tries = 10;
  
  m_idle_timeout = m_config.value("prefs/idle_timeout", 2).toInt(); // 2 minute default
  setClearTimeOut(m_config.value("prefs/clear_timeout", 60).toInt()); // 1 minute default

  resize(m_config.value("MainWindow/size", QSize(400, 320)).toSize());

  QVariant pos = m_config.value("MainWindow/position");
  if(!pos.isNull()) {
    move(pos.toPoint());
  }

  for(int i = 0; MyPS_Column_Fields[i] != NULL; i++) {
    readColumnWidth(i, MyPS_Column_Fields[i]);
  }
  
  m_config.endGroup();  
}

void MyPasswordSafe::readColumnWidth(int col, const char *name)
{
/*
  int w = m_config.readNumEntry(name, -1);
  if(w > -1)
    pwordTreeView->setColumnWidth(col, w);
*/
}

void MyPasswordSafe::writeColumnWidth(int col, const char *name)
{
//  m_config.writeEntry(name, pwordTreeView->columnWidth(col));
}

void MyPasswordSafe::writeConfig()
{
  // save config settings   
  m_config.beginGroup("MyPasswordSafe");

  m_config.setValue("prefs/first_time", false);
  m_config.setValue("prefs/password_length", (int)PwordEditDlg::gen_pword_length);
  m_config.setValue("prefs/default_safe", m_default_safe);
  m_config.setValue("prefs/default_username", PwordEditDlg::default_user);
  m_config.setValue("prefs/generate/show", PwordEditDlg::generateAndShow());
  m_config.setValue("prefs/generate/fetch", PwordEditDlg::generateAndFetch());
  m_config.setValue("prefs/generate/on_new", PwordEditDlg::generateOnNew());
  m_config.setValue("prefs/clear_clipboard", clearClipboardOnExit());
  m_config.setValue("prefs/lock_on_minimize", lockOnMinimize());
  m_config.setValue("prefs/max_tries", m_max_tries);
  m_config.setValue("prefs/idle_timeout", m_idle_timeout);
  m_config.setValue("prefs/clear_timeout", (int)clearTimeOut());

  QSize sz = size();
  m_config.setValue("MainWindow/width", sz.width());
  m_config.setValue("MainWindow/height", sz.height());

  QPoint position = pos();
  m_config.setValue("MainWindow/left", position.x());
  m_config.setValue("MainWindow/top", position.y());

  for(int i = 0; MyPS_Column_Fields[i] != NULL; i++) {
    writeColumnWidth(i, MyPS_Column_Fields[i]);
  }

  m_config.endGroup();
}

/*
void MyPasswordSafe::dragObjectDropped(QMimeSource *drag, SafeListViewItem *target)
{
  DBGOUT("dragObjectDropped");
  QDomDocument doc("safe");
  if(SafeDragObject::decode(drag, doc)) {
    DBGOUT("Dragged data: " << endl << doc.toString().toAscii().data());

    // find the group to add the dragged data to
    SafeListViewGroup *lvi_parent = NULL;
    SafeGroup *safe_parent = m_safe;
    if(target) {
      if(target->rtti() == SafeListViewGroup::RTTI)
	lvi_parent = (SafeListViewGroup *)target;
      else
	lvi_parent = (SafeListViewGroup *)target->parent();

      if(lvi_parent)
	safe_parent = lvi_parent->group();
    }

    // add the items contained in the data
    QDomNode n = doc.firstChild();
    for(; !n.isNull(); n = n.nextSibling()) {
      if(n.isElement()) {
	QDomElement elem = n.toElement();
	QString tag_name = elem.tagName();
	if(tag_name == "item") {
	  SafeEntry *entry = new SafeEntry(safe_parent);
	  if(XmlSerializer::safeEntryFromXml(elem, entry)) {
	    pwordTreeView->itemAdded(entry);
	  }
	}
	else if(tag_name == "group") {
	  SafeGroup *group = new SafeGroup(safe_parent);
	  if(XmlSerializer::safeGroupFromXml(elem, group))
	    pwordTreeView->itemAdded(group);
	}
      }
    }

    m_safe->setChanged(true);
  }
}
*/

void MyPasswordSafe::clipboardCleared()
{
  statusBar()->showMessage(tr("Clipboard cleared"));
}

void MyPasswordSafe::helpAbout()
{
  helpAbout(0);
}

void MyPasswordSafe::slotSecondsIdle(int secs)
{
  if(m_idle_timeout > 0) { // zero means off
    int timeout = m_idle_timeout * 60;
    int count_down = timeout - secs;

    if(count_down == 30
       || count_down == 20
       || count_down <= 10) {
      statusBar()->showMessage(tr("Locking in %1 seconds.").arg(count_down), 2000);
    }

    if(count_down <= 0 && isVisible()) {
      DBGOUT("Locking due to idle");
      lock();
    }
  }
}

void MyPasswordSafe::checkMinimization()
{
  DBGOUT("isMinimized: " << (isMinimized() == QMainWindow::isMinimized()));
  if(lockOnMinimize()) {
    m_do_lock = isMinimized();
  }
}
