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

/* Bring in long long macros needed by unixODBC */
#if !defined(__WIN32__) && !defined(WIN32)
# include <config.h>
#endif

#include <odbc++/types.h>

#include "datahandler.h"
#include "dtconv.h"

using namespace odbc;
using namespace std;

typedef ODBC3_C(SQL_DATE_STRUCT,DATE_STRUCT) DateStruct;
typedef ODBC3_C(SQL_TIME_STRUCT,TIME_STRUCT) TimeStruct;
typedef ODBC3_C(SQL_TIMESTAMP_STRUCT,TIMESTAMP_STRUCT) TimestampStruct;

#define C_TIMESTAMP ODBC3_C(SQL_C_TYPE_TIMESTAMP,SQL_C_TIMESTAMP)
#define C_TIME ODBC3_C(SQL_C_TYPE_TIME,SQL_C_TIME)
#define C_DATE ODBC3_C(SQL_C_TYPE_DATE,SQL_C_DATE)

namespace odbc {
  const char* nameOfSQLType(int sqlType) {
    static struct {
      int id;
      const char* name;
    } sqlTypes[] = {
      { Types::BIGINT,		"BIGINT" },
      { Types::BINARY,		"BINARY" },
      { Types::BIT,		"BIT" },
      { Types::CHAR,		"CHAR" },
      { Types::DATE,		"DATE" },
      { Types::DECIMAL,		"DECIMAL" },
      { Types::DOUBLE, 		"DOUBLE" },
      { Types::FLOAT,		"FLOAT" },
      { Types::INTEGER,		"INTEGER" },
      { Types::LONGVARBINARY, 	"LONGVARBINARY" },
      { Types::LONGVARCHAR, 	"LONGVARCHAR" },
      { Types::NUMERIC, 	"NUMERIC" },
      { Types::REAL,		"REAL" },
      { Types::SMALLINT,	"SMALLINT" },
      { Types::TIME,		"TIME" },
      { Types::TIMESTAMP,	"TIMESTAMP" },
      { Types::TINYINT,		"TINYINT" },
      { Types::VARBINARY, 	"VARBINARY" },
      { Types::VARCHAR, 	"VARCHAR" },
      {0,			NULL }
    };

    for(unsigned int i=0; sqlTypes[i].name!=NULL; i++) {
      if(sqlTypes[i].id==sqlType) {
	return sqlTypes[i].name;
      }
    }

    return "UNKNOWN";
  }

  const char* nameOfCType(int cType) {
    static struct {
      int id;
      const char* name;
    } cTypes[] = {
      { SQL_C_CHAR, 		"SQL_C_CHAR" },
      { SQL_C_BINARY,		"SQL_C_BINARY" },
      { SQL_C_BIT,		"SQL_C_BIT" },
      { SQL_C_TINYINT,		"SQL_C_TINYINT" },
      { SQL_C_SHORT,		"SQL_C_SHORT" },
      { SQL_C_LONG,		"SQL_C_LONG" },
      { SQL_C_DOUBLE,		"SQL_C_DOUBLE" },
      { SQL_C_FLOAT,		"SQL_C_FLOAT" },
      { SQL_C_DATE,		"SQL_C_DATE" },
      { SQL_C_TIME,		"SQL_C_TIME" },
      { SQL_C_TIMESTAMP,	"SQL_C_TIMESTAMP" },
#if ODBCVER >= 0x0300
      { SQL_C_SBIGINT,		"SQL_C_SBIGINT" },
      { SQL_C_TYPE_TIME,	"SQL_C_TYPE_TIME" },
      { SQL_C_TYPE_DATE,	"SQL_C_TYPE_DATE" },
      { SQL_C_TYPE_TIMESTAMP,	"SQL_C_TYPE_TIMESTAMP" },
#endif
      { 0,			NULL }
    };

    for(unsigned int i=0; cTypes[i].name!=NULL; i++) {
      if(cTypes[i].id==cType) {
	return cTypes[i].name;
      }
    }
    return "UNKNOWN";
  }

};

#define UNSUPPORTED_GET(as_type)				\
throw SQLException						\
("[libodbc++]: Could not get SQL type "+intToString(sqlType_)+	\
 " ("+nameOfSQLType(sqlType_)+"), C type "+intToString(cType_)+	\
 " ("+nameOfCType(cType_)+") as " as_type)

