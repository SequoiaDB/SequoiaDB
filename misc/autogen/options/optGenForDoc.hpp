#ifndef OPT_GEN_FOR_DOC_HPP
#define OPT_GEN_FOR_DOC_HPP

#include "optGeneratorBase.hpp"

#define OPT_OTHER_DOC_FILENAME      "optOtherInfoForWeb.xml"
#define OPT_OTHER_DOC_FILE_PATH     "./" OPT_OTHER_DOC_FILENAME
#define OPT_RUNTIME_CONFIG_PATH     DOCUMENT_PATH"Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md"

#define OPT_FILTER_FOR_DOC_LIST \
"help",\
"version",\

typedef struct _optOtherInfoEle
{
   string title ;
   string subTitle ;

   // stentry tags
   string name ;
   string acronym ;
   string type ;
   string reloadable ;
   string reloadstrategy ;
   string desc ;

   // note tags
   string first ;
   string second ;
   string third ;
} OPTION_OTHER_INFO_ELE ;

class optGenForDoc : public optGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   optGenForDoc() ;
   ~optGenForDoc() ;

   int init() ;
   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout, string &outputPath ) ;
   const char* name(){ return "options for doc" ; }
private:
   int _loadOptOtherInfo() ;

private:
   bool _isFinish ;
   const char *_tmpLang ;
   vector<OPTION_OTHER_INFO_ELE> _optionOtherList ;
} ;

#endif