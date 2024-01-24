#include "rcGenForPython.hpp"

#if defined (GENERAL_RC_PYTHON_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( rcGenForPython, GENERAL_RC_PYTHON_FILE ) ;
#endif

rcGenForPython::rcGenForPython() : _isFinish( false )
{
}

rcGenForPython::~rcGenForPython()
{
}

bool rcGenForPython::hasNext()
{
   return !_isFinish ;
}

int rcGenForPython::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_rcInfoList.size() ;
   string headerDesc ;

   outputPath = RC_PYTHON_FILE_PATH ;

   rc = _buildStatement( 2, headerDesc ) ;
   if ( rc )
   {
      printLog( PD_ERROR ) << "failed to build file header desc" << endl ;
      rc = 1 ;
      goto error ;
   }

   fout << std::left << headerDesc << "\n"
        << "class Errcode(object):\n"
           "    \"\"\"Errcode of SequoiaDB.\n"
           "    \"\"\"\n"
           "\n"
           "    def __init__(self, name, code, desc):\n"
           "        self.__name = name\n"
           "        self.__code = code\n"
           "        self.__desc = desc\n"
           "\n"
           "    @property\n"
           "    def name(self):\n"
           "        \"\"\"Name of this error code.\n"
           "        \"\"\"\n"
           "        return self.__name\n"
           "\n"
           "    @property\n"
           "    def code(self):\n"
           "        \"\"\"Code of this error code.\n"
           "        \"\"\"\n"
           "        return self.__code\n"
           "\n"
           "    @property\n"
           "    def desc(self):\n"
           "        \"\"\"Description of this error code.\n"
           "        \"\"\"\n"
           "        return self.__desc\n"
           "\n"
           "    def __eq__(self, other):\n"
           "        \"\"\"Errcode can equals to Errcode and int.\n"
           "        \"\"\"\n"
           "        if isinstance(other, Errcode):\n"
           "            return self.code == other.code\n"
           "        elif isinstance(other, int):\n"
           "            return self.code == other\n"
           "        else:\n"
           "            return False\n"
           "\n"
           "    def __ne__(self, other):\n"
           "        \"\"\"Errcode can not equals to Errcode and int.\n"
           "        \"\"\"\n"
           "        return not self.__eq__(other)\n"
           "\n"
           "    def __repr__(self):\n"
           "        return \"Errcode('%s', %d, '%s')\" % (self.name, self.code, self.desc)\n"
           "\n"
           "    def __str__(self):\n"
           "        return \"Errcode('%s', %d, '%s')\" % (self.name, self.code, self.desc)\n"
           "\n"
           "SDB_OK = Errcode(\"SDB_OK\", 0, \"OK\")\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      const RCInfo& rcInfo = _rcInfoList[i] ;

      fout << rcInfo.name << " = Errcode("
           << "\"" << rcInfo.name << "\", "
           << rcInfo.value << ", "
           << "\"" << rcInfo.getDesc( _lang ) << "\"" << ")\n" ;
   }

   fout << "\n"
           "_errcode_map = {\n";

   for ( i = 0; i < listSize; ++i )
   {
     const RCInfo& rcInfo = _rcInfoList[i] ;

     fout << "    " << rcInfo.value << ": " << rcInfo.name
          << ( ( i + 1 < listSize ) ? ",\n" : "\n" ) ;
   }

   fout << "}\n"
           "\n"
           "\n"
           "def get_errcode(code):\n"
           "    return _errcode_map.get(code)\n" ;

done:
   _isFinish = true ;
   return rc ;
error:
   goto done ;
}