#define UNSUPPORTED_SET(to_type)				\
throw SQLException						\
("[libodbc++]: Could not set SQL type "+intToString(sqlType_)+	\
 " ("+nameOfSQLType(sqlType_)+"), C type "+intToString(cType_)+	\
 " ("+nameOfCType(cType_)+") to " to_type)





void DataHandler::setupBuffer(size_t s)
{
  if(bufferSize_>0) {
    delete[] buffer_;
  }
  
  bufferSize_=s;
  if(bufferSize_>0) {
    buffer_=new char[rows_*bufferSize_];
  } else {
    buffer_=NULL;
  }
}


DataHandler::DataHandler(unsigned int& cr, size_t rows,
			 int sqlType, int precision, int scale,
			 bool use3)
  :currentRow_(cr),rows_(rows),
   buffer_(NULL),bufferSize_(0), dataStatus_(NULL),
   isStreamed_(false),stream_(NULL),ownStream_(false),
   sqlType_(sqlType),
   precision_(precision), 
   scale_(scale),
   use3_(use3)
{

  size_t bs=0;

  switch(sqlType_) {
  case Types::CHAR:
  case Types::VARCHAR:
#if defined(ODBCXX_HAVE_SQLUCODE_H)
  case Types::WVARCHAR:   //convert unicode columns to ANSI
  case Types::WCHAR:              //
#endif
    cType_=SQL_C_CHAR;
    scale_=0;
    bs=precision_+1; //string+null
    break;

  case Types::NUMERIC:
  case Types::DECIMAL:
    cType_=SQL_C_CHAR;
    bs=precision_+3; //sign,comma,null
    break;

  case Types::BIGINT:
    // for ODBC3, we fetch this as an SQLBIGINT
    // for ODBC2, we convert to a string
#if ODBCVER >= 0x0300
    if(use3_) {
      cType_=SQL_C_SBIGINT;
      scale_=0;
      bs=sizeof(SQLBIGINT);
      break;
    }
#endif
    cType_=SQL_C_CHAR;
    scale_=0;
    bs=21; //19 digits, sign,null
    break;
    
  case Types::BIT:
    cType_=SQL_C_BIT;
    scale_=0;
    bs=sizeof(signed char);
    break;

  case Types::TINYINT:
    cType_=SQL_C_TINYINT;
    scale_=0;
    bs=sizeof(signed char);
    break;

  case Types::SMALLINT:
    cType_=SQL_C_SHORT;
    scale_=0;
    bs=sizeof(short);
    break;

  case Types::INTEGER:
    cType_=SQL_C_LONG;
    scale_=0;
    bs=sizeof(SQLINTEGER);
    break;

  case Types::FLOAT:
  case Types::DOUBLE:
    cType_=SQL_C_DOUBLE;
    bs=sizeof(double);
    break;

  case Types::REAL:
    cType_=SQL_C_FLOAT;
    bs=sizeof(float);
    break;

  case Types::BINARY:
  case Types::VARBINARY:
    cType_=SQL_C_BINARY;
    bs=precision_;
    break;

  case Types::DATE:
    cType_=C_DATE;
    bs=sizeof(DateStruct);
    break;

  case Types::TIME:
    cType_=C_TIME;
    bs=sizeof(TimeStruct);
    break;
    
  case Types::TIMESTAMP:
    cType_=C_TIMESTAMP;
    bs=sizeof(TimestampStruct);
    break;

  case Types::LONGVARCHAR:
#if defined(ODBCXX_HAVE_SQLUCODE_H)
  case Types::WLONGVARCHAR:
#endif
    cType_=SQL_C_CHAR;
    bs=0; // this one is streamed
    isStreamed_=true;
    break;

  case Types::LONGVARBINARY:
    cType_=SQL_C_BINARY;
    bs=0; // same here
    isStreamed_=true;
    break;

  default:
    throw SQLException
      ("[libodbc++]: DataHandler: unhandled SQL type "+intToString(sqlType_));
    
  };
  this->setupBuffer(bs);

  dataStatus_=new SQLINTEGER[rows_];

  //set everything to NULL
  for(unsigned int i=0; i<rows_; i++) {
    dataStatus_[i]=SQL_NULL_DATA;
  }
}


#define GET_AS(type) (CURRENT_RETTYPE)*(type*)this->data()

#define ACCEPT_GET(id,type) case id: return GET_AS(type)

