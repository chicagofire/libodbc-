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

#include <odbc++/resultsetmetadata.h>
#include <odbc++/resultset.h>
#include "driverinfo.h"


using namespace odbc;
using namespace std;

ResultSetMetaData::ResultSetMetaData(ResultSet* rs)
  :resultSet_(rs),
   needsGetData_(false)
{
  this->_fetchColumnInfo();
}


//private
int ResultSetMetaData::_getNumericAttribute(unsigned int col,
					   SQLUSMALLINT attr)
{
  SQLINTEGER res=0;
  SQLRETURN r=
    ODBC3_C(SQLColAttribute,SQLColAttributes)(resultSet_->hstmt_,
					      (SQLUSMALLINT)col,
					      attr,
					      0,
					      0,
					      0,
					      &res);
  resultSet_->_checkStmtError(resultSet_->hstmt_,
			      r,"Error fetching numeric attribute");
  return (int)res;
}


//private
ODBCXX_STRING 
ResultSetMetaData::_getStringAttribute(unsigned int col,
				       SQLUSMALLINT attr, unsigned int maxlen)
{
  char* buf=new char[maxlen+1];
  odbc::Deleter<char> _buf(buf,true);
  buf[maxlen]=0;

  SQLINTEGER res=0;
  SQLSMALLINT len=0;

  SQLRETURN r=
    ODBC3_C(SQLColAttribute,SQLColAttributes)(resultSet_->hstmt_,
					      (SQLUSMALLINT)col,
					      attr,
					      (SQLCHAR*)buf,
					      (SQLSMALLINT)maxlen,
					      &len,
					      &res);
  resultSet_->_checkStmtError(resultSet_->hstmt_,
			      r,"Error fetching string attribute");
  return ODBCXX_STRING_C(buf);
}

//private
void ResultSetMetaData::_fetchColumnInfo()
{
  //first argument is ignored
  numCols_=this->_getNumericAttribute
    (1,ODBC3_DC(SQL_DESC_COUNT,SQL_COLUMN_COUNT));

  for(int i=1; i<=numCols_; i++) {
    colNames_.push_back(this->_getStringAttribute
			(i,ODBC3_DC(SQL_DESC_NAME,SQL_COLUMN_NAME)));
    
    int colType=this->_getNumericAttribute
      (i,ODBC3_DC(SQL_DESC_CONCISE_TYPE,SQL_COLUMN_TYPE));
    colTypes_.push_back(colType);

    //remember if we saw any column that needs GetData
    if(colType==Types::LONGVARCHAR || colType==Types::LONGVARBINARY) {
      needsGetData_=true;
    }
    
    colPrecisions_.push_back
      (this->_getNumericAttribute
       (i,ODBC3_DC(SQL_DESC_PRECISION,SQL_COLUMN_PRECISION)));
    
    colScales_.push_back(this->_getNumericAttribute
			 (i,ODBC3_DC(SQL_DESC_SCALE,SQL_COLUMN_SCALE)));

#if ODBCVER >= 0x0300
    if(this->_getDriverInfo()->getMajorVersion()>=3) {
    // ODBC3 does not return character maxlength with SQL_DESC_PRECISION
    // so we need this as well.
      colLengths_.push_back(this->_getNumericAttribute(i,SQL_DESC_LENGTH));
    }
#endif
  }
}

#define CHECK_COL(x) 					\
do {							\
  if(x<1 || x>numCols_) {				\
    throw SQLException					\
      ("Column index out of bounds");			\
  } 							\
} while(false)

int ResultSetMetaData::getColumnCount() const
{
  return numCols_;
}

const ODBCXX_STRING& ResultSetMetaData::getColumnName(int col) const
{
  CHECK_COL(col);
  return colNames_[col-1];
}

int ResultSetMetaData::getColumnType(int col) const
{
  CHECK_COL(col);
  return colTypes_[col-1];
}

int ResultSetMetaData::getPrecision(int col) const
{
  CHECK_COL(col);
  return colPrecisions_[col-1];
}

int ResultSetMetaData::getScale(int col) const
{
  CHECK_COL(col);
  return colScales_[col-1];
}


int ResultSetMetaData::getColumnDisplaySize(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute(col,SQL_COLUMN_DISPLAY_SIZE);
}


ODBCXX_STRING ResultSetMetaData::getCatalogName(int col)
{
  CHECK_COL(col);
  return this->_getStringAttribute
    (col,ODBC3_DC(SQL_DESC_CATALOG_NAME,SQL_COLUMN_QUALIFIER_NAME));
}

ODBCXX_STRING ResultSetMetaData::getColumnLabel(int col)
{
  CHECK_COL(col);
  return this->_getStringAttribute
    (col,ODBC3_DC(SQL_DESC_LABEL,SQL_COLUMN_LABEL));
}

ODBCXX_STRING ResultSetMetaData::getColumnTypeName(int col)
{
  CHECK_COL(col);
  return this->_getStringAttribute
    (col,ODBC3_DC(SQL_DESC_TYPE_NAME,SQL_COLUMN_TYPE_NAME));
}


ODBCXX_STRING ResultSetMetaData::getSchemaName(int col)
{
  CHECK_COL(col);
  return this->_getStringAttribute
    (col,ODBC3_DC(SQL_DESC_SCHEMA_NAME,SQL_COLUMN_OWNER_NAME));
}

ODBCXX_STRING ResultSetMetaData::getTableName(int col)
{
  CHECK_COL(col);
  return this->_getStringAttribute
    (col,ODBC3_DC(SQL_DESC_TABLE_NAME,SQL_COLUMN_TABLE_NAME));
}

bool ResultSetMetaData::isAutoIncrement(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_AUTO_UNIQUE_VALUE,
		  SQL_COLUMN_AUTO_INCREMENT))!=SQL_FALSE;
}

bool ResultSetMetaData::isCaseSensitive(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_CASE_SENSITIVE,SQL_COLUMN_CASE_SENSITIVE))!=SQL_FALSE;
}

bool ResultSetMetaData::isCurrency(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_FIXED_PREC_SCALE,
		  SQL_COLUMN_MONEY))!=SQL_FALSE;
}

bool ResultSetMetaData::isDefinitelyWritable(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_UPDATABLE,SQL_COLUMN_UPDATABLE))==SQL_ATTR_WRITE;
}

int ResultSetMetaData::isNullable(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_NULLABLE,SQL_COLUMN_NULLABLE));
}

bool ResultSetMetaData::isReadOnly(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_UPDATABLE,SQL_COLUMN_UPDATABLE))==SQL_ATTR_READONLY;
}

bool ResultSetMetaData::isSearchable(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_SEARCHABLE,SQL_COLUMN_SEARCHABLE))!=SQL_UNSEARCHABLE;
}

bool ResultSetMetaData::isSigned(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_UNSIGNED,SQL_COLUMN_UNSIGNED))==SQL_FALSE;
}

bool ResultSetMetaData::isWritable(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_UPDATABLE,SQL_COLUMN_UPDATABLE))!=SQL_ATTR_READONLY;
}


