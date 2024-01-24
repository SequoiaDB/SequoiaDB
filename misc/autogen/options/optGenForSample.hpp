#ifndef OPT_GEN_FOR_C_HPP
#define OPT_GEN_FOR_C_HPP

#include "optGeneratorBase.hpp"

#define OPT_C_FILE_PATH ENGINE_PATH"include/pmdOptions.h"

#define OPT_SAMPLE_COORD_PATH       SAMPLES_PATH"sdb.conf.coord"
#define OPT_SAMPLE_CATALOG_PATH     SAMPLES_PATH"sdb.conf.catalog"
#define OPT_SAMPLE_DATA_PATH        SAMPLES_PATH"sdb.conf.data"
#define OPT_SAMPLE_STANDALONE_PATH  SAMPLES_PATH"sdb.conf.standalone"

class optGenForSample : public optGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   optGenForSample() ;
   ~optGenForSample() ;

   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout, string &outputPath ) ;
   const char* name(){ return "options for Sample" ; }

private:
   int _genCoordConfig( fileOutStream &fout, string &outputPath ) ;
   int _genCatalogdConfig( fileOutStream &fout, string &outputPath ) ;
   int _genDataConfig( fileOutStream &fout, string &outputPath ) ;
   int _genStandaloneConfig( fileOutStream &fout, string &outputPath ) ;
   void _genConfPair( fileOutStream &fout,
                      const string &key, const string &value1,
                      const string &value2, const string &desc ) ;
private:
   bool _isFinish ;
} ;

#endif