bool DataHandler::getBoolean() const
{
  return this->getInt()!=0;
}

signed char DataHandler::getByte() const
{
  return (signed char)this->getInt();
}

short DataHandler::getShort() const
{
  return (short)this->getInt();
}

#undef CURRENT_RETTYPE
#define CURRENT_RETTYPE int

int DataHandler::getInt() const
{
  if(!this->isNull()) {
    switch(cType_) {
      ACCEPT_GET(SQL_C_LONG,SQLINTEGER);
      ACCEPT_GET(SQL_C_SHORT,short);
      ACCEPT_GET(SQL_C_TINYINT,signed char);
      ACCEPT_GET(SQL_C_BIT,signed char);
#if ODBCVER >= 0x0300
      ACCEPT_GET(SQL_C_SBIGINT,SQLBIGINT);
#endif
      ACCEPT_GET(SQL_C_FLOAT,float);
      ACCEPT_GET(SQL_C_DOUBLE,double);
      
    case SQL_C_CHAR:
      if(!isStreamed_) {
	return stringToInt(this->data());
      }
      
    default:
      UNSUPPORTED_GET("an int");
    }
  }
  
  return 0;
}

#undef CURRENT_RETTYPE
#define CURRENT_RETTYPE Long

Long DataHandler::getLong() const
{
  if(!this->isNull()) {
    switch(cType_) {
#if ODBCVER >= 0x0300
      ACCEPT_GET(SQL_C_SBIGINT,SQLBIGINT);
#endif
      ACCEPT_GET(SQL_C_LONG,SQLINTEGER);
      ACCEPT_GET(SQL_C_SHORT,short);
      ACCEPT_GET(SQL_C_TINYINT,signed char);
      ACCEPT_GET(SQL_C_BIT,signed char);
      ACCEPT_GET(SQL_C_FLOAT,float);
      ACCEPT_GET(SQL_C_DOUBLE,double);

    case SQL_C_CHAR:
      if(!isStreamed_) {
	return stringToLong(this->data());
      }
      
    default:
      UNSUPPORTED_GET("a Long");
    }
  }
  
  return 0;
}

#undef CURRENT_RETTYPE
#define CURRENT_RETTYPE float

float DataHandler::getFloat() const
{
  if(!this->isNull()) {
    switch(cType_) {
      ACCEPT_GET(SQL_C_FLOAT,float);
      ACCEPT_GET(SQL_C_DOUBLE,double);
#if ODBCVER >= 0x0300
      ACCEPT_GET(SQL_C_SBIGINT,SQLBIGINT);
#endif
      ACCEPT_GET(SQL_C_LONG,SQLINTEGER);
      ACCEPT_GET(SQL_C_SHORT,short);
      ACCEPT_GET(SQL_C_TINYINT,signed char);
      ACCEPT_GET(SQL_C_BIT,signed char);

    case SQL_C_CHAR:
      if(!isStreamed_) {
	return (float)stringToDouble(this->data());
      }

    default:
      UNSUPPORTED_GET("a float");
    }
  }

  return 0;
}

#undef CURRENT_RETTYPE
#define CURRENT_RETTYPE double

double DataHandler::getDouble() const
{
  if(!this->isNull()) {
    switch(cType_) {
      ACCEPT_GET(SQL_C_DOUBLE,double);
      ACCEPT_GET(SQL_C_FLOAT,float);
#if ODBCVER >= 0x0300
      ACCEPT_GET(SQL_C_SBIGINT,SQLBIGINT);
#endif
      ACCEPT_GET(SQL_C_LONG,SQLINTEGER);
      ACCEPT_GET(SQL_C_SHORT,short);
      ACCEPT_GET(SQL_C_TINYINT,signed char);
      ACCEPT_GET(SQL_C_BIT,signed char);

    case SQL_C_CHAR:
      if(!isStreamed_) {
	return (float)stringToDouble(this->data());
      }

    default:
      UNSUPPORTED_GET("a double");
    }
  }

  return 0;
}

#undef CURRENT_RETTYPE

