#include <odbc++/parametermetadata.h>
#include <odbc++/databasemetadata.h>
#include "datahandler.h"

namespace odbc {

	ParameterMetaData::ParameterMetaData( PreparedStatement *ps ) : stmt_(ps) { }

	int ParameterMetaData::getParameterCount() { return stmt_->numParams_; }

	int ParameterMetaData::getParameterMode(int param) { return stmt_->directions_[param]; }

	int ParameterMetaData::getParameterType(int param) { return stmt_->rowset_->getColumn(param)->sqlType_; }

    ResultSet *ParameterMetaData::_getTypeInfo(SQLSMALLINT type)
    {
      Connection* conn = stmt_->getConnection();
      Statement*  stmt = conn->createStatement();

      try {
        return stmt->_getTypeInfo(type);
      } catch(...) {
        ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stmt;
        throw;
      }
    }

	ODBCXX_STRING ParameterMetaData::getParameterTypeName(int param)
	{
        ODBCXX_STRING result;
        int odbcType = this->getParameterType(param);
        Connection *cn = this->stmt_->getConnection();
        ResultSet *typeInfo = this->_getTypeInfo(odbcType);

        if (typeInfo->next())
        {
            result = typeInfo->getString(1);
        }

        ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) typeInfo;

        return result;
#if 0
        // untested -- I imagine data types can be added, but this
        // could be used to cache the type info result set and then
        // refer to it

        if ( typeInfo_ == 0 )
        {
            DatabaseMetaData *md = cn->getMetaData();
            typeInfo_ = md->getTypeInfo(SQL_ALL_TYPES);
        }

        for(; typeInfo_->first(); typeInfo_->next()) {
            if (typeInfo_->getShort(2) == odbcType)
            {
                result = typeInfo_->getString(1);
            }
        }

        return result;
#endif
	}

	int ParameterMetaData::getPrecision(int param) { return stmt_->rowset_->getColumn(param)->precision_; }

	int ParameterMetaData::getScale(int param) { return stmt_->rowset_->getColumn(param)->scale_; }

	int ParameterMetaData::isNullable(int param) { return stmt_->rowset_->getColumn(param)->nullable_; }

}
