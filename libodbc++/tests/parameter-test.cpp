#include <odbc++/drivermanager.h>
#include <odbc++/connection.h>
#include <odbc++/preparedstatement.h>
#include <odbc++/resultset.h>
#include <odbc++/resultsetmetadata.h>
#include <odbc++/parametermetadata.h>

#include <iostream>
#include <memory>

using namespace odbc;
using namespace std;

int main(int argc, char** argv)
{
 if(argc!=3 &amp;&amp; argc!=5) {
   cerr << "Usage: " << argv[0] << " connect-string sql" << endl
        << "or     " << argv[0] << " dsn username password sql" << endl;
   return 0;
 }

 std::vector<ODBCXX_STRING> vargv(argc-1);
 const size_t MAX_CHARS = 256;
 for(int i=1;i<argc;++i)
 {
     ODBCXX_STRING&amp; arg=vargv[i-1];
#if defined(ODBCXX_UNICODE)
     wchar_t buffer[MAX_CHARS];
     size_t len=mbstowcs(buffer,argv[i],MAX_CHARS);
     if(0<len&amp;&amp;MAX_CHARS>len)
     {
        arg=buffer;
     }
#else
     arg=argv[i];
#endif
 }

 ODBCXX_STRING sql;
 std::auto_ptr<Connection> con;
 try {
   if(argc==3) {
     ODBCXX_COUT << ODBCXX_STRING_CONST("Connecting to ") << vargv[0]
                 << ODBCXX_STRING_CONST("...") << flush;
     con=std::auto_ptr<Connection>(DriverManager::getConnection(vargv[0]));
     sql = vargv[1];
   } else {
     ODBCXX_COUT << ODBCXX_STRING_CONST("Connecting to dsn=") << vargv[0]
                 << ODBCXX_STRING_CONST(", uid=") << vargv[1]
                 << ODBCXX_STRING_CONST(", pwd=") << vargv[2]
                 << ODBCXX_STRING_CONST("...") << flush;
     con=std::auto_ptr<Connection>(DriverManager::getConnection(vargv[0],vargv[1],vargv[2]));
     sql = vargv[3];
   }

   ODBCXX_COUT << ODBCXX_STRING_CONST("\n") << ODBCXX_STRING_CONST("sql=")
               << sql << ODBCXX_STRING_CONST("\n");

   PreparedStatement* ps=con->prepareStatement(sql);
   ParameterMetaData *pd = ps->getParameterMetaData();

   int m = pd->getParameterCount();
   if (m)
   {
       ODBCXX_COUT << ODBCXX_STRING_CONST("parameter types (odbc/database):\n");
       for(int i=1; i<=m; ++i)
       {
           ODBCXX_COUT << ODBCXX_STRING_CONST("parameter ") << i << ODBCXX_STRING_CONST(": ");
           ODBCXX_COUT << pd->getParameterType(i) << ODBCXX_STRING_CONST("/")
                       << pd->getParameterTypeName(i) << ODBCXX_STRING_CONST("\n");
       }
   }
   else
       ODBCXX_COUT << ODBCXX_STRING_CONST("no parameters\n");

   ODBCXX_COUT << ODBCXX_STRING_CONST("\n");

   ResultSetMetaData* md=ps->getMetaData();
   int n = md->getColumnCount();
   if ( n ) {
     ODBCXX_COUT << ODBCXX_STRING_CONST("result set (name/type):\n");
     for(int i=1; i<=n; ++i) {
        ODBCXX_COUT << ODBCXX_STRING_CONST("column ") << i << ODBCXX_STRING_CONST(": ");
        ODBCXX_COUT << md->getColumnName(i) << ODBCXX_STRING_CONST("/")
                    << md->getColumnType(i) << ODBCXX_STRING_CONST("\n");
     }
   }
   else
     ODBCXX_COUT << ODBCXX_STRING_CONST("no columns\n");

   ODBCXX_COUT << ODBCXX_STRING_CONST("\n");

 } catch(SQLException&amp; e) {
   ODBCXX_CERR << endl << e.getMessage() << endl;
 }

 return 0;
}