Date DataHandler::getDate() const
{
  if(!this->isNull()) {
    switch(cType_) {
    case C_DATE:
      {
	DateStruct* ds=(DateStruct*)this->data();
	return Date(ds->year,ds->month,ds->day);
      }

    case C_TIMESTAMP:
      {
	TimestampStruct* ts=(TimestampStruct*)this->data();
	return Date(ts->year,ts->month,ts->day);
      }
      
    case SQL_C_CHAR:
      if(!isStreamed_) {
	return Date(this->data());
      }
      
    default:
      UNSUPPORTED_GET("a Date");
    }
  }
  
  return Date();
}

Time DataHandler::getTime() const
{
  if(!this->isNull()) {
    switch(cType_) {
    case C_TIME:
      {
	TimeStruct* ts=(TimeStruct*)this->data();
	return Time(ts->hour,ts->minute,ts->second);
      }

    case C_TIMESTAMP:
      {
	TimestampStruct* ts=(TimestampStruct*)this->data();
	return Time(ts->hour,ts->minute,ts->second);
      }

    case SQL_C_CHAR:
      if(!isStreamed_) {
	return Time(this->data());
      }

    default:
      UNSUPPORTED_GET("a Time");
    }
  }
  
  return Time();
}

Timestamp DataHandler::getTimestamp() const
{
  if(!this->isNull()) {
    switch(cType_) {
    case C_TIMESTAMP:
      {
	TimestampStruct* ts=(TimestampStruct*)this->data();
	return Timestamp(ts->year,ts->month,ts->day,
			 ts->hour,ts->minute,ts->second,ts->fraction);
      }

    case C_DATE:
      {
	DateStruct* ds=(DateStruct*)this->data();
	return Timestamp(ds->year,ds->month,ds->day,
			 0,0,0);
      }

    case C_TIME:
      {
	TimeStruct* ts=(TimeStruct*)this->data();
	return Timestamp(0,0,0,ts->hour,ts->minute,ts->second);
      }

    case SQL_C_CHAR:
      if(!isStreamed_) {
	return Timestamp(this->data());
      }

    default:
      UNSUPPORTED_GET("a Timestamp");
    }
  }

  return Timestamp();
}


ODBCXX_STRING DataHandler::getString() const
{
  if(!this->isNull()) {
    switch(cType_) {
    case SQL_C_CHAR:
      if(!isStreamed_) {
	if(this->getDataStatus()==SQL_NTS) {
	  return ODBCXX_STRING_C(this->data());
	} else {
	  return ODBCXX_STRING_CL(this->data(),this->getDataStatus());
	}
      } else {
	throw SQLException("[libodbc++]: NYI: Getting a stream as a string");
      }

    case C_DATE:
      return this->getDate().toString();
      
    case C_TIME:
      return this->getTime().toString();
      
    case C_TIMESTAMP:
      return this->getTimestamp().toString();

    case SQL_C_BIT:
    case SQL_C_TINYINT:
    case SQL_C_SHORT:
    case SQL_C_LONG:
      return intToString(this->getInt());

#if ODBCVER >= 0x0300
    case SQL_C_SBIGINT:
      return longToString(this->getLong());
#endif

    case SQL_C_FLOAT:
    case SQL_C_DOUBLE:
      return doubleToString(this->getDouble());

    default:
      UNSUPPORTED_GET("a string");
    }
  }

  return ODBCXX_STRING("");
}


ODBCXX_BYTES DataHandler::getBytes() const
{
  if(!this->isNull()) {
    switch(cType_) {
    case SQL_C_CHAR:
    case SQL_C_BINARY:
      if(!isStreamed_) {
	return ODBCXX_BYTES_C(this->data(),this->getDataStatus());
      }

    default:
      UNSUPPORTED_GET("a Bytes");
    }
  }

  return ODBCXX_BYTES_C(NULL,0);
}


ODBCXX_STREAM* DataHandler::getStream() const
{
  // in here, we can't trust this->isNull(), since
  // this probably isn't bound.
  switch(cType_) {
  case SQL_C_BINARY:
  case SQL_C_CHAR:
    if(isStreamed_) {
      return stream_;
    }
    
  default:
    UNSUPPORTED_GET("an stream");
  }
  
  // notreached
  assert(false);
  ODBCXX_DUMMY_RETURN(NULL);
}

#define SET_TO(type,val)			\
*(type*)this->data()=(type)val;			\
this->setDataStatus(sizeof(type))


#define ACCEPT_SET_VAL(id,type,val)		\
case id: SET_TO(type,val); break

#define ACCEPT_SET(id,type) ACCEPT_SET_VAL(id,type,val)

