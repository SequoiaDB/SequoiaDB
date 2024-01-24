#include "rcGenForCS.hpp"

#if defined (GENERAL_RC_CS_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( rcGenForCS, GENERAL_RC_CS_FILE ) ;
#endif

rcGenForCS::rcGenForCS() : _isFinish( false )
{
}

rcGenForCS::~rcGenForCS()
{
}

bool rcGenForCS::hasNext()
{
   return !_isFinish ;
}

int rcGenForCS::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_rcInfoList.size() ;
   string headerDesc ;

   outputPath = RC_CS_FILE_PATH ;

   rc = _buildStatement( 1, headerDesc ) ;
   if ( rc )
   {
      printLog( PD_ERROR ) << "failed to build file header desc" << endl ;
      rc = 1 ;
      goto error ;
   }

   fout << std::left << headerDesc << "\n" ;

   fout << std::left
        << "namespace SequoiaDB\n"
           "{\n"
           "    class Errors\n"
           "    {\n"
           "        public enum errors : int\n"
           "        {\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      fout << "            "
           << setw(_maxFieldWidth + 2) << _rcInfoList[i].name << " = "
           << _rcInfoList[i].value
           << ( ( i + 1 < listSize ) ? ",\n" : "\n" ) ;
   }

   fout << "        } ;\n"
           "\n"
           "        public static readonly string[] descriptions = {\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      fout << "            \"" << _rcInfoList[i].getDesc( _lang ) << "\""
           << ( ( i + 1 < listSize ) ? ",\n" : "\n" ) ;
   }

   fout << "        } ;\n"
           "    }\n"
           "}" ;

done:
   _isFinish = true ;
   return rc ;
error:
   goto done ;
}