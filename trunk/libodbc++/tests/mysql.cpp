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
#include <odbc++/connection.h>
#include <odbc++/resultset.h>
#include <odbc++/resultsetmetadata.h>
#include <odbc++/preparedstatement.h>

#include <cstdio>
#include <cmath>
#include <iostream>

#if defined(WIN32)
# define snprintf _snprintf
#endif

#if !defined(ODBCXX_QT)

#include <strstream>

#else

#include <qbuffer.h>
#include <qstring.h>

// QT uses this
#undef ASSERT

#endif


/*
  This is a simple test to be run against a mysql database
  Assure you have create table permissions before you run.
*/


using namespace odbc;
using namespace std;

const char* testchars="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

const int odbctestRows=5000;

const int longtestRows=30;
const int longtestSize=123456;

static int assertionsFailed=0;

#define ASSERT(x)						\
do {								\
  if(!(x)) {							\
    cerr << "Assertion \"" << #x << "\" failed" << endl;	\
    assertionsFailed++;                                         \
  }								\
} while(false)

inline bool feq(const double& a, const double& b)
{
  return (fabs(a-b)<=a*0.0000001);
}


//this generates the values for odbctest
class ValueGen1 {
private:
  size_t cnt_;
  size_t max_;

public:
  ValueGen1(size_t max) : cnt_(0), max_(max) {}
  ~ValueGen1() {}
  
  bool next() {
    return ((++cnt_) <= max_);
  }

  size_t cnt() {
    return cnt_;
  }

  signed char i1() {
    return (signed char)(cnt_%256 - 128);
  }

  short i2() {
    return (short)(cnt_%65536 - 32768);
  }

  int i3() {
    return (int)(cnt_%16777215-8388608);
  }

  int i4() {
    return (int)cnt_;
  }

  Long i5() {
    return ((Long)cnt_)*1000000L;
  }

  float f1() {
    return ((float)cnt_);
  }

  float f2() {
    return ((float)cnt_/10);
  }

  double f3() {
    return ((double)cnt_/20);
  }

  Long d1() {
    return (((Long)cnt_)-max_/2)*4567;
  }

  ODBCXX_STRING s1() {
    return ODBCXX_STRING_CL(&testchars[cnt_%strlen(testchars)],1);
  }

  ODBCXX_STRING s3() {
    return this->s1();
  }

  ODBCXX_STRING s2() {
    char buf[41];
    snprintf(buf,41,"This is row number %d",cnt_);
    return ODBCXX_STRING_C(buf);
  }

  ODBCXX_STRING s4() {
    return this->s2();
  }

  Date dt() {
    return Date((time_t)cnt_*10000);
  }

  Time t() {
    return Time((time_t)cnt_);
  }

  Timestamp ts() {
    return Timestamp((time_t)cnt_*10000);
  }
};


//this is for the BLOB/CLOB test

class ValueGen2 {
private:
  size_t cnt_;
  size_t rows_;

  char currentC[longtestSize+1];
  char currentB[longtestSize];

public:
  ValueGen2(size_t rows)
    :cnt_(0), rows_(rows) {}
  ~ValueGen2() {}

  bool next() {
    return ((++cnt_) <= rows_);
  }

  int id() { 
    return cnt_; 
  }

  ODBCXX_STREAM* c() {
    int len=strlen(testchars);
    for(int i=0; i<longtestSize; i++) {
      currentC[i]=testchars[(i+cnt_)%len];
    }
    currentC[longtestSize]=0;
#if !defined(ODBCXX_QT)
    istrstream* s=new istrstream(currentC,longtestSize);
#else
    QBuffer* s=new QBuffer();
    s->open(IO_WriteOnly);
    s->writeBlock(currentC,longtestSize);
    s->close();
    s->open(IO_ReadOnly);
#endif
    return s;
  }

  ODBCXX_STREAM* b() {
    for(int i=0; i<longtestSize; i++) {
      currentB[i]=((i+cnt_)%256);
    }
#if !defined(ODBCXX_QT)
    istrstream* s=new istrstream(currentB,longtestSize);
#else
    QBuffer* s=new QBuffer();
    s->open(IO_WriteOnly);
    s->writeBlock(currentB,longtestSize);
    s->close();
    s->open(IO_ReadOnly);
#endif
    return s;
  }
};

