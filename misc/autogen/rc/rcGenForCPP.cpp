#include "rcGenForCPP.hpp"

#if defined (GENERAL_RC_CPP_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( rcGenForCPP, GENERAL_RC_CPP_FILE ) ;
#endif

rcGenForCPP::rcGenForCPP() : _isFinish( false )
{
}

rcGenForCPP::~rcGenForCPP()
{
}

bool rcGenForCPP::hasNext()
{
   return !_isFinish ;
}

int rcGenForCPP::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_rcInfoList.size() ;
   string headerDesc ;

   outputPath = RC_CPP_FILE_PATH ;

   rc = _buildStatement( 1, headerDesc ) ;
   if ( rc )
   {
      printLog( PD_ERROR ) << "failed to build file header desc" << endl ;
      rc = 1 ;
      goto error ;
   }

   fout << std::left
        << headerDesc << "\n"
        << "#include \"ossErr.h\"\n\n"
        << "const CHAR* getErrDesp ( INT32 errCode )\n"
           "{\n"
           "   INT32 code = -errCode ;\n"
           "   const static CHAR* errDesp[] =\n"
           "   {\n"
           "      \"Succeed\",\n" ;

   for (int i = 0; i < listSize; ++i )
   {
      fout << "      \"" << _rcInfoList[i].getDesc( _lang ) << "\""
           << ( ( i + 1 < listSize ) ? ",\n" : "\n" ) ;

   }

   fout << "   } ;\n"
           "\n"
           "   if ( code < 0 || (UINT32)code >= ( sizeof ( errDesp ) / sizeof ( CHAR* ) ) )\n"
           "   {\n"
           "      return \"unknown error\" ;\n"
           "   }\n"
           "\n"
           "   return errDesp[code] ;\n"
           "}\n" ;

done:
   _isFinish = true ;
   return rc ;
error:
   goto done ;
}