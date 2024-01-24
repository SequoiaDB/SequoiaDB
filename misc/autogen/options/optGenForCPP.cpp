#include "optGenForCPP.hpp"

#if defined (GENERAL_OPT_CPP_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( optGenForCPP, GENERAL_OPT_CPP_FILE ) ;
#endif

optGenForCPP::optGenForCPP() : _isFinish( false )
{
}

optGenForCPP::~optGenForCPP()
{
}

bool optGenForCPP::hasNext()
{
   return !_isFinish ;
}

int optGenForCPP::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_optionList.size() ;
   string headerDesc ;

   outputPath = OPT_CPP_FILE_PATH ;

   rc = _buildStatement( 1, headerDesc ) ;
   if ( rc )
   {
      printLog( PD_ERROR ) << "failed to build file header desc" << endl ;
      rc = 1 ;
      goto error ;
   }

   fout << std::left << headerDesc << "\n" ;

   fout << "#ifndef PMDOPTIONS_HPP_\n"
           "#define PMDOPTIONS_HPP_\n"
           "\n"
           "#include \"pmdOptions.h\"\n"
           "#define PMD_COMMANDS_OPTIONS \\\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      OPTION_ELE optEle = _optionList[i] ;

      if ( !optEle.hiddentag )
      {
         if ( 0 != optEle.shorttag )
         {
            fout << "        ( PMD_COMMANDS_STRING ("
                 << optEle.nametag << ", \"," << optEle.shorttag << "\"), " ;
         }
         else
         {
            fout << "        ( " << optEle.nametag <<", " ;
         }

         if ( optEle.typetag.length() > 0 )
         {
            fout << "boost::program_options::value<"
                 << optEle.typetag << ">(), " ;
         }

         fout << "\"" << optEle.getDesc( _lang ) << "\" ) \\\n" ;
      }
   }

   fout << "\n"
           "#define PMD_HIDDEN_COMMANDS_OPTIONS \\\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      OPTION_ELE optEle = _optionList[i] ;

      if ( optEle.hiddentag )
      {
         if ( 0 != optEle.shorttag )
         {
            fout << "        ( PMD_COMMANDS_STRING ("
                 << optEle.nametag << ", \"," << optEle.shorttag << "\"), " ;
         }
         else
         {
            fout << "        ( " << optEle.nametag << ", " ;
         }

         if ( optEle.typetag.length() > 0 )
         {
            fout << "boost::program_options::value<"
                 << optEle.typetag << ">(), " ;
         }

         fout << "\"" << optEle.getDesc( _lang ) << "\" ) \\\n" ;
      }
   }

   fout << "\n"
           "#endif /* PMDOPTIONS_HPP_ */" ;

done:
   _isFinish = true ;
   return rc ;
error:
   goto done ;
}