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
#include <odbc++/preparedstatement.h>
#include <odbc++/resultset.h>
#include <odbc++/databasemetadata.h>

#include <iostream>
#include <memory>

using namespace odbc;
using namespace std;

#if defined(ODBCXX_QT)
ostream& operator<<(ostream& o, const QString& s) 
{
  o << ODBCXX_STRING_CSTR(s);
  return o;
}
#endif


inline const char* s(bool b)
{
  return b?"Yes":"No";
}

inline ODBCXX_STRING maybeQuote(const ODBCXX_STRING& str, const ODBCXX_STRING& qc)
{
  if(qc!=" ") {
    return qc+str+qc;
  }
  return str;
}

static void rsInfo(DatabaseMetaData* md, int rsType, const char* name)
{
  if(md->supportsResultSetType(rsType)) {
      cout << name << endl;

      if(md->supportsResultSetConcurrency(rsType,
					  ResultSet::CONCUR_READ_ONLY)) {
	cout << "  + ResultSet::CONCUR_READ_ONLY" << endl;
      }

      if(md->supportsResultSetConcurrency(rsType,
					  ResultSet::CONCUR_UPDATABLE)) {
	cout << "  + ResultSet::CONCUR_UPDATABLE" << endl;
      }

      if(md->ownInsertsAreVisible(rsType)) {
	cout << "    Own inserts are visible" << endl;
      }

      if(md->ownUpdatesAreVisible(rsType)) {
	cout << "    Own updates are visible" << endl;
      }
      if(md->ownDeletesAreVisible(rsType)) {
	cout << "    Own deletes are visible" << endl;
      }

      cout << endl;
  }
}