inline bool compareStreams(ODBCXX_STREAM* s1, ODBCXX_STREAM* s2)
{
  size_t cnt=0;
#if !defined(ODBCXX_QT)
  char c1, c2;
  while(s1->get(c1) && s2->get(c2)) {
    cnt++;
    if(c1!=c2)
      return false;
  }
#else
  char buf1[1024];
  char buf2[1024];
  int r1, r2;
  while((r1=s1->readBlock(buf1,1024))!=-1 && 
	(r2=s2->readBlock(buf2,1024))!=-1) {
    if(r1!=r2) return false;

    for(int i=0; i<r1; i++) {
      cnt++;
      if(buf1[i]!=buf2[i]) return false;
    }
  }
#endif
  return (cnt==longtestSize);
}


static void createTables(Connection* con)
{
  cout << "Creating tables:" << flush;
  PreparedStatement* pstmt=con->prepareStatement
    ("create table odbctest ("
     "i1 tinyint not null, "
     "i2 smallint not null, "
     "i3 mediumint not null, "
     "i4 int not null, "
     "i5 bigint not null, "
     "f1 float(4) not null, "
     "f2 float(8) not null, "
     "f3 double(10,3) not null, "
     "d1 decimal(20,5) not null, "
     "s1 char(1) not null, "
     "s2 char(40) not null, "
     "s3 varchar(1) not null, "
     "s4 varchar(40) not null, "
     "dt date not null, "
     "t time not null, "
     "ts datetime not null"
     ")");
  pstmt->executeUpdate();
  cout << " odbctest" << flush;
  delete pstmt;

  pstmt=con->prepareStatement
    ("create table odbctest2 ("
     "id int not null, "
     "c mediumtext, "
     "b mediumblob)");
  pstmt->executeUpdate();
  cout << " odbctest2" << flush;

  delete pstmt;
  
  cout << endl;
}

static void populateTables(Connection* con)
{
  
  cout << "Populating:" << flush;

  PreparedStatement* pstmt=con->prepareStatement
    ("insert into odbctest(i1,i2,i3,i4,i5,f1,f2,f3,d1,s1,s2,s3,s4,dt,t,ts) "
     "values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");

  ValueGen1 vg(odbctestRows);
  while(vg.next()) {
    pstmt->setByte(1,vg.i1());
    pstmt->setShort(2,vg.i2());
    pstmt->setInt(3,vg.i3());
    pstmt->setInt(4,vg.i4());
    pstmt->setLong(5,vg.i5());
    pstmt->setFloat(6,vg.f1());
    pstmt->setFloat(7,vg.f2());
    pstmt->setDouble(8,vg.f3());
    pstmt->setLong(9,vg.d1());
    pstmt->setString(10,vg.s1());
    pstmt->setString(11,vg.s2());
    pstmt->setString(12,vg.s3());
    pstmt->setString(13,vg.s4());
    pstmt->setDate(14,vg.dt());
    pstmt->setTime(15,vg.t());
    pstmt->setTimestamp(16,vg.ts());
    pstmt->executeUpdate();
  }
  
  delete pstmt;
  cout << " odbctest (" << odbctestRows << ")" << flush;
  
  pstmt=con->prepareStatement
    ("insert into odbctest2(id,c,b) values(?,?,?)");

  ValueGen2 vg2(longtestRows);
  while(vg2.next()) {
    pstmt->setInt(1,vg2.id());
    ODBCXX_STREAM* cs=vg2.c();
    pstmt->setAsciiStream(2,cs,longtestSize);
    ODBCXX_STREAM* bs=vg2.b();
    pstmt->setBinaryStream(3,bs,longtestSize);
   
    pstmt->executeUpdate();

    delete cs;
    delete bs;
  }

  // insert an extra row containing NULL for c and b
  pstmt->setInt(1,vg2.id()+1);
  pstmt->setNull(2,Types::LONGVARCHAR);
  pstmt->setNull(3,Types::LONGVARBINARY);
  pstmt->executeUpdate();

  delete pstmt;
  
  cout << " odbctest2 (" << longtestRows << ")" << flush;

  cout << endl;
}


