#include "rcGenForJS.hpp"

#if defined (GENERAL_RC_JS_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( rcGenForJS, GENERAL_RC_JS_FILE ) ;
#endif

rcGenForJS::rcGenForJS() : _isFinish( false )
{
}

rcGenForJS::~rcGenForJS()
{
}

bool rcGenForJS::hasNext()
{
   return !_isFinish ;
}

int rcGenForJS::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_rcInfoList.size() ;

   outputPath = RC_JS_FILE_PATH ;

   fout << std::left << "/* Error Constants */\n" ;

   for ( i = 0; i < _conslist.size(); ++i )
   {
      fout << "const " << setw(_maxFieldWidth + 2) << _conslist[i].first << " = "
           << setw(6) << _conslist[i].second << ";\n" ;
   }

   fout << "\n"
        << "/* Error Codes */\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      fout << "const " << setw(_maxFieldWidth + 2) << _rcInfoList[i].name << " = "
           << setw(6) << -(i + 1) << "; // "
           << _rcInfoList[i].getDesc( _lang ) << ";\n" ;
   }

   fout << "\n"
           "function _getErr (errCode) {\n"
           "   var errDesp = [ \n"
           "      \"Succeed\",\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      fout << "      \"" << _rcInfoList[i].getDesc( _lang )
           << ( ( i + 1 < listSize ) ? "\",\n" : "\"\n" ) ;
   }

   fout << "   ]; \n"
           "   var index = -errCode ;\n"
           "   if ( index < 0 || index >= errDesp.length ) {\n"
           "      return \"unknown error\";\n"
           "   }\n"
           "   return errDesp[index] ;\n"
           "}\n"
           "function getErr (errCode) {\n"
           "   return _getErr ( errCode ) ;\n"
           "}\n" ;

done:
   _isFinish = true ;
   return rc ;
error:
   goto done ;
}