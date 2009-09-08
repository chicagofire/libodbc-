/* 
   This file is part of libodbc++.
   
   Copyright (C) 1999-2000 Manush Dodunekov <manush@stendahls.net>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "connectwindow.h"
#include <iostream>

#include <odbc++/drivermanager.h>
#include <odbc++/connection.h>

#include <qframe.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qmessagebox.h>

using namespace odbc;

ConnectWindow::ConnectWindow(QWidget* parent,
			     const char* name)
  :QDialog(parent),
   con(NULL)
{
  setWindowTitle("Connect to datasource");

  QVBoxLayout* vl=new QVBoxLayout(this);
  QGridLayout* gl=new QGridLayout(this);
  QHBoxLayout* hl=new QHBoxLayout();

  vl->addLayout(gl);
  vl->addLayout(hl);


  dataSources=new QComboBox(this);
  dataSources->setEditable(true);
  gl->addWidget(dataSources,0,1);

  
  userBox=new QLineEdit(this);
  gl->addWidget(userBox,1,1);

  passwordBox=new QLineEdit(this);
  passwordBox->setEchoMode(QLineEdit::Password);
  gl->addWidget(passwordBox,2,1);

  // Create label and set buddy for datasource
  QLabel *dataSourceLabel = new QLabel("&Datasource:",this);
  dataSourceLabel->setBuddy(dataSources);
  gl->addWidget(dataSourceLabel,0,0);

  // Create label and set buddy for username
  QLabel *userBoxLabel = new QLabel("&Username:",this);
  userBoxLabel->setBuddy(userBox);
  gl->addWidget(userBoxLabel,1,0);

  // Create label and set buddy for password
  QLabel *passwordBoxLabel = new QLabel("&Password:",this);
  passwordBoxLabel->setBuddy(passwordBox);
  gl->addWidget(passwordBoxLabel,2,0);
  
  QPushButton* ok=new QPushButton("Connect",this);
  ok->setDefault(true);
  QPushButton* cancel=new QPushButton("Cancel",this);

  hl->addWidget(ok);
  hl->addWidget(cancel);

  connect(ok,SIGNAL(clicked()),this,SLOT(tryConnect()));
  connect(cancel,SIGNAL(clicked()),this,SLOT(reject()));

  QSize sh=this->sizeHint();
  this->setMaximumHeight(sh.height());

  this->setup();
}

void ConnectWindow::setup()
{
  DataSourceList* l=DriverManager::getDataSources();
  for(DataSourceList::iterator i=l->begin();
      i!=l->end(); i++) {
    DataSource* ds=*i;
    dataSources->addItem(QString::fromStdString(ds->getName()));
  }
  delete l;

  dataSources->setFocus();
}


ConnectWindow::~ConnectWindow()
{
}

void ConnectWindow::tryConnect()
{
  try {
    con=DriverManager::getConnection
      (dataSources->currentText().toStdString(),
       userBox->text().toStdString(),
       passwordBox->text().toStdString());
    
    this->accept();

  } catch(SQLException& e) {
    QMessageBox::warning(this,"Connect failed",
			 QString::fromStdString(e.getMessage()),
			 QMessageBox::Ok,0,0);
    con=NULL;
  }
}