static void checkTables(Connection* con)
{
  cout << "Checking:" << flush;

  Statement* stmt=con->createStatement();
  stmt->setFetchSize(37); // some odd number
  ResultSet* rs=stmt->executeQuery
    ("select i1,i2,i3,i4,i5,f1,f2,f3,d1,s1,s2,s3,s4,dt,t,ts from odbctest");

  cout << " odbctest" << flush;
  
  ValueGen1 vg(odbctestRows);

  size_t cnt=0;
  while(rs->next() && vg.next()) {
    cnt++;

    ASSERT(vg.cnt()==rs->getRow());

    ASSERT(vg.i1()==rs->getByte(1) && 
	   vg.i1()==rs->getByte("i1"));
    ASSERT(vg.i2()==rs->getShort(2) &&
	   vg.i2()==rs->getShort("i2"));
    ASSERT(vg.i3()==rs->getInt(3) && 
	   vg.i3()==rs->getInt("i3"));
    ASSERT(vg.i4()==rs->getInt(4) &&
	   vg.i4()==rs->getInt("i4"));
    ASSERT(vg.i5()==rs->getLong(5) && 
	   vg.i5()==rs->getLong("i5"));
    
    ASSERT(feq(vg.f1(),rs->getFloat(6)) &&
	   feq(vg.f1(),rs->getFloat("f1")));
    ASSERT(feq(vg.f2(),rs->getFloat(7)) &&
	   feq(vg.f2(),rs->getFloat("f2")));
    ASSERT(feq(vg.f3(),rs->getDouble(8)) &&
	   feq(vg.f3(),rs->getDouble("f3")));

    ASSERT(vg.d1()==rs->getLong(9) &&
	   vg.d1()==rs->getLong("d1"));

    ASSERT(vg.s1()==rs->getString(10)
	   && vg.s1()==rs->getString("s1"));
    ASSERT(vg.s2()==rs->getString(11) && 
	   vg.s2()==rs->getString("s2"));
    ASSERT(vg.s3()==rs->getString(12) &&
	   vg.s3()==rs->getString("s3"));
    ASSERT(vg.s4()==rs->getString(13) &&
	   vg.s4()==rs->getString("s4"));

    ASSERT(vg.dt().toString()==rs->getString(14));
    ASSERT(vg.t().toString()==rs->getString(15));
    ASSERT(vg.ts().toString()==rs->getString(16));
  }

  delete rs;
  delete stmt;

  ASSERT(cnt==odbctestRows);
  cout << "(" << cnt << ")" << flush;

  stmt=con->createStatement();
  stmt->setFetchSize(10);
  rs=stmt->executeQuery("select id,c,b from odbctest2");
  // since we have LONGVARwhatevers in the result set
  // this should fall down to 1
  ASSERT(rs->getFetchSize()==1);
  
  ValueGen2 vg2(longtestRows);
  cnt=0;
  cout << " odbctest2" << flush;
  while(rs->next() && vg2.next()) {
    cnt++;
    ASSERT(vg2.id()==rs->getInt("id"));
    ODBCXX_STREAM* cs=vg2.c();
    ODBCXX_STREAM* bs=vg2.b();
    ASSERT(compareStreams(cs,rs->getAsciiStream("c")));
    ASSERT(compareStreams(bs,rs->getBinaryStream("b")));
    delete cs;
    delete bs;
  }

  // here, rs->next has been called an extra time above 
  // (we're at the row containing NULL values)
  ODBCXX_STREAM* tmp=rs->getAsciiStream("c");
  ASSERT(rs->wasNull());
  tmp=rs->getBinaryStream("b");
  ASSERT(rs->wasNull());
  
  ASSERT(cnt==longtestRows);
  cout << "(" << cnt << ")" << flush;

  delete rs;
  delete stmt;

  cout << endl;
}

static bool dropTable(Connection* con, const ODBCXX_STRING& tableName)
{
  bool r=true;
  PreparedStatement* pstmt=con->prepareStatement
    ("drop table "+tableName);
  try {
    pstmt->executeUpdate();
  } catch(SQLException& e) { r=false; }

  delete pstmt;
  return r;
}

static void dropTables(Connection* con) 
{
  cout << "Dropping tables:" << flush;
  if(dropTable(con,"odbctest")) {
    cout << " odbctest" << flush;
  }

  if(dropTable(con," odbctest2")) {
    cout << " odbctest2" << flush;
  }
  cout << endl;
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

    dropTables(con);
    createTables(con);

    populateTables(con);

    checkTables(con);

    dropTables(con);
    
    delete con;

    DriverManager::shutdown();

    if(assertionsFailed) {
      cerr << assertionsFailed << " assertions failed" << endl;
      return 1;
    } else {
      cout << "Apparently, this worked!" << endl;
    }

    
  } catch(SQLException& e) {
    cerr << endl << e.getMessage() << endl;
    return 2;
  }

  return 0;
}
