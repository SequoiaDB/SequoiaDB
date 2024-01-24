#include <stdio.h>
#include <iostream>
#include <map>
#include "core.h"
#include "cJSON2.h"
#include "iniReader.hpp"
#include "mdParser.hpp"
#include "system.hpp"
#include "options.hpp"
using namespace std ;

static string _rootPath ;
static string _localPath ;
static string _htmlPath ;
static string _cssFile ;
static string _jsFile ;
static string _language ;
static BOOLEAN _fullHtml ;
static BOOLEAN _fullPath ;
static map<string, INT32> _fileMap ;
static map<string, string> _cnMap ;

static string _mdPath ;
static string _imgPath ;
static string _mode ;
static string _convertMode ;

static BOOLEAN _tabPage ;
static BOOLEAN _navPage ;

static string _major ;
static string _minor ;

INT32 init() ;
INT32 check( options &opt ) ;
INT32 exec( options &opt ) ;
INT32 parseCmd( options &opt, INT32 argc, CHAR *argv[] ) ;
INT32 parseConf( string configPath ) ;
INT32 parseMarkdown( string md, string &html, string path, INT32 level, string title = "" ) ;
INT32 getLocalPath( string &path ) ;
INT32 execConfFiles( CJSON_MACHINE *pMachine,
                     const cJson_iterator *pIter,
                     BOOLEAN isFirst,
                     BOOLEAN &isFirstMd,
                     BOOLEAN isArray,
                     string subPath,
                     string cnPath,
                     INT32 level ) ;
INT32 execIterFiles( string path, string subPath ) ;
INT32 parseJson( const CHAR *pJson, CJSON_MACHINE *pMachine ) ;
INT32 outputHtmlHeader() ;
INT32 outputHtmlFooter() ;
INT32 buildFileMap( CJSON_MACHINE *pMachine,
                    const cJson_iterator *pIter,
                    BOOLEAN isFirst,
                    BOOLEAN isArray,
                    string subPath,
                    string cnPath,
                    INT32 level ) ;

INT32 main( INT32 argc, CHAR *argv[] )
{
   INT32 rc = SDB_OK ;
   options opt ;

   cout << endl ;

   rc = parseCmd( opt, argc, argv ) ;
   if( rc == SDB_SDB_HELP_ONLY )
   {
      goto done ;
   }
   else if( rc )
   {
      cout << "Failed to parse program options" << endl ;
      goto error ;
   }

   rc = init() ;
   if( rc )
   {
      goto error ;
   }

   rc = check( opt ) ;
   if( rc )
   {
      goto error ;
   }

   rc = exec( opt ) ;
   if( rc )
   {
      goto error ;
   }

done:
   if( rc == SDB_SDB_HELP_ONLY )
   {
      opt.printHelp() ;
      rc = SDB_OK ;
   }
   else if( rc == SDB_OK )
   {
      cout << "Success convert." << endl ;
   }
   else
   {
      cout << "Failed convert." << endl ;
   }
   return rc ;
error:
   goto done ;
}

INT32 init()
{
   INT32 rc = SDB_OK ;
   rc = getLocalPath( _localPath ) ;
   if( rc )
   {
      cout << "Failed to get local path." << endl ;
      goto error ;
   }

   rc = parseConf( _localPath + "mdConverter.ini" ) ;
   if( rc )
   {
      cout << "Failed to parse ini file, path:" << _localPath + "mdConverter.ini" << endl ;
      goto error ;
   }

   _rootPath = _localPath + _rootPath ;
   _htmlPath = _rootPath + "build" ;

   rc = mkdir( _htmlPath ) ;
   if( rc )
   {
      cout << "1 Failed to create dir, path: " << _htmlPath << endl ;
      goto error ;
   }

   rc = mkdir( _htmlPath + "/output" ) ;
   if( rc )
   {
      cout << "2 Failed to create dir, path: " << _htmlPath + "/output" << endl ;
      goto error ;
   }

   _htmlPath = _htmlPath + "/mid" ;
   rc = mkdir( _htmlPath ) ;
   if( rc )
   {
      cout << "3 Failed to create dir, path: " << _htmlPath << endl ;
      goto error ;
   }

   cout << "Local path: " << _localPath << endl ;
   cout << "Root path:  " << _rootPath << endl ;

done:
   return rc ;
error:
   goto done ;
}

