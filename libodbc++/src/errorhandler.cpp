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

#include <odbc++/errorhandler.h>

using namespace odbc;
using namespace std;

struct ErrorHandler::PD {
#ifdef ODBCXX_ENABLE_THREADS
    Mutex access_;
#endif
};

ErrorHandler::ErrorHandler(bool cw)
  :pd_(new PD()),
   warnings_(new WarningList()),
   collectWarnings_(cw)
{
}

ErrorHandler::~ErrorHandler()
{
  delete warnings_;
  delete pd_;
}

void ErrorHandler::clearWarnings()
{
  ODBCXX_LOCKER(pd_->access_);
  if(!warnings_->empty()) {
    WarningList* old=warnings_;
    warnings_=new WarningList();
    delete old;
  }
}


WarningList* ErrorHandler::getWarnings()
{
  ODBCXX_LOCKER(pd_->access_);
  WarningList* ret=warnings_;
  warnings_=new WarningList();
  return ret;
}


void ErrorHandler::_postWarning(SQLWarning* w)
{
  ODBCXX_LOCKER(pd_->access_);
  
  if(collectWarnings_) {
    warnings_->insert(warnings_->end(),w);
    
    if(warnings_->size()>MAX_WARNINGS) {
      //nuke oldest warning
      WarningList::iterator i=warnings_->begin();
      delete *i;
      warnings_->erase(i);
    }
  } else {
    delete w;
  }
}


// the following assume there is a warning/error to fetch
// they should only be called for SQL_ERROR and SQL_SUCCESS_WITH_INFO

#if ODBCVER < 0x0300

void ErrorHandler::_checkErrorODBC2(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt,
				    SQLRETURN ret, 
				    const ODBCXX_STRING& what)
{
  DriverMessage* m=new DriverMessage();
  SQLSMALLINT tmp;
  SQLRETURN r=SQLError(henv, hdbc, hstmt,
		       (SQLCHAR*)m->state_,
		       &m->nativeCode_,
		       (SQLCHAR*)m->description_,
		       SQL_MAX_MESSAGE_LENGTH-1,
		       &tmp);
  switch(r) {
  case SQL_SUCCESS:
    break;

  case SQL_INVALID_HANDLE:
    delete m;
    m=NULL;
    throw SQLException
      ("[libodbc++]: ErrorHandler::_checkErrorODBC2() called with invalid handle");
    break;

  default:
    delete m;
    m=NULL;
    break;
  }

  if(ret==SQL_ERROR) {
    // construct an error description based on parameter what and optionally the
    // driver message
    ODBCXX_STRING errmsg("");
    if(ODBCXX_STRING_LEN(what)>0) {
      errmsg=what+": ";
    }
    
    if(m!=NULL) {
      errmsg+=m->getDescription();
    } else {
      errmsg+="No description available";
    }

    delete m;

    throw SQLException(errmsg);

  } else if(ret==SQL_SUCCESS_WITH_INFO && m!=NULL) {
    // we got ourselves a warning
    this->_postWarning(new SQLWarning(*m));
    
  } 

  delete m;
}


#else

void ErrorHandler::_checkErrorODBC3(SQLINTEGER handleType, SQLHANDLE handle,
				    SQLRETURN ret, 
				    const ODBCXX_STRING& what)
{
  DriverMessage* m=new DriverMessage();
  SQLSMALLINT tmp;
  
  SQLRETURN r=SQLGetDiagRec(handleType,handle,
			    1, //record number
			    (SQLCHAR*)m->state_,
			    &m->nativeCode_,
			    (SQLCHAR*)m->description_,
			    SQL_MAX_MESSAGE_LENGTH-1,
			    &tmp);
  
  switch(r) {
  case SQL_SUCCESS:
    break;
    
  case SQL_INVALID_HANDLE:
    delete m;
    m=NULL;
    throw SQLException
      ("[libodbc++]: ErrorHandler::_checkErrorODBC3() called with invalid handle");
    break;
    
  default:
    delete m;
    m=NULL;
    break;
  }

  if(ret==SQL_ERROR) {
    // construct an error description based on parameter what and optionally the
    // driver message
    ODBCXX_STRING errmsg("");
    if(what.length()>0) {
      errmsg=what+": ";
    }
    
    if(m!=NULL) {
      errmsg+=m->getDescription();
    } else {
      errmsg+="No description available";
    }

    delete m;

    throw SQLException(errmsg);

  } else if(ret==SQL_SUCCESS_WITH_INFO && m!=NULL) {
    // we got ourselves a warning
    this->_postWarning(new SQLWarning(*m));
    
  } 

  delete m;
}

#endif // ODBCVER < 0x0300
