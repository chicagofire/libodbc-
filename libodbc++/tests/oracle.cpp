#if !defined(ODBCXX_QT)

#include <odbc++/drivermanager.h>
#include <odbc++/connection.h>
#include <odbc++/resultset.h>
#include <odbc++/resultsetmetadata.h>
#include <odbc++/callablestatement.h>
#include <odbc++/databasemetadata.h>

#include <strstream>
#include <iostream>

using namespace std;
using namespace odbc;

// note: this must be even
const int TABLE_ROWS=500;

#define PREFIX "odbcxx_"

const string tableName=PREFIX "test";
const string procName=PREFIX "ptest";
const string funcName=PREFIX "ftest";

int assertionsFailed=0;

#define ASSERT(x) 					\
if(!(x)) { 						\
  cout << __FILE__ << ":" << __LINE__ << ": " 		\
       << "Assertion " << #x << " failed." << endl; 	\
  assertionsFailed++; 					\
}

static void dumpWarnings(Statement* stmt)
{
  WarningList* warnings=stmt->getWarnings();
  for(WarningList::iterator i=warnings->begin();
      i!=warnings->end(); i++) {
    cout << "Warning: " << (*i)->getMessage() << endl;
  }
  delete warnings;
}

static void dropStuff(Connection* con)
{
  Statement* stmt=con->createStatement();
  try {
    stmt->executeUpdate("drop table "+tableName);
    cout << "Dropped table " << tableName << endl;
    dumpWarnings(stmt);
  } catch(SQLException& e) {}

  try {
    stmt->executeUpdate("drop procedure "+procName);
    cout << "Dropped procedure " << procName << endl;
    dumpWarnings(stmt);
  } catch(SQLException& e) {}

  try {
    stmt->executeUpdate("drop function "+funcName);
    cout << "Dropped function " << funcName << endl;
    dumpWarnings(stmt);
  } catch(SQLException& e) {}
  delete stmt;
}

static void createStuff(Connection* con)
{
  Statement* stmt=con->createStatement();

  // create the table
  stmt->executeUpdate("create table "+tableName+" ("
		      "id number(4) not null primary key, "
		      "name varchar2(22) not null, "
		      "ts date)");
  
  dumpWarnings(stmt);
  cout << "Created table " << tableName << endl;

  // create the procedure
  stmt->executeUpdate
    ("create procedure "+procName+
     " (a in integer, b out integer, s in out varchar2) as "
     "begin "
     "  b:=a*2; "
     "  s:= s || ': ' || a || '*2=' || b; "
     "end;");

  dumpWarnings(stmt);
  cout << "Created procedure " << procName << endl;

  // create the function
  stmt->executeUpdate
    ("create function "+funcName+" (a in number, s in out varchar2) "
     "return number as "
     "b number; "
     "begin "
     "  b:=a*2; "
     "  s:= s || ': ' || a || '*2=' || b; "
     "  return b; "
     "end;");

  dumpWarnings(stmt);
  cout << "Created function " << funcName << endl;

  delete stmt;
}

static void testProc(Connection* con)
{
  CallableStatement* stmt=con->prepareCall
    ("{call "+procName+"(?,?,?)}");
  stmt->setInt(1,22);
  stmt->registerOutParameter(2,Types::INTEGER);
  stmt->setString(3, "Okay");
  stmt->executeUpdate();
  
  ASSERT(stmt->getInt(2)==44);
  ASSERT(stmt->getString(3)=="Okay: 22*2=44");
  delete stmt;
}

static void testFunc(Connection* con)
{
  CallableStatement* stmt=con->prepareCall
    ("{?=call "+funcName+"(?,?)}");

  stmt->registerOutParameter(1,Types::INTEGER);
  stmt->setInt(2,22);
  stmt->setString(3,"Okay");
  stmt->executeUpdate();

  ASSERT(stmt->getInt(1)==44);
  ASSERT(stmt->getString(3)=="Okay: 22*2=44");
  delete stmt;
}


