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

#if ODBCVER < 0x0300

// ODBC v2

// static
DriverMessage* DriverMessage::fetchMessage(SQLHENV henv,
					   SQLHDBC hdbc,
					   SQLHSTMT hstmt)
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
    // we have a message
    break;

  case SQL_INVALID_HANDLE:
    // internal whoops
    delete m;
    throw SQLException
      ("[libodbc++]: fetchMessage() called with invalid handle");
    break;

  case SQL_ERROR:
    // this should be extremely rare, but still..
    delete m;
    throw SQLException
      ("[libodbc++]: SQLError() returned SQL_ERROR");
    break;

  default:
    // we got no message it seems
    delete m;
    m=NULL;
    break;
  }

  return m;
}
				   

#else

// ODBC v3

// static
DriverMessage* DriverMessage::fetchMessage(SQLINTEGER handleType,
					   SQLHANDLE h,
					   int idx)
{
  DriverMessage* m=new DriverMessage();
  
  SQLSMALLINT tmp;

  SQLRETURN r=SQLGetDiagRec(handleType, h, idx,
			    (SQLCHAR*)m->state_,
			    &m->nativeCode_,
			    (SQLCHAR*)m->description_,
			    SQL_MAX_MESSAGE_LENGTH-1,
			    &tmp);

  switch(r) {
  case SQL_SUCCESS:
    // we have a message
    break;

  case SQL_INVALID_HANDLE:
    // internal whoops
    delete m;
    throw SQLException
      ("[libodbc++]: fetchMessage() called with invalid handle");
    break;

  case SQL_ERROR:
    // this should be extremely rare, but still..
    delete m;
    throw SQLException
      ("[libodbc++]: SQLGetDiagRec() returned SQL_ERROR");
    break;

  default:
    // we got no message it seems
    delete m;
    m=NULL;
    break;
  }

  return m;
}

#endif


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

  DriverMessage* m=DriverMessage::fetchMessage(henv, hdbc, hstmt);
  
  if(ret==SQL_ERROR) {
    
    Deleter<DriverMessage> _m(m);

    // fixme: we should fetch all available messages instead
    // of only the first one
    ODBCXX_STRING errmsg("");
    if(ODBCXX_STRING_LEN(what)>0) {
      errmsg=what+": ";
    }

    if(m!=NULL) {
      errmsg+=m->getDescription();
      throw SQLException(errmsg, 
			 m->getSQLState(), 
			 m->getNativeCode());
    } else {
      errmsg+="No description available";
      throw SQLException(errmsg);
    }

  } else if(ret==SQL_SUCCESS_WITH_INFO) {

    while(m!=NULL) {
      this->_postWarning(new SQLWarning(*m));
      delete m;
      m=DriverMessage::fetchMessage(henv, hdbc, hstmt);
    }

  } else {
    
    delete m;
    
  }

}


#else

void ErrorHandler::_checkErrorODBC3(SQLINTEGER handleType, SQLHANDLE handle,
				    SQLRETURN ret, 
				    const ODBCXX_STRING& what)
{

  int idx=1;

  DriverMessage* m=DriverMessage::fetchMessage(handleType, handle, idx);
  
  if(ret==SQL_ERROR) {

    Deleter<DriverMessage> _m(m);

    // fixme: we should fetch all available messages instead
    // of only the first one
    ODBCXX_STRING errmsg("");
    if(ODBCXX_STRING_LEN(what)>0) {
      errmsg=what+": ";
    }

    if(m!=NULL) {
      errmsg+=m->getDescription();
      throw SQLException(errmsg, 
			 m->getSQLState(), 
			 m->getNativeCode());
    } else {
      errmsg+="No description available";
      throw SQLException(errmsg);
    }

  } else if(ret==SQL_SUCCESS_WITH_INFO) {

    while(m!=NULL) {
      _postWarning(new SQLWarning(*m));
      delete m;
      m=DriverMessage::fetchMessage(handleType, handle, ++idx);
    }

  } else {
    
    delete m;
    
  }

}

#endif // ODBCVER < 0x0300
