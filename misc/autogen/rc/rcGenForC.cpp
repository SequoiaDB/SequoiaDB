#include "rcGenForC.hpp"

#if defined (GENERAL_RC_C_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( rcGenForC, GENERAL_RC_C_FILE ) ;
#endif

rcGenForC::rcGenForC() : _isFinish( false )
{
}

rcGenForC::~rcGenForC()
{
}

bool rcGenForC::hasNext()
{
   return !_isFinish ;
}

int rcGenForC::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_conslist.size() ;
   string headerDesc ;

   outputPath = RC_C_FILE_PATH ;

   rc = _buildStatement( 1, headerDesc ) ;
   if ( rc )
   {
      printLog( PD_ERROR ) << "failed to build file header desc" << endl ;
      rc = 1 ;
      goto error ;
   }

   fout << std::left << headerDesc << "\n" ;

   fout << "#ifndef OSSERR_H_\n"
           "#define OSSERR_H_\n"
           "\n"
           "#include \"core.h\"\n"
           "#include \"ossFeat.h\"\n"
           "\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      fout << "#define " << std::setw( _maxFieldWidth + 2 )
           << _conslist[i].first << _conslist[i].second << "\n" ;
   }

   fout << "\n" ;

   fout << "/** \\fn CHAR* getErrDesp ( INT32 errCode )\n"
           "    \\brief Error Code.\n"
           "    \\param [in] errCode The number of the error code\n"
           "    \\returns The meaning of the error code\n"
           " */\n" ;

   fout << "const CHAR* getErrDesp ( INT32 errCode ) ;" << "\n\n" ;

   for ( i = 0; i < _rcInfoList.size(); ++i )
   {
      fout << "#define "
           << std::setw( _maxFieldWidth + 2 ) << _rcInfoList[i].name
           << std::setw( 6 ) << _rcInfoList[i].value
           << "/**< " << _rcInfoList[i].getDesc( _lang ) << " */\n" ;
   }

   fout << "#endif /* OSSERR_H_ */\n" ;

done:
   _isFinish = true ;
   return rc ;
error:
   goto done ;
}