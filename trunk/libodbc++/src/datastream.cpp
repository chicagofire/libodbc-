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

#include <odbc++/types.h>
#include <odbc++/errorhandler.h>

#include "datastream.h"

using namespace odbc;
using namespace std;

#if !defined(ODBCXX_QT)

DataStreamBuf::DataStreamBuf(ErrorHandler* eh, SQLHSTMT hstmt, int col, 
			     int cType,SQLINTEGER& dataStatus)
  :errorHandler_(eh), 
   hstmt_(hstmt), 
   column_(col), 
   cType_(cType),
   dataStatus_(dataStatus)
{
  switch(cType_) {
  case SQL_C_BINARY:
    bufferSize_=GETDATA_CHUNK_SIZE;
    break;
    
  case SQL_C_CHAR:
    bufferSize_=GETDATA_CHUNK_SIZE+1;
    break;

  default:
    throw SQLException
      ("[libodbc++]: internal error, constructed stream for invalid type");
  }
  
  char* gbuf=new char[bufferSize_];
  this->setg(gbuf,gbuf+bufferSize_,gbuf+bufferSize_);

  // fetch the first chunk of data - otherwise we don't know whether it's
  // NULL or not
  try {
    this->underflow();
  } catch(...) {
    delete[] gbuf;
    throw;
  }
}

DataStreamBuf::~DataStreamBuf()
{
  delete[] this->eback();
}

//virtual
int DataStreamBuf::underflow()
{
  if(gptr()<egptr()) {
    return (unsigned char) *gptr();
  }
  
  //after the call, this is the number of bytes that were 
  //available _before_ the call
  SQLINTEGER bytes;

  //the actual number of bytes that should end up in our buffer
  //we don't care about NULL termination
  size_t bs=(cType_==SQL_C_CHAR?bufferSize_-1:bufferSize_);

  SQLRETURN r=SQLGetData(hstmt_,column_,
			 cType_,
			 (SQLCHAR*)this->eback(),
			 bufferSize_,
			 &bytes);
  dataStatus_=bytes;
  
  errorHandler_->_checkStmtError(hstmt_,
				 r,"Error fetching chunk of data");

  if(r==ODBC3_C(SQL_NO_DATA,SQL_NO_DATA_FOUND)) {
    return EOF;
  }
  
  switch(bytes) {
  case SQL_NULL_DATA:
    return EOF;
    break;
    
  case SQL_NO_TOTAL:
    //The driver could not determine the size of the data
    //We can assume that the bytes read == our buffer size
    bytes=bs;
    break;

  case 0:
    return EOF;

  default:
    //as we're going to use bytes to set up our
    //pointers below, we adjust it to the number of bytes
    //we read
    if(bytes>(SQLINTEGER)bs) {
      bytes=bs;
    }
    break;
  }

  this->setg(this->eback(), this->eback(), this->eback()+bytes);
  return (unsigned char) *this->gptr();
}

#else // defined(ODBCXX_QT)


// QT QIODevice based implementation
// duplicates lots of code from above, but mixing the two up would become
// really ugly

DataStream::DataStream(ErrorHandler* eh, SQLHSTMT hstmt, int col, 
		       int cType,SQLINTEGER& dataStatus)
  :errorHandler_(eh), 
   hstmt_(hstmt), 
   column_(col), 
   cType_(cType),
   dataStatus_(dataStatus),
   bufferStart_(0),
   bufferEnd_(0),
   eof_(false)
{
  switch(cType_) {
  case SQL_C_BINARY:
    bufferSize_=GETDATA_CHUNK_SIZE;
    break;
    
  case SQL_C_CHAR:
    bufferSize_=GETDATA_CHUNK_SIZE+1;
    break;

  default:
    throw SQLException
      ("[libodbc++]: internal error, constructed stream for invalid type");
  }

  buffer_=new char[bufferSize_];
  try {
    this->_readStep(); // now we know whether we're NULL or not
  } catch(...) {
    // avoid leaking memory
    delete[] buffer_;
  }
}

DataStream::~DataStream()
{
  delete[] buffer_;
}

// private
void DataStream::_readStep()
{
  SQLINTEGER bytes;
  
  // see above
  size_t bs=(cType_==SQL_C_CHAR?bufferSize_-1:bufferSize_);
  
  SQLRETURN r=SQLGetData(hstmt_,column_,
			 cType_,
			 (SQLPOINTER)buffer_,
			 bufferSize_,
			 &bytes);

  dataStatus_=bytes; // now a caller of ResultSet::wasNull() knows if this is NULL

  errorHandler_->_checkStmtError(hstmt_,
				 r,"Error fetching chunk of data");

  if(r==ODBC3_C(SQL_NO_DATA,SQL_NO_DATA_FOUND)) {
    eof_=true;
    return;
  }

  switch(bytes) {
  case 0:
  case SQL_NULL_DATA:
    eof_=true;
    break;

  case SQL_NO_TOTAL:
    bytes=bs;
    break;

  default:
    if(bytes>(SQLINTEGER)bs) {
      bytes=bs;
    }
    break;
  }

  bufferStart_=0;
  bufferEnd_=(size_t)bytes;
}


DataStream::BlockRetType DataStream::readBlock(char* data, BlockLenType len)
{
  size_t bytesRead=0;
  while(!eof_ && bytesRead<len) {

    if(bufferEnd_-bufferStart_==0) {
      this->_readStep();
    }

    size_t b=bufferEnd_-bufferStart_;
    if(b>0) {

      if(b>len-bytesRead) {
	b=len-bytesRead;
      }

      memcpy((void*)data,(void*)&buffer_[bufferStart_],b);
      bufferStart_+=b;
      bytesRead+=b;
    }
  }

  return (int)bytesRead;
}

int DataStream::getch()
{
  if(eof_) {
    return -1;
  } 

  if(bufferEnd_-bufferStart_>0) {
    return (unsigned char)buffer_[bufferStart_++];
  }

  this->_readStep();
  if(eof_ || bufferEnd_-bufferStart_==0) {
    return -1;
  }
  return (unsigned char)buffer_[bufferStart_++];
}


#endif
