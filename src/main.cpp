/* $Header: /home/cvsroot/MyPasswordSafe/src/main.cpp,v 1.7 2004/11/02 21:25:57 nolan Exp $
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
#include <iostream>
#include <qapplication.h>
#include <qtranslator.h>
#include <qtextcodec.h>
#include <qmime.h>
#include <qimage.h>
#include "config.h"
#include "safe.hpp"
#include "startupdlg.hpp"
#include "mypasswordsafe.hpp"

using namespace std;

const char *locale_dir = PREFIX "/share/MyPasswordSafe/locale";

bool doStartupDialog(MyPasswordSafe *myps, int argc, char *argv[])
{
  StartupDlg dlg(myps);

  if(argc > 1) {
    // use the last argument
    dlg.setFilename(argv[argc - 1]);
    dlg.setAction(StartupDlg::Browse);
  }
  else if(myps->getDefaultSafe().length() > 0) {
    dlg.setAction(StartupDlg::OpenDefault);
  }
  else {
    dlg.setAction(StartupDlg::CreateNew);
  }

  if(dlg.exec() == StartupDlg::Rejected)
    return false;
  else
    return true;
}

int main( int argc, char ** argv )
{
  QApplication a( argc, argv );
  a.setOrganizationName(ORGNAME);
  a.setOrganizationDomain(ORGDOMAIN);
  a.setApplicationName(APPNAME);

  MyPasswordSafe myps;

  QTextCodec *codec = QTextCodec::codecForLocale();
  QTranslator qt(0);
#ifdef DEBUG
  cout << "Using locale: " << codec->name() << endl;
#endif
  qt.load(QString("qt_") + codec->name(), locale_dir);
  a.installTranslator(&qt);
  QTranslator myapp(0);
  if(!myapp.load(QString("mypasswordsafe_") + codec->name(),
		 locale_dir)) {
#ifdef DEBUG
    cout << "No locale file for " << codec->name()
	 << " found in " << locale_dir << endl;
#endif
  }
  else {
    a.installTranslator(&myapp);
  }

  if(myps.firstTime()) {
    myps.helpAbout(1); // show license
  }

  if(!doStartupDialog(&myps, argc, argv)) {
    return 0;
  }

  myps.show();
  //a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );
  a.connect(&myps, SIGNAL(quit()), &a, SLOT(quit()));
  return a.exec();
}
