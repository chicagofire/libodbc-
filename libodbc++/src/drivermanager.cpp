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

#include <odbc++/drivermanager.h>
#include <odbc++/errorhandler.h>
#include <odbc++/connection.h>


using namespace odbc;
using namespace std;

SQLHENV DriverManager::henv_=SQL_NULL_HENV;
ErrorHandler* DriverManager::eh_=NULL;

//-1 means don't touch, 0 means wait forever, >0 means set it for every opened
//connection
int DriverManager::loginTimeout_=-1;


// Note: this will probably not work in all cases, 
// static constructors are known to cause problems

#ifdef ODBCXX_ENABLE_THREADS

static odbc::Mutex DMAccess;

#endif /* ODBCXX_ENABLE_THREADS */


void DriverManager::shutdown()
{
  ODBCXX_LOCKER(DMAccess);

  if(henv_!=SQL_NULL_HENV) {

    SQLRETURN r=ODBC3_C
      (SQLFreeHandle(SQL_HANDLE_ENV, henv_),
       SQLFreeEnv(henv_));

    switch(r) {
    case SQL_SUCCESS:
      break;
      
    case SQL_INVALID_HANDLE:
      // the check above should prevent this
      assert(false);
      break;

    case SQL_ERROR:
      // try to obtain an error description
      eh_->_checkEnvError(henv_, r, "Failed to shutdown DriverManager");
      break;
    }

    henv_=SQL_NULL_HENV;
    //if henv_ was valid, so is eh_
    delete eh_;
    eh_=NULL;
  }
}


//static
void DriverManager::_checkInit()
{
  //we lock around this, so two concurrent calls don't result
  //in several HENVs being allocated
  ODBCXX_LOCKER(DMAccess);

  if(henv_==SQL_NULL_HENV) {
    SQLRETURN r=
#if ODBCVER >= 0x0300
      SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&henv_)
#else
      SQLAllocEnv(&henv_)
#endif
      ;
    if(r!=SQL_SUCCESS && r!=SQL_SUCCESS_WITH_INFO) {
      throw SQLException
	("Failed to allocate environment handle");
    }
#if ODBCVER >= 0x0300
    // this should immediately follow an AllocEnv per ODBC3
    SQLSetEnvAttr(henv_,
		  SQL_ATTR_ODBC_VERSION,
		  (SQLPOINTER)SQL_OV_ODBC3,
		  SQL_IS_UINTEGER);
#endif
    //don't collect warnings
    eh_=new ErrorHandler(false);
  }
}

//static
void DriverManager::setLoginTimeout(int to)
{
  ODBCXX_LOCKER(DMAccess);
  loginTimeout_=to;
}

//static
int DriverManager::getLoginTimeout()
{
  ODBCXX_LOCKER(DMAccess);
  return loginTimeout_;
}

//static
// This assumes _checkInit has been called
Connection* DriverManager::_createConnection()
{
  SQLHDBC hdbc;
  SQLRETURN r;
  
  //allocate a handle
  r=
#if ODBCVER >= 0x0300
    SQLAllocHandle(SQL_HANDLE_DBC,henv_,&hdbc)
#else
    SQLAllocConnect(henv_,&hdbc)
#endif
    ;

  eh_->_checkEnvError(henv_,r,"Failed to allocate connection handle");
  
  Connection* con=new Connection(hdbc);
  {
    ODBCXX_LOCKER(DMAccess); //since we read loginTimeout_
    
    //apply the login timeout. -1 means do-not-touch (the default)
    if(loginTimeout_>-1) {
      
      con->_setNumericOption(
#if ODBCVER < 0x0300
			     SQL_LOGIN_TIMEOUT
#else
			     SQL_ATTR_LOGIN_TIMEOUT
#endif
			     
			     ,(SQLUINTEGER)loginTimeout_);
    }
  } // end of lock scope
  return con;
}



//static
Connection* DriverManager::getConnection(const ODBCXX_STRING& connectString)
{
  DriverManager::_checkInit();

  Connection* con=DriverManager::_createConnection();
  con->_connect(connectString);
  return con;
}


//static
Connection* DriverManager::getConnection(const ODBCXX_STRING& dsn,
					 const ODBCXX_STRING& user,
					 const ODBCXX_STRING& password)
{
  DriverManager::_checkInit();
  Connection* con=DriverManager::_createConnection();
  con->_connect(dsn,user,password);
  return con;
}


DataSourceList* DriverManager::getDataSources()
{
  DriverManager::_checkInit();
  
  SQLRETURN r;
  SQLSMALLINT dsnlen,desclen;
  
  DataSourceList* l=new DataSourceList();

  char dsn[SQL_MAX_DSN_LENGTH+1];
  char desc[256];

  {
    ODBCXX_LOCKER(DMAccess);
    r=SQLDataSources(henv_,
		     SQL_FETCH_FIRST,
		     (SQLCHAR*)dsn,
		     SQL_MAX_DSN_LENGTH+1,
		     &dsnlen,
		     (SQLCHAR*)desc,
		     256,
		     &desclen);
    
    eh_->_checkEnvError(henv_,r,"Failed to obtain a list of datasources");
    
    while(r==SQL_SUCCESS || r==SQL_SUCCESS_WITH_INFO) {
      l->insert(l->end(),new DataSource(dsn,desc));
      
      r=SQLDataSources(henv_,
		       SQL_FETCH_NEXT,
		       (SQLCHAR*)dsn,
		       SQL_MAX_DSN_LENGTH+1,
		       &dsnlen,
		       (SQLCHAR*)desc,
		       256,
		       &desclen);
      
      eh_->_checkEnvError(henv_,r,"Failed to obtain a list of datasources");
    }
  } // lock scope end
  return l;
}

#define MAX_DESC_LEN 64
#define MAX_ATTR_LEN 1024 

DriverList* DriverManager::getDrivers()
{
  DriverManager::_checkInit();

  SQLRETURN r;
  DriverList* l=new DriverList();

  
  char desc[MAX_DESC_LEN];
  char attrs[MAX_ATTR_LEN];

  SQLSMALLINT dlen,alen;

  {
    ODBCXX_LOCKER(DMAccess);
    r=SQLDrivers(henv_,
		 SQL_FETCH_FIRST,
		 (SQLCHAR*)desc,
		 MAX_DESC_LEN,
		 &dlen,
		 (SQLCHAR*)attrs,
		 MAX_ATTR_LEN,
		 &alen);
    
    eh_->_checkEnvError(henv_,r,"Failed to obtain a list of drivers");
    
    while(r==SQL_SUCCESS || r==SQL_SUCCESS_WITH_INFO) {
      vector<ODBCXX_STRING> attr;
      unsigned int i=0, last=0;
      
      //find our attributes
      if(attrs[0]!=0) {
	do {
	  while(attrs[++i]!=0);
	  attr.push_back(ODBCXX_STRING_CL(&(attrs[last]),i-last));
	  last=i+1;
	} while(attrs[last]!=0);
      }
      
      Driver* d=new Driver(desc,attr);
      l->insert(l->end(),d);
      
      r=SQLDrivers(henv_,
		   SQL_FETCH_NEXT,
		   (SQLCHAR*)desc,
		   MAX_DESC_LEN,
		   &dlen,
		   (SQLCHAR*)attrs,
		   MAX_ATTR_LEN,
		   &alen);
      
      eh_->_checkEnvError(henv_,r,"Failed to obtain a list of drivers");
    }
  } // lock scope end
  
  return l;
}
