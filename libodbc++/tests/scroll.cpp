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


/*
  This should work with almost any almost-compliant database out there,
  providing it supports scrollable cursors.
 */


#include <odbc++/drivermanager.h>
#include <odbc++/connection.h>
#include <odbc++/databasemetadata.h>
#include <odbc++/resultset.h>
#include <odbc++/resultsetmetadata.h>
#include <odbc++/preparedstatement.h>

#include <iostream>

using namespace odbc;
using namespace std;

#if !defined(ODBCXX_QT)
# include <strstream>
#else

# undef ASSERT

ostream& operator<<(ostream& o, const QString& s)
{
  o << ODBCXX_STRING_CSTR(s);
  return o;
}
#endif

static int assertionsFailed=0;

#define ASSERT(x)						\
do {								\
  if(!(x)) {							\
    cerr << "Assertion \"" << #x << "\" failed" << endl;	\
    assertionsFailed++;                                         \
  }								\
} while(false)

#define NAME_PREFIX "odbcxx_"

#define TABLE_NAME NAME_PREFIX "test"

#define TABLE_ROWS 1000

static void commit(Connection* con)
{
  if(con->getMetaData()->supportsTransactions()) {
    con->commit();
  }
}

static void createStuff(Connection* con)
{
  // create our table
  Statement* stmt=con->createStatement();
  stmt->executeUpdate
    ("create table " TABLE_NAME "("
     "id integer not null primary key, "
     "name varchar(40) not null)");
  
  cout << "Table " << TABLE_NAME << " created." << endl;
  
  delete stmt;
}

// Drops the database objects.

static void dropStuff(Connection* con)
{
  Statement* stmt=con->createStatement();
  try {
    stmt->executeUpdate("drop table " TABLE_NAME);
    cout << "Dropped table " << TABLE_NAME << endl;
  } catch(SQLException& e) {
  }
  
  delete stmt;
}

static ODBCXX_STRING makeName(int n)
{
#if !defined(ODBCXX_QT)
  strstream ss;
  ss << "This is row number " << n;
  return string(ss.str(),ss.pcount());
#else
  QString s("This is row number ");
  s+=QString::number(n);
  return s;
#endif
}


static void populate(Connection* con)
{
  {
    PreparedStatement* pstmt=con->prepareStatement
      ("insert into " TABLE_NAME " (id,name) values(?,?)");
    for(int i=0; i<TABLE_ROWS; i++) {
      pstmt->setInt(1,i);
      pstmt->setString(2,makeName(i));
      pstmt->executeUpdate();
    }
    delete pstmt;
    commit(con);
    cout << "Inserted " << TABLE_ROWS << " rows." << endl;
  }
}


static void compare(Connection* con)
{
  // decide whether we should use a scroll insensitive 
  // or a scroll sensitive cursor
  
  int rstype;
  int rsconc;
  DatabaseMetaData* md=con->getMetaData();

  if(md->supportsResultSetType(ResultSet::TYPE_SCROLL_INSENSITIVE)) {
    rstype=ResultSet::TYPE_SCROLL_INSENSITIVE;
  } else if(md->supportsResultSetType(ResultSet::TYPE_SCROLL_SENSITIVE)) {
    rstype=ResultSet::TYPE_SCROLL_SENSITIVE;
  } else {
    cout << "Skipping compare, data source does not support scrollable cursors" 
	 << endl;
    return;
  }


  if(md->supportsResultSetConcurrency(rstype,ResultSet::CONCUR_READ_ONLY)) {
    // this is all we need
    rsconc=ResultSet::CONCUR_READ_ONLY;
  } else {
    rsconc=ResultSet::CONCUR_UPDATABLE;
  }
  
  Statement* stmt=con->createStatement
    (rstype,rsconc);
  ResultSet* rs=stmt->executeQuery
    ("select id,name from " TABLE_NAME);

  ASSERT(rs->isBeforeFirst());
  ASSERT(rs->first());
  ASSERT(!rs->isBeforeFirst());
  ASSERT(rs->isFirst());
  
  ASSERT(rs->last());
  ASSERT(rs->isLast());
  ASSERT(!rs->isAfterLast());
  rs->afterLast();
  ASSERT(rs->isAfterLast());

  ASSERT(rs->previous());
  ASSERT(rs->isLast());
  
  
  cout << "Positioned on the last row (" 
       << rs->getRow() << ")" << endl;
  int i=TABLE_ROWS;
  
  do {
    i--;
    ODBCXX_STRING name(makeName(i));
    ASSERT(rs->getInt(1) == i);
    ASSERT(rs->getString(2)==name);
  } while(rs->previous());
  ASSERT(i==0);
  ASSERT(rs->isBeforeFirst());
  cout << TABLE_ROWS << " rows checked with expected values." << endl;

  delete stmt; //will kill rs
    
}





int main(int argc, char** argv)
{
  if(argc!=2 && argc!=4) {
    cerr << "Usage: " << argv[0] << " connect-string" << endl
	       << "or     " << argv[0] << " dsn username password" << endl;
    return 0;
  }
  try {
    Connection* con;
    if(argc==2) {
      cout << "Connecting to " << argv[1] << "..." << flush;
      con=DriverManager::getConnection(argv[1]);
    } else {
      cout << "Connecting to dsn=" << argv[1]
	   << ", uid=" << argv[2] 
	   << ", pwd=" << argv[3] << "..." << flush;
      con=DriverManager::getConnection(argv[1],argv[2],argv[3]);
    }
    cout << " done." << endl;

    // we don't want autocommit
    if(con->getMetaData()->supportsTransactions()) {
      con->setAutoCommit(false);
    }
//      con->setTraceFile("/tmp/fisk");
//      con->setTrace(true);

    dropStuff(con);
    createStuff(con);
    
    populate(con);
    compare(con);
    commit(con);
    
    dropStuff(con);


    commit(con);
    delete con;

    if(assertionsFailed>0) {
      cout << assertionsFailed << " assertions failed." << endl;
    }
  } catch(SQLException& e) {
    cerr << endl << e.getMessage() << endl;
    return 1;
  }

  return 0;
}
