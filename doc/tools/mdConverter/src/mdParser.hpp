
#ifndef MDPARSER_HPP__
#define MDPARSER_HPP__

#include <string>
#include <map>

#ifdef linux
#include <string.h>
#endif

#include "core.h"
#include "sundown/markdown.h"
#include "sundown/html.h"
using namespace std ;

class mdParser
{
private:
   struct sd_callbacks _callbacks ;
   struct html_renderopt _options ;
   struct sd_markdown *_pMarkdown ;
public:
   mdParser() ;
   ~mdParser() ;
   INT32 init( INT32 level,
               string rootPath,
               string mdPath,
               string imgPath,
               INT32 edition,
               string filePath,
               BOOLEAN isFullPath = TRUE,
               string convertMode = "normal",
               BOOLEAN tabPage = TRUE,
               BOOLEAN navPage = TRUE,
               map<string, INT32> *pFileMap = NULL,
               map<string, string> *pCnMap = NULL ) ;
   INT32 parse( string mdContent, string version, string &htmlContent ) ;
} ;

INT32 convertHtmlHeader( INT32 level,
                         string anchor,
                         string contents,
                         string &htmlCode ) ;

#endif