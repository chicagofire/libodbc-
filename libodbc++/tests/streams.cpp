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
  This tests the ResultSet's (get|update)(Ascii|Binary)Stream.
  Expected to be run against an Oracle database using Openlink's driver
  You must have create table/drop table rights.

  This test does the following:
  1. Create a table containing a number column and a long raw column.
  2. Insert some rows into the table using a PreparedStatement
  3. Insert some more rows into the table using a ResultSet
  4. Compare the contents of the table with values from the
  value generator.
  5. Update all the rows in the table using a ResultSet
  6. Compare again.
  7. Drop table

  The value generator simply gives different binary chunks of different sizes
  depending on the row number.

  This depends on a datasource that keeps statements open across 
  commits.
 */

#if !defined(ODBCXX_QT)

#include <odbc++/drivermanager.h>
#include <odbc++/connection.h>
#include <odbc++/resultset.h>
#include <odbc++/resultsetmetadata.h>
#include <odbc++/preparedstatement.h>

#include <iostream>
#include <strstream>

using namespace odbc;
using namespace std;

#define NAME_PREFIX "odbcxx_"

#define TABLE_NAME NAME_PREFIX "test"

const int SIZE_MULTIPLIER = 1024*20; //1st row=20k, 2nd row=40k ...
const int MAX_SIZE = 1024*1024; //max 1 megabyte per value
const int TEST_ROWS = 20; // 

typedef pair<istream*,int> TestStream;


static int assertionsFailed=0;

#define ASSERT(x)						\
do {								\
  if(!(x)) {							\
    cerr << "Assertion \"" << #x << "\" failed" << endl;	\
    assertionsFailed++;                                         \
  }								\
} while(false)



// dumps all warnings for a given ErrorHandler
static void dumpWarnings(ErrorHandler* eh, const string& who)
{
  WarningList* warnings=eh->getWarnings();
  if(!warnings->empty()) {
    cout << who << " had " << warnings->size() << " warnings." << endl;
    for(WarningList::iterator i=warnings->begin();
	i!=warnings->end(); i++) {
      SQLWarning& w=**i;
      cout << "SQLState   : " << w.getSQLState() << endl;
      cout << "Description: " << w.getMessage() << endl;
    }
  }
}


// Creates the database objects needed

static void createStuff(Connection* con)
{
  // create our table
  Statement* stmt=con->createStatement();
  stmt->executeUpdate
    ("create table " TABLE_NAME "("
     "id number constraint " TABLE_NAME "_pk_id primary key, "
     "content long raw not null)");
  
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
    //ignore
  }
  
  delete stmt;
}


class ValueGen {
private:
  size_t cnt_;
  size_t rows_;
  
  char current[MAX_SIZE];

public:
  ValueGen(size_t rows)
    :cnt_(0), rows_(rows) {}
  ~ValueGen() {}

  bool next() {
    return ((++cnt_) <= rows_);
  }

  int id() { 
    return cnt_; 
  }

  TestStream* b() {
    // cnt_ can never be 0 here
    int len=((cnt_)*SIZE_MULTIPLIER)%MAX_SIZE;
    
    for(int i=0; i<len; i++) {
      current[i]=((i+cnt_)%256);
    }
    istrstream* s=new istrstream(current,len);
    return new TestStream(s,len);
  }
};


// this compares two streams
inline bool compareStreams(istream* s1, istream* s2, int len)
{
  char c1, c2;
  size_t cnt=0;
  while(s1->get(c1) && s2->get(c2)) {
    cnt++;
    if(c1!=c2)
      return false;
  }
  return (cnt==len);
}



static void populate(Connection* con)
{
  PreparedStatement* pstmt=con->prepareStatement
    ("insert into " TABLE_NAME "(id,content) values(?,?)");

  ValueGen vg(TEST_ROWS);
  int cnt=0;
  while(cnt<TEST_ROWS/2 && vg.next()) {
    pstmt->setInt(1,vg.id());
    TestStream* s=vg.b();
    pstmt->setBinaryStream(2,s->first,s->second);
    pstmt->executeUpdate();
    
    delete s->first;
    delete s;

    //we commit after every row, since we can reach pretty 
    //big chunks of data
    con->commit(); 
    cnt++;
  }

  cout << "Inserted " << cnt << " rows using a PreparedStatement" << endl;
  delete pstmt;

  // go cursors
  Statement* stmt=con->createStatement(ResultSet::TYPE_SCROLL_SENSITIVE,
				       ResultSet::CONCUR_UPDATABLE);
  ResultSet* rs=stmt->executeQuery
    ("select id,content from " TABLE_NAME);

  rs->next();
  rs->moveToInsertRow();
  while(vg.next()) {
    rs->updateInt(1,vg.id());
    TestStream* s=vg.b();
    rs->updateBinaryStream(2,s->first,s->second);

    rs->insertRow();

    cout << "Did a row" << endl;

    delete s->first;
    delete s;

    con->commit(); 
  }
  
  cout << "Inserted " << TEST_ROWS/2 << " rows using a ResultSet" << endl;
  
  delete rs;
  delete stmt;
}


static void compare(Connection* con)
{
  Statement* stmt=con->createStatement();
  ResultSet* rs=stmt->executeQuery("select id,content from " TABLE_NAME
				   " order by id");
  
  ValueGen vg(TEST_ROWS);
  int cnt=0;

  cout << "Comparing values with database (can take a while)..." << flush;
  
  while(rs->next() && vg.next()) {
    cnt++;
    ASSERT(vg.id()==rs->getInt(1));

    TestStream* vgStream=vg.b();
    istream* rsStream=rs->getBinaryStream(2);
    
    ASSERT(compareStreams(vgStream->first,rsStream,vgStream->second));

    delete vgStream->first;
    delete vgStream;
    
    //rsStream is taken care of by the result set
  }

  ASSERT(cnt==TEST_ROWS);

  delete rs;
  delete stmt;

  cout << " done." << endl;
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
    con->setAutoCommit(false);

    dropStuff(con);
    createStuff(con);

    populate(con);
    compare(con);
    
    dropStuff(con);


    con->commit();
    delete con;
  } catch(SQLException& e) {
    cerr << endl << e.getMessage() << endl;
  }

  return 0;
}

#else

int main() { return 0; }

#endif
