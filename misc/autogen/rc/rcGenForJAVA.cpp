#include "rcGenForJAVA.hpp"

#if defined (GENERAL_RC_JAVA_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( rcGenForJAVA, GENERAL_RC_JAVA_FILE ) ;
#endif

rcGenForJAVA::rcGenForJAVA() : _isFinish( false )
{
}

rcGenForJAVA::~rcGenForJAVA()
{
}

bool rcGenForJAVA::hasNext()
{
   return !_isFinish ;
}

int rcGenForJAVA::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_rcInfoList.size() ;
   string headerDesc ;

   outputPath = RC_JAVA_FILE_PATH ;

   rc = _buildStatement( 1, headerDesc ) ;
   if ( rc )
   {
      printLog( PD_ERROR ) << "failed to build file header desc" << endl ;
      rc = 1 ;
      goto error ;
   }

   fout << std::left << headerDesc << "\n" ;

   fout << "package com.sequoiadb.exception;\n"
        << "\n"
        << "public enum SDBError {\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      const RCInfo& rcInfo = _rcInfoList[i] ;

      fout << "    "
           << std::left  << setfill( ' ' ) << setw( _maxFieldWidth + 4 )
           << rcInfo.name + "("
           << std::right << setfill( ' ' ) << setw( 5 )
           << rcInfo.value << ",    "
           << "\"" << rcInfo.getDesc( _lang ) << "\"" << "    )"
           << ( ( i + 1 < listSize ) ? ",\n" : ";\n" ) ;
    }

    fout << "\n"
            "    private int code;\n"
            "    private String desc;\n"
            "\n"
            "    private SDBError(int code, String desc) {\n"
            "        this.code = code;\n"
            "        this.desc = desc;\n"
            "    }\n"
            "\n"
            "    @Override\n"
            "    public String toString() {\n"
            "          return this.name() + \"(\" + this.code + \")\" + \": \" + this.desc;\n"
            "    }\n"
            "\n"
            "    public int getErrorCode() {\n"
            "          return this.code;\n"
            "    }\n"
            "\n"
            "    public String getErrorDescription() {\n"
            "          return this.desc;\n"
            "    }\n"
            "\n"
            "    public String getErrorType() {\n"
            "          return this.name();\n"
            "    }"
            "\n"
            "\n"
            "    public static SDBError getSDBError(int errorCode) {\n"
            "        switch(errorCode) {\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      const RCInfo& rcInfo = _rcInfoList[i] ;

      fout << "        case "
           << std::right << setfill(' ') << setw(5) << rcInfo.value
           << ": " << "return " << rcInfo.name << ";\n" ;
   }

   fout << "        default:    return null;\n"
           "        }\n"
           "    }\n"
           "}\n" ;

done:
   _isFinish = true ;
   return rc ;
error:
   goto done ;
}