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

#ifndef __DTCONV_H
#define __DTCONV_H

#include <odbc++/types.h>

using namespace std;

#include <cstdio>
#include <cstdlib>

#if defined(ODBCXX_QT)
# include <qiodevice.h>
# include <qbuffer.h>
#else
# if defined(ODBCXX_HAVE_SSTREAM)
#  include <sstream>
# else
#  include <strstream>
# endif
#endif
/* conversion functions string <-> int/float/double/long double
 */

// This is a pure mess, but seems to function (tm).

namespace odbc {
  
  const int INT_STR_LEN=12; //10 digits, sign, null
  const int LONG_STR_LEN=22; //20 digits, sign, null
  const int DOUBLE_STR_LEN=80; //???
    

  inline ODBCXX_STRING intToString(int i) {

#if defined(ODBCXX_QT)

    return QString::number(i);

#else

    char buf[INT_STR_LEN];

# if defined(WIN32) && defined(ODBCXX_HAVE__ITOA)
    _itoa(i,buf,10);
# elif defined(WIN32) && defined(ODBCXX_HAVE_ITOA)
    itoa(i,buf,10);
# else
    snprintf(buf,INT_STR_LEN,"%d",i);
# endif
    return std::string(buf);
#endif
  }
  
  inline int stringToInt(const ODBCXX_STRING& s) {
#if defined(ODBCXX_QT)
    return s.toInt();
#else
    return (int)strtol(s.c_str(),NULL,10);
#endif
  }
  
  inline ODBCXX_STRING longToString(Long l) {
    char buf[LONG_STR_LEN];
#if defined(WIN32) && defined(ODBCXX_HAVE__I64TOA)
    _i64toa(l,buf,10);
#else
    snprintf(buf,LONG_STR_LEN,
# if defined(PRId64)
	     "%" PRId64
# elif ODBCXX_SIZEOF_LONG==8
	     "%ld"
# else
	     "%lld"
# endif
	     ,l);
#endif // _i64toa
    return ODBCXX_STRING_C(buf);
  }

  inline odbc::Long stringToLong(const ODBCXX_STRING& _s) {
#if !defined(ODBCXX_QT)
    const char* s=ODBCXX_STRING_CSTR(_s);
#else
    QCString cs(_s.local8Bit());
    const char* s=cs.data();
#endif

#if defined(WIN32) && defined(ODBCXX_HAVE__ATOI64)
    return (Long)_atoi64(s);
#elif ODBCXX_SIZEOF_LONG==4 && defined(ODBCXX_HAVE_STRTOLL)
    return (Long)strtoll(s,NULL,10);
#elif ODBCXX_SIZEOF_LONG==4 && defined(ODBCXX_HAVE_STRTOQ)
    return (Long)strtoq(s,NULL,10);
#else
    // either 64bit platform, or I'm stupid.
    return (Long)strtol(s,NULL,10);
#endif
  }

  inline ODBCXX_STRING doubleToString(double d) {
    char buf[DOUBLE_STR_LEN];
#if defined(ODBCXX_HAVE_SNPRINTF)
    snprintf(buf,DOUBLE_STR_LEN,"%f",d);
#elif defined(ODBCXX_HAVE__SNPRINTF)
    _snprintf(buf,DOUBLE_STR_LEN,"%f",d);
#else
    sprintf(buf,"%f",d);
#endif
    return ODBCXX_STRING_C(buf);
  }
  
  inline double stringToDouble(const ODBCXX_STRING& s) {
#if defined(ODBCXX_QT)
    return s.toDouble();
#else
    return strtod(s.c_str(),NULL);
#endif
  }

  // stream stuff
  // this should return <=0 on EOF, and number of bytes 
  // read otherwise
  inline int readStream(ODBCXX_STREAM* s,
			char* buf,
			unsigned int maxlen) {
#if defined(ODBCXX_QT)
    return s->readBlock(buf,maxlen);
#else
    if(*s) {
      s->read(buf,maxlen);
      return (int)s->gcount();
    } else {
      return 0;
    }
#endif
  }

  // returns a newly allocated stream
  inline ODBCXX_STREAM* stringToStream(const ODBCXX_STRING& str) {
#if !defined(ODBCXX_QT)
    ODBCXX_SSTREAM* s=new ODBCXX_SSTREAM();
    *s << str;
    return s;
#else // defined(ODBCXX_QT)
    QBuffer* b=new QBuffer();
    b->open(IO_WriteOnly);
    b->writeBlock(ODBCXX_STRING_CSTR(str),ODBCXX_STRING_LEN(str));
    b->close();
    b->open(IO_ReadOnly);
    return b;
#endif
  }

  // this is rather ineffective...
  inline ODBCXX_STRING streamToString(ODBCXX_STREAM* s) {
    char buf[GETDATA_CHUNK_SIZE];
#if defined(ODBCXX_QT)
    int r;
    QString ret;
    while((r=s->readBlock(buf,GETDATA_CHUNK_SIZE))>0) {
      ret+=ODBCXX_STRING_CL(buf,r);
    }
#else
    std::string ret;
    while(s->read(buf,GETDATA_CHUNK_SIZE) || s->gcount()) {
      ret+=ODBCXX_STRING_CL(buf,s->gcount());
    }
#endif
    return ret;
  }

  inline ODBCXX_BYTES streamToBytes(ODBCXX_STREAM* s) {
    char buf[GETDATA_CHUNK_SIZE];
    char* bigbuf=NULL;
    unsigned int size=0;
    int r;
#if defined(ODBCXX_QT)
    while((r=s->readBlock(buf,GETDATA_CHUNK_SIZE))!=-1) {
      char* tmp=new char[size+(unsigned int)r];
      if(size>0) {
	memcpy((void*)tmp,(void*)bigbuf,size);
      }
      memcpy((void*)&tmp[size],buf,r);
      delete[] bigbuf;
      bigbuf=tmp;
      size+=(unsigned int)r;
    }

    // this should take care of deleting bigbuf
    return QByteArray().assign(bigbuf,size);
#else
    while(s->read(buf,GETDATA_CHUNK_SIZE) || s->gcount()) {
      char* tmp=new char[size+s->gcount()];
      if(size>0) {
	memcpy((void*)tmp,(void*)bigbuf,size);
      }
      memcpy((void*)&tmp[size],buf,s->gcount());
      delete[] bigbuf;
      bigbuf=tmp;
      size+=s->gcount();
    }

    // this copies the buffer's contents
    Bytes b((signed char*)bigbuf,size);
    delete[] bigbuf;
    return b;
#endif
  }

  inline ODBCXX_STREAM* bytesToStream(const ODBCXX_BYTES& b) {
#if !defined(ODBCXX_QT)
    ODBCXX_SSTREAM* s=new ODBCXX_SSTREAM();
    if(b.getSize()>0) {
      s->write((char*)b.getData(),b.getSize());
    }
    return s;
#else // ODBCXX_QT
    QBuffer* buf=new QBuffer(b.copy());
    buf->open(IO_ReadOnly);
    return buf;
#endif
  }

  
};


#endif