INT32 check( options &opt )
{
   INT32 rc = SDB_OK ;
   BOOLEAN isDefault = FALSE ;
   string fullHtml ;
   string fullPath ;

   rc = opt.getOption( "m", _mode ) ;
   if( rc )
   {
      cout << "Failed to get -m options" << endl ;
      goto error ;
   }

   rc = opt.getOption( "v", _major ) ;
   if( rc )
   {
      cout << "Failed to get -v options" << endl ;
      goto error ;
   }
   
   rc = opt.getOption( "e", _minor ) ;
   if( rc )
   {
      cout << "Failed to get -e options" << endl ;
      goto error ;
   }

   rc = opt.getOption( "a", _language ) ;
   if( rc )
   {
      cout << "Failed to get -a options" << endl ;
      goto error ;
   }

   if( _mode == "conf" )
   {
      string confPath ;
      rc = opt.getOption( "c", confPath, &isDefault ) ;
      if( rc )
      {
         cout << "Failed to get -c options" << endl ;
         goto error ;
      }
      if( isDefault )
      {
         confPath = _rootPath + confPath ;
      }
      rc = fileIsExist( confPath ) ;
      if( rc )
      {
         cout << "File not exists, path: " << confPath << endl ;
         goto error ;
      }
   }
   else if( _mode == "iter" )
   {
      string documentPath ;
      rc = opt.getOption( "p", documentPath, &isDefault ) ;
      if( rc )
      {
         cout << "Failed to get -p options" << endl ;
         goto error ;
      }
      if( isDefault )
      {
         documentPath = _rootPath + documentPath ;
      }
      rc = fileIsExist( documentPath ) ;
      if( rc )
      {
         cout << "File not exists, path: " << documentPath << endl ;
         goto error ;
      }
   }
   else if( _mode == "file" )
   {
      string filePath ;
      string outputPath ;
      rc = opt.getOption( "f", filePath ) ;
      if( rc )
      {
         cout << "Failed to get -f options" << endl ;
         goto error ;
      }
      rc = fileIsExist( filePath ) ;
      if( rc )
      {
         cout << "File not exists, path: " << filePath << endl ;
         goto error ;
      }
      rc = opt.getOption( "o", outputPath ) ;
      if( rc )
      {
         cout << "Failed to get -o options" << endl ;
         goto error ;
      }
   }
   else
   {
      cout << "Unknow mode." << endl ;
   }

   rc = opt.getOption( "u", fullHtml ) ;
   if( rc )
   {
      cout << "Failed to get -u options" << endl ;
      goto error ;
   }

   if( fullHtml == "true" )
   {
      _fullHtml = TRUE ;
   }
   else
   {
      _fullHtml = FALSE ;
   }

   {
      string tmp ;

      rc = opt.getOption( "t", tmp ) ;
      if( rc )
      {
         cout << "Failed to get -t options" << endl ;
         goto error ;
      }

      if( tmp == "true" )
      {
         _tabPage = TRUE ;
      }
      else
      {
         _tabPage = FALSE ;
      }
   }

   {
      string tmp ;

      rc = opt.getOption( "n", tmp ) ;
      if( rc )
      {
         cout << "Failed to get -n options" << endl ;
         goto error ;
      }

      if( tmp == "true" )
      {
         _navPage = TRUE ;
      }
      else
      {
         _navPage = FALSE ;
      }
   }

   rc = opt.getOption( "s", _cssFile ) ;
   if( rc )
   {
      rc = SDB_OK ;
   }
   else
   {
      if( _fullHtml == FALSE )
      {
         cout << "include css, -u must TRUE" << endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = fileIsExist( _cssFile ) ;
      if( rc )
      {
         cout << "File not exists, path: " << _cssFile << endl ;
         goto error ;
      }
   }

   rc = opt.getOption( "j", _jsFile ) ;
   if( rc )
   {
      rc = SDB_OK ;
   }
   else
   {
      if( _fullHtml == FALSE )
      {
         cout << "include javascript, -u must TRUE" << endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = fileIsExist( _jsFile ) ;
      if( rc )
      {
         cout << "File not exists, path: " << _jsFile << endl ;
         goto error ;
      }
   }

   rc = opt.getOption( "d", _convertMode ) ;
   if( rc )
   {
      cout << "Failed to get -d options" << endl ;
      goto error ;
   }
   if( _convertMode == "website" && _mode != "conf" )
   {
      rc = SDB_INVALIDARG ;
      cout << "URL mode is website, Convert mode must be conf." << endl ;
      goto error ;
   }

   rc = opt.getOption( "l", fullPath ) ;
   if( rc )
   {
      cout << "Failed to get -l options" << endl ;
      goto error ;
   }
   if( fullPath == "true" )
   {
      _fullPath = TRUE ;
   }
   else
   {
      _fullPath = FALSE ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 exec( options &opt )
{
   INT32 rc = SDB_OK ;
   BOOLEAN isDefault = FALSE ;

   if( _mode == "conf" )
   {
      BOOLEAN isFirstMd = TRUE ;
      string confPath ;
      string confContent ;
      CJSON_MACHINE *pMachine = NULL ;
      const cJson_iterator *pIter = NULL ;
      const cJson_iterator *pIter2 = NULL ;

      opt.getOption( "c", confPath, &isDefault ) ;
      if( isDefault )
      {
         confPath = _rootPath + confPath ;
      }
      rc = file_get_contents( confPath, confContent ) ;
      if( rc )
      {
         cout << "File not exists, path: " << confPath << endl ;
         goto error ;
      }
      pMachine = cJsonCreate() ;
      if( pMachine == NULL )
      {
         rc = SDB_OOM ;
         cout << "Failed to call cJsonCreate." << endl ;
         goto error ;
      }
      rc = parseJson( confContent.c_str(), pMachine ) ;
      if( rc )
      {
         cJsonRelease( pMachine ) ;
         cout << "Failed to parse json, path: " << confPath << endl ;
         goto error ;
      }
      pIter = cJsonIteratorInit( pMachine ) ;
      if( pIter == NULL )
      {
         cJsonRelease( pMachine ) ;
         rc = SDB_OOM ;
         cout << "Failed to init iterator" << endl ;
         goto error ;
      }
      pIter2 = cJsonIteratorInit( pMachine ) ;
      if( pIter2 == NULL )
      {
         cJsonRelease( pMachine ) ;
         rc = SDB_OOM ;
         cout << "Failed to init iterator" << endl ;
         goto error ;
      }
      rc = buildFileMap( pMachine, pIter2, TRUE, FALSE, "", "", 1 ) ;
      if( rc )
      {
         cJsonRelease( pMachine ) ;
         cout << "Failed to build files map, path: " << confPath << endl ;
         goto error ;
      }
      if( _convertMode == "single" || _convertMode == "word" )
      {
         rc = outputHtmlHeader() ;
         if( rc )
         {
            cJsonRelease( pMachine ) ;
            rc = SDB_SYS ;
            cout << "Failed output html header." << endl ;
            goto error ;
         }
      }
      rc = execConfFiles( pMachine, pIter, TRUE, isFirstMd, FALSE, "", "", 1 ) ;
      if( rc )
      {
         cJsonRelease( pMachine ) ;
         cout << "Failed to convert files, path: " << confPath << endl ;
         goto error ;
      }
      if( _convertMode == "single" || _convertMode == "word" )
      {
         rc = outputHtmlFooter() ;
         if( rc )
         {
            cJsonRelease( pMachine ) ;
            rc = SDB_SYS ;
            cout << "Failed output html footer." << endl ;
            goto error ;
         }
      }
      cJsonRelease( pMachine ) ;
   }
   else if( _mode == "iter" )
   {
      string documentPath ;
      opt.getOption( "p", documentPath, &isDefault ) ;
      if( isDefault )
      {
         documentPath = _rootPath + documentPath ;
      }
      if( _convertMode == "single" || _convertMode == "word" )
      {
         rc = outputHtmlHeader() ;
         if( rc )
         {
            rc = SDB_SYS ;
            cout << "Failed output html header." << endl ;
            goto error ;
         }
      }
      rc = execIterFiles( documentPath, _htmlPath ) ;
      if( rc )
      {
         cout << "Failed to convert files, path: " << documentPath << endl ;
         goto error ;
      }
      if( _convertMode == "single" || _convertMode == "word" )
      {
         rc = outputHtmlFooter() ;
         if( rc )
         {
            rc = SDB_SYS ;
            cout << "Failed output html footer." << endl ;
            goto error ;
         }
      }
   }
   else if( _mode == "file" )
   {
      string mdContent ;
      string filePath ;
      string html ;
      string outputPath ;
      opt.getOption( "f", filePath ) ;
      opt.getOption( "o", outputPath, &isDefault ) ;
      if( isDefault )
      {
         outputPath = _rootPath + outputPath ;
      }
      rc = file_get_contents( filePath, mdContent ) ;
      if( rc )
      {
         cout << "Failed to get file content, path: " << filePath << endl ;
         goto error ;
      }
      rc = parseMarkdown( mdContent, html, "", 1 ) ;
      if( rc )
      {
         cout << "Failed to convert markdown file, path: " << filePath << endl ;
         goto error ;
      }
      rc = file_put_contents( outputPath, html ) ;
      if( rc )
      {
         cout << "Failed to put file, path: " << outputPath << endl ;
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 parseCmd( options &opt, INT32 argc, CHAR *argv[] )
{
   INT32 rc = SDB_OK ;
   BOOLEAN isDefault = FALSE ;
   string value ;

   opt.setOptions( "", "", "Command options:" ) ;
   opt.setOptions( "h", "help", "help" ) ;
   opt.setOptions( "v", "1", "major version, default: 1" ) ;
   opt.setOptions( "e", "0", "minor version, default: 0" ) ;
   opt.setOptions( "m", "conf", "Load mode, default: conf. "
                                "[conf] configuration mode: read the JSON file to do the conversion. "
                                "[iter] Iteration mode: all the specified directory files conversion. "
                                "[file] Single file mode: specified the conversion of a file. " ) ;
   opt.setOptions( "u", "true", "Generates a complete HTML code, include the HTML header, default: true" ) ;
   opt.setOptions( "s", "", "Load css file path" ) ;
   opt.setOptions( "j", "", "Load javascript file path" ) ;
   opt.setOptions( "l", "true", "Full path, default: true. use image and md link." ) ;
   opt.setOptions( "a", "cn", "Language, [cn] Simplified Chinese; [en] English." ) ;
   opt.setOptions( "d", "normal", "Conversion mode, default normal. "
                                  "[normal] convert multi file url. "
                                  "[chm] convert chm html file. "
                                  "[word] convert word single file. "
                                  "[offline] convert offline html document."
                                  "[single] convert single file url. "
                                  "[website] convert sequoiadb.com url. " ) ;

   opt.setOptions( "", "", "Convert config:" ) ;
   opt.setOptions( "t", "true", "Support tab page parsing, default: true" ) ;
   opt.setOptions( "n", "false", "Auto generate intra-page navigation, default: true" ) ;

   opt.setOptions( "", "", "Config mode:" ) ;
   opt.setOptions( "c", "doc/config/toc.json", "The JSON file path, default: doc/config/toc.json" ) ;

   opt.setOptions( "", "", "Iteration mode:" ) ;
   opt.setOptions( "p", "src/document", "Converted folder path, default doc/src/document" ) ;

   opt.setOptions( "", "", "Single mode:" ) ;
   opt.setOptions( "f", "", "The specified the conversion of a file." ) ;
   opt.setOptions( "o", "", "Output html path, default doc/build/mid/xxx.html." ) ;

   rc = opt.parse( argc, argv ) ;
   if( rc )
   {
      opt.printHelp() ;
      goto error ;
   }
   rc = opt.getOption( "h", value, &isDefault ) ;
   if( rc == SDB_OK && isDefault == FALSE )
   {
      rc = SDB_SDB_HELP_ONLY ;
      goto done ;
   }
   rc = SDB_OK ;
done:
   return rc ;
error:
   goto done ;
}

INT32 parseConf( string configPath )
{
   INT32 rc = SDB_OK ;
   iniReader reader ;

   rc = reader.init( configPath.c_str() ) ;
   if( rc )
   {
      goto error ;
   }

   rc = reader.get( "root", _rootPath ) ;
   if( rc )
   {
      goto error ;
   }

   rc = reader.get( "md", _mdPath ) ;
   if( rc )
   {
      goto error ;
   }

   rc = reader.get( "img", _imgPath ) ;
   if( rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 outputHtmlHeader()
{
   INT32 rc = SDB_OK ;
   string cssContent ;
   string html ;
   string outputPath = _htmlPath + "/build.html" ;
   if( _cssFile.size() > 0 )
   {
      rc = file_get_contents( _cssFile, cssContent ) ;
      if( rc )
      {
         cout << "Failed to get file content, path: " << _cssFile << endl ;
         goto error ;
      }

      if( _jsFile.size() > 0 )
      {
         string jsContent ;

         rc = file_get_contents( _jsFile, jsContent ) ;
         if( rc )
         {
            cout << "Failed to get file content, path: " << _jsFile << endl ;
            goto error ;
         }

         html = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\"><style type=\"text/css\">" + cssContent + "</style><script type=\"text/javascript\">" + jsContent + "</script></head><body>" ;
      }
      else
      {
         html = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\"><style type=\"text/css\">" + cssContent + "</style></head><body>" ;
      }
   }
   else
   {
      if( _jsFile.size() > 0 )
      {
         string jsContent ;

         rc = file_get_contents( _jsFile, jsContent ) ;
         if( rc )
         {
            cout << "Failed to get file content, path: " << _jsFile << endl ;
            goto error ;
         }

         html = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\"><script type=\"text/javascript\">" + jsContent + "</script></head><body>" ;
      }
      else
      {
         html = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\"></head><body>" ;
      }
   }
   rc = file_put_contents( outputPath, html ) ;
   if( rc )
   {
      cout << "Failed to put file, path: " << outputPath << endl ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 outputHtmlFooter()
{
   INT32 rc = SDB_OK ;
   string html = "</body></html>" ;
   string outputPath = _htmlPath + "/build.html" ;
   rc = file_put_contents( outputPath, html, TRUE ) ;
   if( rc )
   {
      cout << "Failed to put file, path: " << outputPath << endl ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 parseMarkdown( string md, string &html, string path, INT32 level, string title )
{
   INT32 rc = SDB_OK ;
   INT32 edition = atoi( _major.c_str() ) * 100 + atoi( _minor.c_str() ) ;
   mdParser parser ;
   string version = _major + "." + _minor ;

   rc = parser.init( level, _rootPath, _mdPath, _imgPath, edition, path,
                     _fullPath, _convertMode, _tabPage, _navPage,
                     &_fileMap, &_cnMap ) ;
   if( rc )
   {
      goto error ;
   }

   if( md.size() > 0 )
   {
      cout << "Convert file: " << path << endl ;
      rc = parser.parse( md, version, html ) ;
      if( rc )
      {
         goto error ;
      }
      cout << endl << endl ;
   }

   if( _fullHtml && ( ( _convertMode != "single" && _convertMode != "word" ) || _mode == "file" ) )
   {
      string cssContent ;
      string jsContent ;
      string tmp ;

      if( _cssFile.size() > 0 )
      {
         rc = file_get_contents( _cssFile, cssContent ) ;
         if( rc )
         {
            cout << "Failed to get file content, path: " << _cssFile << endl ;
            goto error ;
         }
      }
      
      if( _jsFile.size() > 0 )
      {
         rc = file_get_contents( _jsFile, jsContent ) ;
         if( rc )
         {
            cout << "Failed to get file content, path: " << _jsFile << endl ;
            goto error ;
         }
      }

      tmp = "<html><head>" ;

      if( _convertMode == "chm" || _convertMode == "offline" )
      {
         tmp += "<title>" + title + "</title>" ;
      }

      tmp += "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\">" ;

      if( cssContent.size() > 0 )
      {
         tmp += "<style type=\"text/css\">" + cssContent + "</style>" ;
      }

      if( jsContent.size() > 0 )
      {
         tmp += "<script type=\"text/javascript\">" + jsContent + "</script>" ;
      }

      html = tmp + "</head><body>" + html + " </body></html>" ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 buildFileMap( CJSON_MACHINE *pMachine,
                    const cJson_iterator *pIter,
                    BOOLEAN isFirst,
                    BOOLEAN isArray,
                    string subPath,
                    string cnPath,
                    INT32 level )
{
   INT32 rc = SDB_OK ;
   CJSON_VALUE_TYPE cJsonType = CJSON_NONE ;
   const cJson_iterator *pIterSub = NULL ;

   if( isFirst )
   {
      if( !cJsonIteratorMore( pIter ) )
      {
         rc = SDB_SYS ;
         cout << "contents field not found." << endl ;
         goto error ;
      }
      string key = cJsonIteratorKey( pIter ) ;
      if( key != "contents" )
      {
         rc = SDB_SYS ;
         cout << "contents field not found." << endl ;
         goto error ;
      }
      cJsonType = cJsonIteratorType( pIter ) ;
      if( cJsonType != CJSON_ARRAY )
      {
         rc = SDB_SYS ;
         cout << "contents field type must be array." << endl ;
         goto error ;
      }
      pIterSub = cJsonIteratorSub( pIter ) ;
      if( pIterSub == NULL )
      {
         rc = SDB_SYS ;
         cout << "Failed to get '" << key << "' sub iterator" << endl ;
         goto error ;
      }
      rc = buildFileMap( pMachine, pIterSub, FALSE, TRUE, subPath, cnPath, 1 ) ;
      if( rc )
      {
         goto error ;
      }
   }
   else if( isArray == TRUE )
   {
      while( cJsonIteratorMore( pIter ) )
      {
         cJsonType = cJsonIteratorType( pIter ) ;
         if( cJsonType == CJSON_NONE )
         {
            break ;
         }
         if( cJsonType != CJSON_OBJECT )
         {
            rc = SDB_SYS ;
            cout << "type must be object." << endl ;
            cout << cJsonType << endl ;
            goto error ;
         }
         pIterSub = cJsonIteratorSub( pIter ) ;
         if( pIterSub == NULL )
         {
            rc = SDB_SYS ;
            cout << "Failed to get array sub iterator" << endl ;
            goto error ;
         }
         rc = buildFileMap( pMachine, pIterSub, FALSE, FALSE, subPath, cnPath, level ) ;
         if( rc )
         {
            goto error ;
         }
         cJsonIteratorNext( pIter ) ;
      }
   }
   else
   {
      INT32 id = -1 ;
      BOOLEAN disable = FALSE ;
      string cnTitle ;
      string enTitle ;
      string file ;
      string dir ;

      while( cJsonIteratorMore( pIter ) )
      {
         cJsonType = cJsonIteratorType( pIter ) ;
         if( cJsonType == CJSON_NONE )
         {
            goto done ;
         }
         string key = cJsonIteratorKey( pIter ) ;
         if( key == "id" )
         {
            if( cJsonType != CJSON_INT32 )
            {
               rc = SDB_SYS ;
               cout << "id field type must be int." << endl ;
               goto error ;
            }
            id = cJsonIteratorInt32( pIter ) ;
         }
         else if( key == "cn" )
         {
            if( cJsonType != CJSON_STRING )
            {
               rc = SDB_SYS ;
               cout << "cn field type must be string." << endl ;
               goto error ;
            }
            cnTitle = cJsonIteratorString( pIter ) ;
         }
         else if( key == "en" )
         {
            if( cJsonType != CJSON_STRING )
            {
               rc = SDB_SYS ;
               cout << "en field type must be int." << endl ;
               goto error ;
            }
            enTitle = cJsonIteratorString( pIter ) ;
         }
         else if( key == "file" )
         {
            if( cJsonType != CJSON_STRING )
            {
               rc = SDB_SYS ;
               cout << "file field type must be string." << endl ;
               goto error ;
            }
            file = cJsonIteratorString( pIter ) ;
         }
         else if( key == "dir" )
         {
            if( cJsonType != CJSON_STRING )
            {
               rc = SDB_SYS ;
               cout << "dir field type must be string." << endl ;
               goto error ;
            }
            dir = cJsonIteratorString( pIter ) ;
         }
         else if( key == "contents" )
         {
            if( cJsonType != CJSON_ARRAY )
            {
               rc = SDB_SYS ;
               cout << "contents field type must be array." << endl ;
               goto error ;
            }
            pIterSub = cJsonIteratorSub( pIter ) ;
            if( pIterSub == NULL )
            {
               rc = SDB_SYS ;
               cout << "Failed to get '" << key << "' sub iterator" << endl ;
               goto error ;
            }
         }
         else if( key == "disable" )
         {
            if( cJsonType != CJSON_TRUE && cJsonType != CJSON_FALSE )
            {
               rc = SDB_SYS ;
               cout << "disable field type must be bool." << endl ;
               goto error ;
            }
            disable = cJsonIteratorBoolean( pIter ) ;
         }
         else if ( key == "top" )
         {
         }
         else
         {
            cout << "Warning: " << key << " field not defined, id: " << id << endl ;
         }
         cJsonIteratorNext( pIter ) ;
      }
      if( id < 0 )
      {
         rc = SDB_SYS ;
         cout << "id field must be defined." << endl ;
         goto error ;
      }
      if( cnTitle.size() <= 0 )
      {
         rc = SDB_SYS ;
         cout << "cn field must be defined." << endl ;
         goto error ;
      }
      if( enTitle.size() <= 0 )
      {
         rc = SDB_SYS ;
         cout << "en field must be defined." << endl ;
         goto error ;
      }

      if( disable )
      {
         cout << "Warning: disable item, id: " << id << ", cn: " << cnTitle << endl ;
         goto done ;
      }

      if( file.size() > 0 )
      {
         if( dir.size() > 0 )
         {
            rc = SDB_SYS ;
            cout << "file field can't be with dir field, id="<< id << endl ;
            goto error ;
         }
         if( pIterSub != NULL )
         {
            rc = SDB_SYS ;
            cout << "file field can't be with contents field." << endl ;
            goto error ;
         }
         //cout << "test: " << subPath << "  " << cnPath << endl ;
         _fileMap.insert( map<string, INT32>::value_type( subPath + file, id ) ) ;
         _cnMap.insert( map<string, string>::value_type( subPath, cnPath ) ) ;
      }
      else
      {
         if( dir.size() <= 0 )
         {
            rc = SDB_SYS ;
            cout << "dir field must be defined." << endl ;
            goto error ;
         }
         if( pIterSub == NULL )
         {
            rc = SDB_SYS ;
            cout << "contents field must be defined. id = " << id << endl ;
            goto error ;
         }

         subPath += dir ;

         _fileMap.insert( map<string, INT32>::value_type( subPath, id ) ) ;

         subPath += "." ;

         _cnMap.insert( map<string, string>::value_type( subPath, "Readme" ) ) ;

         cnPath += cnTitle + "/" ;
         rc = buildFileMap( pMachine, pIterSub, FALSE, TRUE, subPath, cnPath, level + 1 ) ;
         if( rc )
         {
            cout << "Failed to call execConfFiles, path: " << subPath << endl ;
            goto error ;
         }
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 execConfFiles( CJSON_MACHINE *pMachine,
                     const cJson_iterator *pIter,
                     BOOLEAN isFirst,
                     BOOLEAN &isFirstMd,
                     BOOLEAN isArray,
                     string subPath,
                     string cnPath,
                     INT32 level )
{
   INT32 rc = SDB_OK ;
   CJSON_VALUE_TYPE cJsonType = CJSON_NONE ;
   const cJson_iterator *pIterSub = NULL ;

   if( isFirst )
   {
      if( !cJsonIteratorMore( pIter ) )
      {
         rc = SDB_SYS ;
         cout << "contents field not found." << endl ;
         goto error ;
      }
      string key = cJsonIteratorKey( pIter ) ;
      if( key != "contents" )
      {
         rc = SDB_SYS ;
         cout << "contents field not found." << endl ;
         goto error ;
      }
      cJsonType = cJsonIteratorType( pIter ) ;
      if( cJsonType != CJSON_ARRAY )
      {
         rc = SDB_SYS ;
         cout << "contents field type must be array." << endl ;
         goto error ;
      }
      pIterSub = cJsonIteratorSub( pIter ) ;
      if( pIterSub == NULL )
      {
         rc = SDB_SYS ;
         cout << "Failed to get '" << key << "' sub iterator" << endl ;
         goto error ;
      }
      rc = execConfFiles( pMachine, pIterSub, FALSE, isFirstMd, TRUE, subPath, cnPath, 1 ) ;
      if( rc )
      {
         goto error ;
      }
   }
   else if( isArray == TRUE )
   {
      while( cJsonIteratorMore( pIter ) )
      {
         cJsonType = cJsonIteratorType( pIter ) ;
         if( cJsonType == CJSON_NONE )
         {
            break ;
         }
         if( cJsonType != CJSON_OBJECT )
         {
            rc = SDB_SYS ;
            cout << "type must be object." << endl ;
            cout << cJsonType << endl ;
            goto error ;
         }
         pIterSub = cJsonIteratorSub( pIter ) ;
         if( pIterSub == NULL )
         {
            rc = SDB_SYS ;
            cout << "Failed to get array sub iterator" << endl ;
            goto error ;
         }
         rc = execConfFiles( pMachine, pIterSub, FALSE, isFirstMd, FALSE, subPath, cnPath, level ) ;
         if( rc )
         {
            goto error ;
         }
         cJsonIteratorNext( pIter ) ;
      }
   }
   else
   {
      INT32 id = -1 ;
      BOOLEAN disable = FALSE ;
      string cnTitle ;
      string enTitle ;
      string file ;
      string dir ;

      while( cJsonIteratorMore( pIter ) )
      {
         cJsonType = cJsonIteratorType( pIter ) ;
         if( cJsonType == CJSON_NONE )
         {
            goto done ;
         }
         string key = cJsonIteratorKey( pIter ) ;
         if( key == "id" )
         {
            if( cJsonType != CJSON_INT32 )
            {
               rc = SDB_SYS ;
               cout << "id field type must be int." << endl ;
               goto error ;
            }
            id = cJsonIteratorInt32( pIter ) ;
         }
         else if( key == "cn" )
         {
            if( cJsonType != CJSON_STRING )
            {
               rc = SDB_SYS ;
               cout << "cn field type must be string." << endl ;
               goto error ;
            }
            cnTitle = cJsonIteratorString( pIter ) ;
         }
         else if( key == "en" )
         {
            if( cJsonType != CJSON_STRING )
            {
               rc = SDB_SYS ;
               cout << "en field type must be int." << endl ;
               goto error ;
            }
            enTitle = cJsonIteratorString( pIter ) ;
         }
         else if( key == "file" )
         {
            if( cJsonType != CJSON_STRING )
            {
               rc = SDB_SYS ;
               cout << "file field type must be string." << endl ;
               goto error ;
            }
            file = cJsonIteratorString( pIter ) ;
         }
         else if( key == "dir" )
         {
            if( cJsonType != CJSON_STRING )
            {
               rc = SDB_SYS ;
               cout << "dir field type must be string." << endl ;
               goto error ;
            }
            dir = cJsonIteratorString( pIter ) ;
         }
         else if( key == "contents" )
         {
            if( cJsonType != CJSON_ARRAY )
            {
               rc = SDB_SYS ;
               cout << "contents field type must be array." << endl ;
               goto error ;
            }
            pIterSub = cJsonIteratorSub( pIter ) ;
            if( pIterSub == NULL )
            {
               rc = SDB_SYS ;
               cout << "Failed to get '" << key << "' sub iterator" << endl ;
               goto error ;
            }
         }
         else if( key == "disable" )
         {
            if( cJsonType != CJSON_TRUE && cJsonType != CJSON_FALSE )
            {
               rc = SDB_SYS ;
               cout << "disable field type must be bool." << endl ;
               goto error ;
            }
            disable = cJsonIteratorBoolean( pIter ) ;
         }
         else if( key == "top" )
         {
         }
         else
         {
            cout << "Warning: " << key << " field not defined, id: " << id << endl ;
         }
         cJsonIteratorNext( pIter ) ;
      }
      if( id < 0 )
      {
         rc = SDB_SYS ;
         cout << "id field must be defined." << endl ;
         goto error ;
      }
      if( cnTitle.size() <= 0 )
      {
         rc = SDB_SYS ;
         cout << "cn field must be defined." << endl ;
         goto error ;
      }
      if( enTitle.size() <= 0 )
      {
         rc = SDB_SYS ;
         cout << "en field must be defined." << endl ;
         goto error ;
      }

      if( disable )
      {
         cout << "Warning: disable item, id: " << id << ", cn: " << cnTitle << endl ;
         goto done ;
      }

      if( file.size() > 0 )
      {
         string filePath ;
         string outputPath ;
         string mdContent ;
         string html ;
         string extName ;
         if( dir.size() > 0 )
         {
            rc = SDB_SYS ;
            cout << "file field can't be with dir field, id=" << id << endl ;
            goto error ;
         }
         if( pIterSub != NULL )
         {
            rc = SDB_SYS ;
            cout << "file field can't be with contents field." << endl ;
            goto error ;
         }

         if( _language == "cn" )
         {
            filePath = _rootPath + _mdPath + "/" + subPath + file + ".md" ;
         }
         else
         {
            filePath = _rootPath + _mdPath + "/" + subPath + file + "_" + _language + ".md" ;
         }
         rc = file_get_contents( filePath, mdContent ) ;
         if( rc )
         {
            cout << "Failed to get file content, path: " << filePath << endl ;
            goto error ;
         }
         //cout << "file " << filePath << endl ;
         rc = parseMarkdown( mdContent, html, subPath + file + ".md", level, cnTitle ) ;
         if( rc )
         {
            cout << "Failed to convert markdown file, path: " << filePath << endl ;
            goto error ;
         }
         if( _convertMode == "single" || _convertMode == "word" )
         {
            outputPath = _htmlPath + "/build.html" ;
            string htmlTitle ;
            rc = convertHtmlHeader( level, subPath + file + ".md", cnTitle, htmlTitle ) ;
            if( rc )
            {
               cout << "Failed to convert html header" << endl ;
               goto error ;
            }
            if( isFirstMd )
            {
               isFirstMd = FALSE ;
            }
            else if( level == 1 )
            {
               string pageBreak = "<p class=\"page_break\"></p>" ;
               rc = file_put_contents( outputPath, pageBreak, TRUE ) ;
               if( rc )
               {
                  cout << "Failed to put file, path: " << outputPath << endl ;
                  goto error ;
               }
            }
            rc = file_put_contents( outputPath, htmlTitle, TRUE ) ;
            if( rc )
            {
               cout << "Failed to put file, path: " << outputPath << endl ;
               goto error ;
            }
         }
         else
         {
            /*
            if( _convertMode == "chm" )
            {
               outputPath = _htmlPath + "/" + cnPath + file + ".html" ;
            }
            else
            {
               outputPath = _htmlPath + "/" + subPath + file + ".html" ;
            }*/
            outputPath = _htmlPath + "/" + subPath + file + ".html" ;
         }
         rc = file_put_contents( outputPath, html, ( _convertMode == "single" || _convertMode == "word" ) ) ;
         if( rc )
         {
            cout << "Failed to put file, path: " << outputPath << endl ;
            goto error ;
         }
      }
      else
      {
         string dirPath ;
         string readmeHtml ;

         if( dir.size() <= 0 )
         {
            rc = SDB_SYS ;
            cout << "dir field must be defined." << endl ;
            goto error ;
         }
         if( pIterSub == NULL )
         {
            rc = SDB_SYS ;
            cout << "contents field must be defined. id = " << id << endl ;
            goto error ;
         }
         cnPath += cnTitle + "/" ;
         subPath += dir + "/" ;

         if( _convertMode != "single" && _convertMode != "word" )
         {
            /*
            if( _convertMode == "chm" )
            {
               dirPath = _htmlPath + "/" + cnPath ;
            }
            else
            {
               dirPath = _htmlPath + "/" + subPath ;
            }*/
            dirPath = _htmlPath + "/" + subPath ;
            rc = mkdir( dirPath ) ;
            if( rc )
            {
               cout << "4 Failed to create dir, path: " << dirPath << endl ;
               goto error ;
            }
         }

         {
            //readme专用
            string filePath ;

            if( _language == "cn" )
            {
               filePath = _rootPath + _mdPath + "/" + subPath + "Readme.md" ;
            }
            else
            {
               filePath = _rootPath + _mdPath + "/" + subPath + "Readme_" + _language + ".md" ;
            }

            rc = fileIsExist( filePath ) ;
            if( rc == SDB_OK )
            {
               string mdContent ;

               rc = file_get_contents( filePath, mdContent ) ;
               if( rc )
               {
                  cout << "Failed to get readme content, path: " << filePath << endl ;
                  goto error ;
               }

               rc = parseMarkdown( mdContent, readmeHtml, subPath + "Readme.md", level, "" ) ;
               if( rc )
               {
                  cout << "Failed to convert readme file, path: " << filePath << endl ;
                  goto error ;
               }
            }
         }

         if( _convertMode != "single" && _convertMode != "word" )
         {

            if( readmeHtml.length() > 0 )
            {
               string outputPath = _htmlPath + "/" + subPath + "Readme.html" ;

               rc = file_put_contents( outputPath, readmeHtml, FALSE ) ;
               if( rc )
               {
                  cout << "Failed to put readme, path: " << outputPath << endl ;
                  goto error ;
               }
            }
         }
         else
         {
            string outputPath = _htmlPath + "/build.html" ;
            string htmlTitle ;
            rc = convertHtmlHeader( level, "", cnTitle, htmlTitle ) ;
            if( rc )
            {
               cout << "Failed to convert html header" << endl ;
               goto error ;
            }
            if( isFirstMd )
            {
               isFirstMd = FALSE ;
            }
            else if( level == 1 )
            {
               string pageBreak = "<p class=\"page_break\"></p>" ;
               rc = file_put_contents( outputPath, pageBreak, TRUE ) ;
               if( rc )
               {
                  cout << "Failed to put file, path: " << outputPath << endl ;
                  goto error ;
               }
            }
            rc = file_put_contents( outputPath, htmlTitle, TRUE ) ;
            if( rc )
            {
               cout << "Failed to put file, path: " << outputPath << endl ;
               goto error ;
            }

            if( readmeHtml.length() > 0 )
            {
               rc = file_put_contents( outputPath, readmeHtml, TRUE ) ;
               if( rc )
               {
                  cout << "Failed to put file, path: " << outputPath << endl ;
                  goto error ;
               }
            }
         }

         rc = execConfFiles( pMachine, pIterSub, FALSE, isFirstMd, TRUE, subPath, cnPath, level + 1 ) ;
         if( rc )
         {
            cout << "Failed to call execConfFiles, path: " << subPath << endl ;
            goto error ;
         }
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 execIterFiles( string path, string subPath )
{
   INT32 rc = SDB_OK ;
   vector<FileStruct> fileList ;
   vector<FileStruct>::iterator iter ;

   rc = getFiles( path, fileList ) ;
   if( rc )
   {
      goto error ;
   }

   for( iter = fileList.begin(); iter != fileList.end(); ++iter )
   {
      if( (*iter).isDir == TRUE )
      {
         string newPath = path + "/" + (*iter).fileName ;
         string dirPath = subPath + "/" + (*iter).fileName ;
         rc = mkdir( dirPath ) ;
         if( rc )
         {
            cout << "Failed to create dir, path: " << dirPath << endl ;
            goto error ;
         }
         rc = execIterFiles( newPath, dirPath ) ;
         if( rc )
         {
            cout << "Failed to call execIterFiles, path: " << newPath << endl ;
            goto error ;
         }
      }
      else
      {
         string fileName ;
         string filePath = path + "/" + (*iter).fileName ;
         string outputPath ;
         string mdContent ;
         string html ;
         string extName ;

         getFileExtName( (*iter).fileName, extName ) ;
         if( extName == "md" )
         {
            rc = file_get_contents( filePath, mdContent ) ;
            if( rc )
            {
               cout << "Failed to get file content, path: " << filePath << endl ;
               goto error ;
            }
            if( mdContent.size() > 0 )
            {
               rc = parseMarkdown( mdContent, html, subPath + "/" + (*iter).fileName, 1 ) ;
               if( rc )
               {
                  cout << "Failed to convert markdown file, path: " << filePath << endl ;
                  goto error ;
               }
            }
            if( _convertMode == "single" && _convertMode == "word" )
            {
               outputPath = _htmlPath + "/build.html" ;
            }
            else
            {
               getFileName( filePath, fileName ) ;
               outputPath = subPath + "/" + fileName + ".html" ;
            }
            rc = file_put_contents( outputPath, html, ( _convertMode == "single" || _convertMode == "word" ) ) ;
            if( rc )
            {
               cout << "Failed to put file, path: " << outputPath << endl ;
               goto error ;
            }
         }
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 parseJson( const CHAR *pJson, CJSON_MACHINE *pMachine )
{
   INT32 rc = SDB_OK ;

   cJsonInit( pMachine, CJSON_RIGOROUS_PARSE, FALSE ) ;
   if( cJsonParse( pJson, pMachine ) == FALSE )
   {
      rc = SDB_UTIL_PARSE_JSON_INVALID ;
      cout <<  "Failed to parse JSON" << endl ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}