void DataHandler::setBoolean(bool b)
{
  this->setInt(b?1:0);
}

void DataHandler::setByte(signed char b)
{
  this->setInt(b);
}

void DataHandler::setShort(short s)
{
  this->setInt(s);
}

void DataHandler::setInt(int val)
{
  switch(cType_) {
    ACCEPT_SET(SQL_C_BIT,signed char);
    ACCEPT_SET(SQL_C_TINYINT,signed char);
    ACCEPT_SET(SQL_C_SHORT,short);
    ACCEPT_SET(SQL_C_LONG,SQLINTEGER);

#if ODBCVER >= 0x0300
    ACCEPT_SET(SQL_C_SBIGINT,SQLBIGINT);
#endif

    ACCEPT_SET(SQL_C_DOUBLE,double);
    ACCEPT_SET(SQL_C_FLOAT,float);
    
  case SQL_C_CHAR:
    this->setString(intToString(val));
    break;

  default:
    UNSUPPORTED_SET("an int");
  }
}

void DataHandler::setLong(Long val)
{
  switch(cType_) {
#if ODBCVER >= 0x0300
    ACCEPT_SET(SQL_C_SBIGINT,SQLBIGINT);
#endif

    ACCEPT_SET(SQL_C_BIT,signed char);
    ACCEPT_SET(SQL_C_TINYINT,signed char);
    ACCEPT_SET(SQL_C_SHORT,short);
    ACCEPT_SET(SQL_C_LONG,SQLINTEGER);
    ACCEPT_SET(SQL_C_DOUBLE,double);
    ACCEPT_SET(SQL_C_FLOAT,float);

  case SQL_C_CHAR:
    this->setString(longToString(val));
    break;

  default:
    UNSUPPORTED_SET("a Long");
  }
}

void DataHandler::setFloat(float val)
{
  switch(cType_) {
    ACCEPT_SET(SQL_C_FLOAT,float);
    ACCEPT_SET(SQL_C_DOUBLE,double);
#if ODBCVER >= 0x0300
    ACCEPT_SET(SQL_C_SBIGINT,SQLBIGINT);
#endif
    ACCEPT_SET(SQL_C_BIT,signed char);
    ACCEPT_SET(SQL_C_TINYINT,signed char);
    ACCEPT_SET(SQL_C_SHORT,short);
    ACCEPT_SET(SQL_C_LONG,SQLINTEGER);

  case SQL_C_CHAR:
    this->setString(doubleToString(val));
    break;

  default:
    UNSUPPORTED_SET("a float");
  }
}

void DataHandler::setDouble(double val)
{
  switch(cType_) {
    ACCEPT_SET(SQL_C_DOUBLE,double);
    ACCEPT_SET(SQL_C_FLOAT,float);
#if ODBCVER >= 0x0300
    ACCEPT_SET(SQL_C_SBIGINT,SQLBIGINT);
#endif
    ACCEPT_SET(SQL_C_BIT,signed char);
    ACCEPT_SET(SQL_C_TINYINT,signed char);
    ACCEPT_SET(SQL_C_SHORT,short);
    ACCEPT_SET(SQL_C_LONG,SQLINTEGER);

  case SQL_C_CHAR:
    this->setString(doubleToString(val));
    break;

  default:
    UNSUPPORTED_SET("a double");
  }
}


void DataHandler::setDate(const Date& d)
{
  switch(cType_) {
  case C_DATE:
    {
      DateStruct* ds=(DateStruct*)this->data();
      ds->year=d.getYear();
      ds->month=d.getMonth();
      ds->day=d.getDay();
      this->setDataStatus(sizeof(DateStruct));
    }
    break;

  case C_TIMESTAMP:
    {
      TimestampStruct* ts=(TimestampStruct*)this->data();
      ts->year=d.getYear();
      ts->month=d.getMonth();
      ts->day=d.getDay();
      ts->hour=0;
      ts->minute=0;
      ts->second=0;
      ts->fraction=0;
      this->setDataStatus(sizeof(TimestampStruct));
    }
    break;

  case SQL_C_CHAR:
    if(!isStreamed_) {
      // ODBC date escape
      this->setString("{d '"+d.toString()+"'}");
      break;
    }

  default:
    UNSUPPORTED_SET("a Date");
  }
}

