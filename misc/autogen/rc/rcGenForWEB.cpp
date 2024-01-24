#include "rcGenForWEB.hpp"

#if defined (GENERAL_RC_WEB_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( rcGenForWEB, GENERAL_RC_WEB_FILE ) ;
#endif

rcGenForWEB::rcGenForWEB() : _isFinish( false )
{
}

rcGenForWEB::~rcGenForWEB()
{
}

bool rcGenForWEB::hasNext()
{
   return !_isFinish ;
}

int rcGenForWEB::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_rcInfoList.size() ;
   const char *lang = _getLang( id ) ;
   string headerDesc ;

   outputPath = RC_WEB_FILE_PATH + string( lang ) + ".php" ;

   fout << std::left
        << "<?php\n"
           "$errno_" << lang << " = array(\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      string first = _rcInfoList[i].name ;
      string second = _rcInfoList[i].getDesc( lang );

      // replace all "$" to "\$" for web
      first  = utilReplaceAll( first,  "$", "\\$" ) ;
      second = utilReplaceAll( second, "$", "\\$" ) ;

      fout << setw(6) << _rcInfoList[i].value
           << " => \"" << first << ": " << second << "\"" ;
      if ( i + 1 < listSize )
      {
         fout << "," ;
      }

      fout << "\n" ;
   }

   fout << ") ;\n"
           "?>\n" ;

   if ( id + 1 == _langListSize() )
   {
      _isFinish = true ;
   }
   return rc ;
}