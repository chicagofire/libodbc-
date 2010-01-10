/*
   This file is part of libodbc++.

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

#ifndef __ODBCXX_PARAMETERMETADATA_H
#define __ODBCXX_PARAMETERMETADATA_H

#include <odbc++/preparedstatement.h>

namespace odbc {

  class ODBCXX_EXPORT ParameterMetaData {
    friend class PreparedStatement;

  private:
     PreparedStatement *stmt_;

     ParameterMetaData( PreparedStatement *ps );

     ResultSet* _getTypeInfo(SQLSMALLINT type);

  public:
     /** Get the number of parameters in the prepared statement
      */
     int getParameterCount();

     /** Get the designated parameter's mode
      */
     int getParameterMode(int param);

     /** Get the designated parameter's SQL type
      */
     int getParameterType(int param);

     /** Get the designated parameter's database-specific type name
      */
     ODBCXX_STRING getParameterTypeName(int param);

      /** Get the designated parameter's specified column size
       */
      int getPrecision(int param);

     /** Get the designated parameter's number of digits to right of the decimal point
      */
     int getScale(int param);

     /** Check if null values are allowed in the designated parameter
      */
     int isNullable(int param);

     /** Check if values for the designated parameter can be signed numbers
      */
     bool isSigned(int param) { return 0; }
  };
}

#endif