void DataHandler::setTime(const Time& t)
{
  switch(cType_) {
  case C_TIME:
    {
      TimeStruct* ts=(TimeStruct*)this->data();
      ts->hour=t.getHour();
      ts->minute=t.getMinute();
      ts->second=t.getSecond();
      this->setDataStatus(sizeof(TimeStruct));
    }
    break;

  case SQL_C_CHAR:
    if(!isStreamed_) {
      this->setString("{t '"+t.toString()+"'}");
      break;
    }

  default:
    UNSUPPORTED_SET("a Time");
  }
}

void DataHandler::setTimestamp(const Timestamp& t)
{
  switch(cType_) {
  case C_TIMESTAMP:
    {
      TimestampStruct* ts=(TimestampStruct*)this->data();
      ts->year=t.getYear();
      ts->month=t.getMonth();
      ts->day=t.getDay();
      ts->hour=t.getHour();
      ts->minute=t.getMinute();
      ts->second=t.getSecond();
      ts->fraction=t.getNanos();
      this->setDataStatus(sizeof(TimestampStruct));
    }
    break;

  case SQL_C_CHAR:
    if(!isStreamed_) {
      this->setString("{ts '"+t.toString()+"'}");
      break;
    }
    
  default:
    UNSUPPORTED_SET("a Timestamp");
  }
}


void DataHandler::setString(const ODBCXX_STRING& str)
{
  switch(cType_) {
  case SQL_C_CHAR:
    if(!isStreamed_) {
      unsigned int len=(unsigned int)ODBCXX_STRING_LEN(str);
      if(len+1>bufferSize_) {
	len=bufferSize_-1;
      }
      char* buf=this->data();
      // we want to pad CHARs with spaces
      unsigned int padlen=(sqlType_==Types::CHAR?bufferSize_-1-len:0);
      
      memcpy(buf,ODBCXX_STRING_DATA(str),len);
      for(unsigned int i=0; i<padlen; i++) {
	buf[len+i]=' ';
      }
      
      buf[len+padlen]=0; //NULL

      this->setDataStatus(len+padlen);
    } else {
      // we fake a real setStream()
      this->setStream(stringToStream(str),
		      ODBCXX_STRING_LEN(str));
      ownStream_=true;
    }
    break;
    
    ACCEPT_SET_VAL(SQL_C_BIT,signed char,stringToInt(str));
    ACCEPT_SET_VAL(SQL_C_TINYINT,signed char,stringToInt(str));
    ACCEPT_SET_VAL(SQL_C_SHORT,short,stringToInt(str));
    ACCEPT_SET_VAL(SQL_C_LONG,SQLINTEGER,stringToInt(str));
    ACCEPT_SET_VAL(SQL_C_DOUBLE,double,stringToDouble(str));
    ACCEPT_SET_VAL(SQL_C_FLOAT,float,stringToDouble(str));

  case C_DATE:
    this->setDate(Date(str));
    break;
    
  case C_TIME:
    this->setTime(Time(str));
    break;

  case C_TIMESTAMP:
    this->setTimestamp(Timestamp(str));
    break;

  default:
    UNSUPPORTED_SET("a string");
  }
}

void DataHandler::setBytes(const ODBCXX_BYTES& b)
{
  switch(cType_) {
  case SQL_C_BINARY:
    if(!isStreamed_) {
      size_t l=ODBCXX_BYTES_SIZE(b);
      // truncate if needed
      if(l>bufferSize_) {
	l=bufferSize_;
      }

      memcpy(this->data(),ODBCXX_BYTES_DATA(b),l);
      this->setDataStatus(l);

    } else {
      // fake a setStream()
      this->setStream(bytesToStream(b),
		      ODBCXX_BYTES_SIZE(b));
      ownStream_=true;
    }
    break;

  default:
    UNSUPPORTED_SET("a const Bytes&");
  }
}

void DataHandler::setStream(ODBCXX_STREAM* s)
{
  this->resetStream();
  stream_=s;
  ownStream_=true;
}

void DataHandler::setStream(ODBCXX_STREAM* s, int len)
{
  switch(cType_) {
  case SQL_C_CHAR:
  case SQL_C_BINARY:
    if(isStreamed_) {
      this->resetStream();
      stream_=s;
      ownStream_=false;
      this->setDataStatus(SQL_LEN_DATA_AT_EXEC(len));
      break;
    }
    
  default:
    UNSUPPORTED_SET("an stream");
  }
}