static void testTable(Connection* con) 
{
  int i=0;
  int driverVersion=con->getMetaData()->getDriverMajorVersion();

  if(driverVersion<3) {
    // insert the first row using a prepared statement
    // ODBC 2 drivers can't do inserts before a fetch is done.
    // some can't do inserts if the result set is not on a 
    // real row.
    PreparedStatement* pstmt=con->prepareStatement
      ("insert into "+tableName+" (id,name,ts) values(?,?,?)");
    pstmt->setInt(1,i);
    pstmt->setString(2,"This is row number 0");
    {
      Timestamp ts;
      pstmt->setTimestamp(3,ts);
    }
    pstmt->executeUpdate();
    cout << "Inserted row 0" << endl;
    delete pstmt;
    i++;
  }

  // populate our table using a ResultSet
  Statement* stmt=con->createStatement
    (ResultSet::TYPE_SCROLL_SENSITIVE, ResultSet::CONCUR_UPDATABLE);

  // set fetch size to something useful
  stmt->setFetchSize(10);

  ResultSet* rs=stmt->executeQuery("select id,name,ts from "+tableName);

  if(driverVersion<3) {
    // position ourselves on a real row
    ASSERT(rs->next());
  }

  rs->moveToInsertRow();

  while(i<TABLE_ROWS) {
    strstream ns;
    ns << "This is row number " << i;
    string name(ns.str(),ns.pcount());
    rs->updateInt(1,i);
    rs->updateString(2,name);
    rs->updateTimestamp(3,Timestamp());
    rs->insertRow();
    cout << "Inserted row " << i << endl;
    i++;
  }
  rs->moveToCurrentRow();
  delete rs;

  con->commit();

  rs=stmt->executeQuery("select id,name from "+tableName);

  i=0;
  while(rs->next()) {
    cout << "Checking row " << i << endl;
    strstream ns;
    ns << "This is row number " << i;
    string name(ns.str(),ns.pcount());
    ASSERT(rs->getString("name")==name);

    ASSERT(rs->getInt("id")==i);
    i++;
  }

  delete rs;
  cout << "Check done" << endl;

  rs=stmt->executeQuery("select id,name,ts from "+tableName);
  i=0;
  while(rs->next()) {
    if((i%2)==1) {
      strstream ns;
      ns << "This IS row number " << i;
      string name(ns.str(),ns.pcount());

      rs->updateString("name",name);
      Timestamp ts;
      rs->updateTimestamp("ts",ts);
      rs->updateRow();
      cout << "Updated row " << i << endl;
    } else {
      rs->deleteRow();
      cout << "Deleted row " << i << endl;
    }
    i++;
  }
  delete rs;

  rs=stmt->executeQuery("select id,name,ts from "+tableName);
  i=1;
  while(rs->next()) {
    strstream ns;
    ns << "This IS row number " << i;
    string name(ns.str(),ns.pcount());

    ASSERT(rs->getString("name")==name);
    ASSERT(rs->getInt(1)==i);

    i+=2;
  }

  ASSERT(i==TABLE_ROWS+1);

  delete stmt;

  con->commit();
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
    
    int numTests=3;
    int failedTests=0;

    con->setAutoCommit(false);
    dropStuff(con);
    createStuff(con);
    try {
      testProc(con);
    } catch(SQLException& e) {
      cout << "Procedure test FAILED: " << e.getMessage() << endl;
      failedTests++;
    }
    
    try {
      testFunc(con);
    } catch(SQLException& e) {
      cout << "Function test FAILED: " << e.getMessage() << endl;
      failedTests++;
    }

    try {
      testTable(con);
    } catch(SQLException& e) {
      cout << "Table test FAILED: " << e.getMessage() << endl;
      failedTests++;
    }
    
    dropStuff(con);

    delete con;

    if(failedTests>0) {
      cout << failedTests << " of " << numTests
	   << " tests failed." << endl;
    }
  } catch(exception& e) {
    cout << "Whoops: " << e.what() << endl;
    return 1;
  }

  if(assertionsFailed>0) {
    cout << assertionsFailed << " assertions failed." << endl;
    return 2;
  }
  return 0;
}

#else
int main() { return 0; }
#endif
