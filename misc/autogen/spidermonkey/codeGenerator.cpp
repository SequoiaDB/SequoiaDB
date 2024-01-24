#include "codeGenerator.hpp"

static const char *_srcFileList[] = {
   SPIDER_MONKEY_SOURCE_FILE_LIST
} ;

#define SRC_FILE_NUM (sizeof( _srcFileList ) / sizeof( const char * ))

#if defined (GENERAL_JS_CODE_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( codeGenerator, GENERAL_JS_CODE_FILE ) ;
#endif

codeGenerator::codeGenerator() : _isFinish( false )
{
}

codeGenerator::~codeGenerator()
{
}

int codeGenerator::init()
{
   return 0 ;
}

bool codeGenerator::hasNext()
{
   return !_isFinish ;
}

int codeGenerator::outputFile( int id, fileOutStream &fout,
                               string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int k  = 0 ;
   vector<size_t> fileSizeList ;

   outputPath = JS_IN_CPP_FILE_PATH ;

   fout << std::left << SPIDER_MONKEY_CODE_STATEMENT << "\n"
        << "#include \"ossTypes.h\"\n"
           "#include \"ossUtil.h\"\n"
           "\n" ;

   for ( i = 0; i < SRC_FILE_NUM; ++i )
   {
      size_t len = 0 ;
      const char *tmp = NULL ;
      string path = utilCatPath( ENGINE_SPT_PATH, _srcFileList[i] ) ;
      string content ;

      path += ".js" ;

      rc = utilGetFileContent( path.c_str(), content ) ;
      if ( rc )
      {
         printLog( PD_ERROR ) << "Failed to get file content: path = "
                              << path << endl ;
         goto error ;
      }

      len = content.length() ;
      tmp = content.c_str() ;

      fileSizeList.push_back( len ) ;

      fout << "static CHAR " << _srcFileList[i] << "_js[] = {" ;

      for( k = 0; k < len; ++k )
      {
         if ( k > 0 )
         {
            fout << "," ;
         }

         fout << (int)tmp[k] ;
      }

      fout << "};\n"
              "\n" ;
   }

   fout << "static CHAR* jsNameArray[] = {" ;
   for ( i = 0; i < SRC_FILE_NUM; ++i )
   {
      if ( i > 0 )
      {
         fout << "," ;
      }

      fout << "\"" << _srcFileList[i] << ".js\"" ;
   }
   fout << "};\n" ;

   fout << "static CHAR* jsTextArray[] = {" ;
   for ( i = 0; i < SRC_FILE_NUM; ++i )
   {
      if ( i > 0 )
      {
         fout << "," ;
      }

      fout << _srcFileList[i] << "_js" ;
   }
   fout << "};\n" ;

   fout << "static UINT32 jsLenArray[] = {" ;
   for ( i = 0; i < SRC_FILE_NUM; ++i )
   {
      if ( i > 0 )
      {
         fout << "," ;
      }

      fout << fileSizeList[i] ;
   }
   fout << "};\n"
           "\n" ;

   fout << "\
static INT32 evalInitScripts( engine::_sptScope * scope )\n\
{\n\
   UINT32 i = 0 ;\n\
   UINT32 len = sizeof ( jsNameArray ) / sizeof ( CHAR * ) ;\n\
   INT32 rc = SDB_OK ;\n\
   for ( ; i < len ; i++ )\n\
   {\n\
      rc = scope->eval( jsTextArray[i] , jsLenArray[i] , jsNameArray[i] ,\n\
                        1 , SPT_EVAL_FLAG_NONE, NULL ) ;\n\
      if ( rc != SDB_OK )\n\
      {\n\
         PD_LOG ( PDERROR , \"fail to eval init script: %s, rc=%d\" , jsNameArray[i] , rc ) ;\n\
         break ;\n\
      }\n\
   }\n\
   return rc ;\n\
}\n" ;

done:
   _isFinish = true ;
   return rc ;
error:
   goto done ;
}


