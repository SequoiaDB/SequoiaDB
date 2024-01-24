#include "rcGenForDoc.hpp"

#if defined (GENERAL_RC_DOC_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( rcGenForDoc, GENERAL_RC_DOC_FILE ) ;
#endif

rcGenForDoc::rcGenForDoc() : _isFinish( false )
{
}

rcGenForDoc::~rcGenForDoc()
{
}

bool rcGenForDoc::hasNext()
{
   return !_isFinish ;
}

int rcGenForDoc::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_rcInfoList.size() ;
   string headerDesc ;

   outputPath = RC_MD_FILE_PATH ;

   fout << std::left
        << "| Name | Error Code | Description |" << endl
        << "| --- | --- | --- |" << endl ;

   for ( i = 0; i < listSize; ++i )
   {
      const RCInfo& rcInfo = _rcInfoList[i] ;

      if ( rcInfo.reserved )
      {
         continue;
      }

      fout << "| "  << rcInfo.name
           << " | " << rcInfo.value
           << " | " << rcInfo.getDesc( LANG_CN )
           << " |"  << endl ;
   }

   _isFinish = true ;
   return rc ;
}