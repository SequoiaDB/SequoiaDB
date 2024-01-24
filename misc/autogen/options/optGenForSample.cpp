#include "optGenForSample.hpp"

#if defined (GENERAL_OPT_SAMPLE_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( optGenForSample, GENERAL_OPT_SAMPLE_FILE ) ;
#endif

optGenForSample::optGenForSample() : _isFinish( false )
{
}

optGenForSample::~optGenForSample()
{
}

bool optGenForSample::hasNext()
{
   return !_isFinish ;
}

int optGenForSample::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;

   if ( 0 == id )
   {
      rc = _genCoordConfig( fout, outputPath ) ;
   }
   else if ( 1 == id )
   {
      rc = _genCatalogdConfig( fout, outputPath ) ;
   }
   else if ( 2 == id )
   {
      rc = _genDataConfig( fout, outputPath ) ;
   }
   else if ( 3 == id )
   {
      rc = _genStandaloneConfig( fout, outputPath ) ;
      _isFinish = true ;
   }

   return rc ;
}

int optGenForSample::_genCoordConfig( fileOutStream &fout,
                                      string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_optionList.size() ;

   outputPath = OPT_SAMPLE_COORD_PATH ;

   fout << std::left << "# SequoiaDB configuration\n\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      OPTION_ELE optEle = _optionList[i] ;

      if ( !optEle.hiddentag && optEle.typetag.length() > 0 )
      {
         _genConfPair( fout, optEle.longtag, optEle.cordtag,
                       optEle.defttag, optEle.getDesc( _lang ) ) ;
         fout << "\n" ;
      }
   }

   return rc ;
}

int optGenForSample::_genCatalogdConfig( fileOutStream &fout,
                                         string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_optionList.size() ;

   outputPath = OPT_SAMPLE_CATALOG_PATH ;

   fout << std::left << "# SequoiaDB configuration\n\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      OPTION_ELE optEle = _optionList[i] ;

      if ( !optEle.hiddentag && optEle.typetag.length() > 0 )
      {
         _genConfPair( fout, optEle.longtag, optEle.catatag,
                       optEle.defttag, optEle.getDesc( _lang ) ) ;
         fout << "\n" ;
      }
   }

   return rc ;
}

int optGenForSample::_genDataConfig( fileOutStream &fout,
                                     string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_optionList.size() ;

   outputPath = OPT_SAMPLE_DATA_PATH ;

   fout << std::left << "# SequoiaDB configuration\n\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      OPTION_ELE optEle = _optionList[i] ;

      if ( !optEle.hiddentag && optEle.typetag.length() > 0 )
      {
         _genConfPair( fout, optEle.longtag, optEle.datatag,
                       optEle.defttag, optEle.getDesc( _lang ) ) ;
         fout << "\n" ;
      }
   }

   return rc ;
}

int optGenForSample::_genStandaloneConfig( fileOutStream &fout,
                                           string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_optionList.size() ;

   outputPath = OPT_SAMPLE_STANDALONE_PATH ;

   fout << std::left << "# SequoiaDB configuration\n\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      OPTION_ELE optEle = _optionList[i] ;

      if ( !optEle.hiddentag && optEle.typetag.length() > 0 )
      {
         _genConfPair( fout, optEle.longtag, optEle.standtag,
                       optEle.defttag, optEle.getDesc( _lang ) ) ;
         fout << "\n" ;
      }
   }

   return rc ;
}

void optGenForSample::_genConfPair ( fileOutStream &fout,
                                     const string &key, const string &value1,
                                     const string &value2, const string &desc )
{
   if ( desc.length() != 0 )
   {
      fout << "# " << desc << endl ;
   }
   if ( value1.length() != 0 )
   {
      fout << key << "=" << value1 << endl ;
   }
   else if ( value2.length() != 0 )
   {
      fout << key << "=" << value2 << endl ;
   }
   else
   {
      fout << "# " << key << "=" << endl ;
   }
}