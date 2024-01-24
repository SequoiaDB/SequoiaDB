#include "optGenForC.hpp"

#if defined (GENERAL_OPT_C_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( optGenForC, GENERAL_OPT_C_FILE ) ;
#endif

optGenForC::optGenForC() : _isFinish( false )
{
}

optGenForC::~optGenForC()
{
}

bool optGenForC::hasNext()
{
   return !_isFinish ;
}

int optGenForC::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_optionList.size() ;
   string headerDesc ;

   outputPath = OPT_C_FILE_PATH ;

   rc = _buildStatement( 1, headerDesc ) ;
   if ( rc )
   {
      printLog( PD_ERROR ) << "failed to build file header desc" << endl ;
      rc = 1 ;
      goto error ;
   }

   fout << std::left << headerDesc
        << "\n"
           "#ifndef PMDOPTIONS_H_\n"
           "#define PMDOPTIONS_H_\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      const OPTION_ELE& optEle = _optionList[i] ;

      fout << "#define " << std::setw( 64 )
           << optEle.nametag << "\"" << optEle.longtag << "\"\n" ;
   }

   fout << "#endif\n" ;

done:
   _isFinish = true ;
   return rc ;
error:
   goto done ;
}