void transactionInfo(DatabaseMetaData* md)
{
  cout << "Transaction support" << endl
       << "===================" << endl;

  if(!md->supportsTransactions()) {
    cout << "This datasource does not support transactions." << endl << endl;
    return;
  }

  int defIsolation=md->getDefaultTransactionIsolation();

  static struct {
    int id;
    const char* name;
  } levels[] = {
    { Connection::TRANSACTION_READ_UNCOMMITTED, 
      "Connection::TRANSACTION_READ_UNCOMMITTED" },
    { Connection::TRANSACTION_READ_COMMITTED, 
      "Connection::TRANSACTION_READ_COMMITTED" },
    { Connection::TRANSACTION_REPEATABLE_READ, 
      "Connection::TRANSACTION_REPEATABLE_READ" },
    { Connection::TRANSACTION_SERIALIZABLE, 
      "Connection::TRANSACTION_SERIALIZABLE" },
    { 0,NULL }
  };

  for(int i=0; levels[i].name!=NULL; i++) {
    if(md->supportsTransactionIsolationLevel(levels[i].id)) {
      cout << " +" << levels[i].name 
	   << (levels[i].id==defIsolation?" (default)":"") << endl;
    }
  }

  // the longest method name I've ever seen!
  if(md->supportsDataDefinitionAndDataManipulationTransactions()) {
    cout << "  Both DML and DDL can be used within a transaction" << endl;
  } else if(md->supportsDataManipulationTransactionsOnly()) {
    cout << "  Only DML can be used within a transaction" << endl;
  } else if(md->dataDefinitionCausesTransactionCommit()) {
    cout << "  DDL causes commit" << endl;
  } else if(md->dataDefinitionIgnoredInTransactions()) {
    cout << "  DDL is ignored in transactions" << endl;
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

    DatabaseMetaData* md=con->getMetaData();

    cout << "Product name                    : " 
	 << md->getDatabaseProductName() << endl

	 << "Product version                 : " 
	 << md->getDatabaseProductVersion() << endl

	 << "Driver name                     : "
	 << md->getDriverName() << endl

	 << "Driver version                  : " 
	 << md->getDriverVersion() << endl

	 << "Driver ODBC version             : "
	 << md->getDriverMajorVersion() << "."
	 << md->getDriverMinorVersion() << endl

	 << "Supports transactions           : " 
	 << s(md->supportsTransactions()) << endl;

    cout << endl;

    transactionInfo(md);


    cout << "Supported system functions" << endl
	 << "==========================" << endl
	 << md->getSystemFunctions() << endl << endl
	 << "Supported string functions" << endl
	 << "==========================" << endl
	 << md->getStringFunctions() << endl << endl
	 << "Supported time/date functions" << endl
	 << "=============================" << endl
	 << md->getTimeDateFunctions() << endl << endl
	 << "Supported numeric functions" << endl
	 << "===========================" << endl
	 << md->getNumericFunctions() << endl 
	 << "Non-ODBC SQL keywords" << endl
	 << "=====================" << endl
	 << md->getSQLKeywords() << endl;

    cout << endl;
    


    cout << "Supported ResultSet types" << endl
	 << "=========================" << endl;

    rsInfo(md,ResultSet::TYPE_FORWARD_ONLY,
	   "ResultSet::TYPE_FORWARD_ONLY");
    rsInfo(md,ResultSet::TYPE_SCROLL_INSENSITIVE,
	   "ResultSet::TYPE_SCROLL_INSENSITIVE");
    rsInfo(md,ResultSet::TYPE_SCROLL_SENSITIVE,
	   "ResultSet::TYPE_SCROLL_SENSITIVE");


    bool cdml=md->supportsCatalogsInDataManipulation();
    bool cproc=md->supportsCatalogsInProcedureCalls();
    bool ctd=md->supportsCatalogsInTableDefinitions();
    bool cid=md->supportsCatalogsInIndexDefinitions();
    bool cpd=md->supportsCatalogsInPrivilegeDefinitions();

    bool sdml=md->supportsSchemasInDataManipulation();
    bool sproc=md->supportsSchemasInProcedureCalls();
    bool std=md->supportsSchemasInTableDefinitions();
    bool sid=md->supportsSchemasInIndexDefinitions();
    bool spd=md->supportsSchemasInPrivilegeDefinitions();

    bool hasCatalogs=cdml || cproc || ctd || cid || cpd;

    if(hasCatalogs) {
      cout << "Supported catalog uses" << endl
	   << "======================" << endl;
      cout << "Data manipulation    : " << s(cdml) << endl;
      cout << "Procedure calls      : " << s(cproc) << endl;
      cout << "Table definitions    : " << s(ctd) << endl;
      cout << "Index definitions    : " << s(cid) << endl;
      cout << "Privilege definitions: " << s(cpd) << endl;
    } else {
      cout << "This datasource does not support catalogs" << endl;
    }
    cout << endl;

    bool hasSchemas=sdml || sproc || std || sid || spd;

    if(hasSchemas) {
      cout << "Supported schema uses" << endl
	   << "=====================" << endl;
      cout << "Data manipulation    : " << s(sdml) << endl;
      cout << "Procedure calls      : " << s(sproc) << endl;
      cout << "Table definitions    : " << s(std) << endl;
      cout << "Index definitions    : " << s(sid) << endl;
      cout << "Privilege definitions: " << s(spd) << endl;
    } else {
      cout << "This datasource does not support schemas" << endl;
    }
    cout << endl;

    ODBCXX_STRING idq(md->getIdentifierQuoteString());
    // space means no quoting supported

    ODBCXX_STRING id(maybeQuote(md->getTableTerm(),idq));
    if(hasSchemas) {
      id=maybeQuote(md->getSchemaTerm(),idq)+"."+id;
    }
    if(hasCatalogs) {
      ODBCXX_STRING catSep(md->getCatalogSeparator());
      if(md->isCatalogAtStart()) {
	id=maybeQuote(md->getCatalogTerm(),idq)+catSep+id;
      } else {
	id+=catSep+maybeQuote(md->getCatalogTerm(),idq);
      }
    }

    cout << "Preferred table identifier format: " << id << endl;

    if(hasCatalogs) {
      cout << "Tables available: " << endl;
      std::vector<ODBCXX_STRING> types;
      auto_ptr<odbc::ResultSet> rs(md->getTables(ODBCXX_STRING(""),
                                                ODBCXX_STRING(""),
                                                ODBCXX_STRING(""),
                                                types));
      while(rs->next()) {
        cout << rs->getString(1) << "." 
             << rs->getString(2) << "."
             << rs->getString(3) << " type="
             << rs->getString(4) << " remarks="
             << rs->getString(5) <<endl;
      }
    }

    delete con;

  } catch(SQLException& e) {
    cerr << endl << e.getMessage() << endl;
  }

  return 0;
}
