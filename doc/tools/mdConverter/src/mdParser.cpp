#include <stdio.h>
#include <iostream>
#include "mdParser.hpp"
#include "system.hpp"
#include "sundown/houdini.h"
#include "ossMem.h"

static CHAR *_pRootPath ;
static CHAR *_pMarkdownPath ;
static CHAR *_pImagePath ;
static map<string, INT32> *_pFileMap ;
static map<string, string> *_pCnMap ;
static BOOLEAN _tabPage ;
static BOOLEAN _navPage ;

static BOOLEAN _isFullPath ;

static string _convertMode ;
static string _filePath ;

static INT32 _edition ;

static INT32 _level ;

static INT32 _titleNum ;

static struct buf *_pNavOb ;

static void ReplaceAll( string &source, string oldStr, string newStr ) ;

static INT32 parseLink( struct buf *ob,
                        const struct buf *link,
                        const struct buf *title,
                        const struct buf *content,
                        void *opaque ) ;

static INT32 parseImage( struct buf *ob,
                         const struct buf *link,
                         const struct buf *title,
                         const struct buf *alt,
                         void *opaque ) ;

static void parseCode( struct buf *ob,
                       const struct buf *text,
                       const struct buf *lang,
                       void *opaque ) ;

static void parseHeader( struct buf *ob,
                         const struct buf *text,
                         int level,
                         void *opaque ) ;

mdParser::mdParser()
{
   _pMarkdown = NULL ;
   _pNavOb = NULL ;
}

mdParser::~mdParser()
{
   if( _pMarkdown )
   {
      sd_markdown_free( _pMarkdown ) ;
      _pMarkdown = NULL ;
   }
   if( _pRootPath )
   {
      free( _pRootPath ) ;
      _pRootPath = NULL ;
   }
   if( _pMarkdownPath )
   {
      free( _pMarkdownPath ) ;
      _pMarkdownPath = NULL ;
   }
   if( _pImagePath )
   {
      free( _pImagePath ) ;
      _pImagePath = NULL ;
   }
}

INT32 mdParser::init( INT32 level,
                      string rootPath,
                      string mdPath,
                      string imgPath,
                      INT32 edition,
                      string filePath,
                      BOOLEAN isFullPath,
                      string convertMode,
                      BOOLEAN tabPage,
                      BOOLEAN navPage,
                      map<string, INT32> *pFileMap,
                      map<string, string> *pCnMap )
{
   INT32 rc = SDB_OK ;
   string right( "\\" ) ;
   string left( "/" ) ;

   _level = level ;

   _titleNum = 0 ;

   ReplaceAll( rootPath, right, left ) ;
   ReplaceAll( mdPath, right, left ) ;
   ReplaceAll( imgPath, right, left ) ;

   _pRootPath     = strdup( rootPath.c_str() ) ;
   _pMarkdownPath = strdup( mdPath.c_str() ) ;
   _pImagePath    = strdup( imgPath.c_str() ) ;
   _isFullPath    = isFullPath ;
   _convertMode   = convertMode ;
   _tabPage       = tabPage ;
   _navPage       = navPage ;
   _pFileMap      = pFileMap ;
   _pCnMap        = pCnMap ;
   _edition       = edition ;
   _filePath      = filePath ;

   if( _pRootPath == NULL || _pMarkdownPath == NULL || _pImagePath == NULL )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   sdhtml_renderer( &_callbacks, &_options, HTML_TOC ) ;

   _callbacks.link  = parseLink ;
   _callbacks.image = parseImage ;
   _callbacks.blockcode = parseCode ;
   _callbacks.header = parseHeader ;

   _pMarkdown = sd_markdown_new( MKDEXT_NO_INTRA_EMPHASIS|
                                 MKDEXT_TABLES|
                                 MKDEXT_FENCED_CODE|
                                 MKDEXT_STRIKETHROUGH|
                                 MKDEXT_SUPERSCRIPT|
                                 MKDEXT_LAX_SPACING,
                                 16,
                                 &_callbacks,
                                 &_options ) ;
   if( _pMarkdown == NULL )
   {
      rc = SDB_OOM ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 mdParser::parse( string mdContent, string version, string &htmlContent )
{
   INT32 rc = SDB_OK ;
   struct buf *pOb = NULL ;

   if( _pMarkdown == NULL )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   ReplaceAll( mdContent, "{version}", version ) ;

   pOb = bufnew( mdContent.size() * 1024 ) ;
   if( pOb == NULL )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   _pNavOb = bufnew( 1024 ) ;
   if( _pNavOb == NULL )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   BUFPUTSL( _pNavOb, "<div style=\"border:1px solid #EEE;padding:10px;\"><span style=\"font-size:16px;font-weight:bold;\">本页导航</span><br/><ul>\n" );

   sd_markdown_render( pOb, (const UINT8 *)(mdContent.c_str()), mdContent.size(), _pMarkdown, _tabPage ) ;

   BUFPUTSL( _pNavOb, "</ul></div>\n" );

   htmlContent.insert( 0, (const CHAR *)pOb->data, 0, pOb->size ) ;

   if ( _navPage )
   {
      string nav( (const char*)_pNavOb->data, _pNavOb->size ) ;

      ReplaceAll( htmlContent, "<!--Nav Position-->", nav ) ;
   }
   else
   {
      ReplaceAll( htmlContent, "<!--Nav Position-->", "" ) ;
   }

done:

   if ( _pNavOb )
   {
      bufrelease( _pNavOb ) ;
      _pNavOb = NULL ;
   }

   if ( pOb )
   {
      bufrelease( pOb ) ;
   }

   return rc ;
error:
   goto done ;
}

INT32 convertHtmlHeader( INT32 level,
                         string anchor,
                         string contents,
                         string &htmlCode )
{
   INT32 rc = SDB_OK ;
   struct buf *ob = NULL ;

   ob = bufnew( contents.size() * 1024 ) ;
   if( ob == NULL )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   
   if( anchor.size() > 0 )
   {
      if( _convertMode == "single" )
      {
         ReplaceAll( anchor, ".md", "_html" ) ;
         ReplaceAll( anchor, "/", "_" ) ;
         ReplaceAll( anchor, "\\", "_" ) ;
         ReplaceAll( anchor, "#", "_" ) ;
         bufprintf( ob, "<h%d id=\"", level ) ;
         houdini_escape_href( ob, (const uint8_t*)anchor.c_str(), anchor.size() ) ;
         bufprintf( ob, "\" name=\"" ) ;
         houdini_escape_href( ob, (const uint8_t*)anchor.c_str(), anchor.size() ) ;
         bufprintf( ob, "\">" ) ;
      }
      else if( _convertMode == "word" )
      {
         string old = anchor ;
         ReplaceAll( anchor, ".md", "" ) ;
         ReplaceAll( anchor, "/", "." ) ;
         ReplaceAll( anchor, "\\", "." ) ;
         map<string, INT32>::iterator iter = _pFileMap->find( anchor ) ;
         if( iter == _pFileMap->end() )
         {
            cout << "Warning Header Not find: " << old << endl ;
            rc = SDB_IO ;
            goto error ;
         }
         else
         {
            CHAR catIdStr[30] ;
            sprintf( catIdStr, "%d", iter->second ) ;
            anchor = catIdStr ;
            bufprintf( ob, "<h%d><a id=\"", level ) ;
            houdini_escape_href(ob, (const uint8_t *)anchor.c_str(), anchor.size() );
            bufprintf( ob, "\" name=\"" ) ;
            houdini_escape_href(ob, (const uint8_t *)anchor.c_str(), anchor.size() );
            bufprintf( ob, "\">" ) ;
         }
      }
   }
   else
   {
      bufprintf( ob, "<h%d>", level ) ;
   }
   if( contents.size() > 0 )
   {
      bufput( ob, (const uint8_t*)contents.c_str(), contents.size() ) ;
   }
   if( _convertMode == "word" )
   {
      bufprintf( ob, "</a>" ) ;
   }
   bufprintf( ob, "</h%d>\n", level ) ;
   
   htmlCode.insert( 0, (const CHAR *)ob->data, 0, ob->size ) ;

   bufrelease( ob ) ;

done:
   return rc ;
error:
   goto done ;
}


static INT32 parseLink( struct buf *ob,
                        const struct buf *link,
                        const struct buf *title,
                        const struct buf *content,
                        void *opaque )
{
   struct html_renderopt *options = (struct html_renderopt *)opaque ;
   if( link != NULL &&
       ( options->flags & HTML_SAFELINK ) != 0 &&
       !sd_autolink_issafe( link->data, link->size ) )
   {
      return 0;
   }

   BUFPUTSL(ob, "<a href=\"");

   if( link && link->size )
   {
      string srcLink( (const char *)link->data, link->size ) ;
      string aLink( (const char *)link->data, link->size ) ;

      if( aLink.find( "manual/" ) == 0 )
      {
         aLink = aLink.substr( 7 ) ;
      }

      if( link->size >= 4 &&
          ( link->data[0] != 'h' ||
            link->data[1] != 't' ||
            link->data[2] != 't' ||
            link->data[3] != 'p' ) )
      {
         if( link->data[0] == 'a' &&
             link->data[1] == 'p' &&
             link->data[2] == 'i' &&
             link->data[3] == '/' )
         {
            if( _convertMode == "chm" || _convertMode == "offline" )
            {
               houdini_escape_href(ob, (const uint8_t *)aLink.c_str(), aLink.size() );
            }
            else
            {
               CHAR editionIdStr[30] ;
               if( _convertMode == "website" )
               {
                  BUFPUTSL(ob, "__DOCUMENT__/");
               }
               else
               {
                  BUFPUTSL(ob, "http://doc.sequoiadb.com/cn/index/Public/Home/document/");
               }
               sprintf( editionIdStr, "%d", _edition ) ;
               bufput(ob, editionIdStr, strlen(editionIdStr)) ;
               BUFPUTSL(ob, "/");
               houdini_escape_href(ob, (const uint8_t *)aLink.c_str(), aLink.size() );
            }
         }
         else
         {
            if( _convertMode == "normal" )
            {
               string linkPath ;
               string fullPath = _pRootPath ;

               fullPath += _pMarkdownPath ;
               if( aLink.find( "#" ) != aLink.npos )
               {
                  fullPath += aLink.substr( 0, aLink.find( "#" ) ) ;
               }
               else
               {
                  fullPath += aLink ;
               }

               if( fileIsExist( fullPath ) )
               {
                  cout << "Warning unknow link: " << srcLink << endl ;
               }
               else
               {
                  ReplaceAll( aLink, ".md", ".html" ) ;
                  if( _isFullPath )
                  {
                     bufput(ob, _pRootPath, strlen( _pRootPath ) ) ;
                     BUFPUTSL(ob, "build/mid/");
                  }
                  houdini_escape_href(ob, (const uint8_t *)aLink.c_str(), aLink.size() );
               }
            }
            else if( _convertMode == "chm" )
            {
               {
                  string fullPath = _pRootPath ;

                  fullPath += _pMarkdownPath ;
                  if( aLink.find( "#" ) != aLink.npos )
                  {
                     fullPath += aLink.substr( 0, aLink.find( "#" ) ) ;
                  }
                  else
                  {
                     fullPath += aLink ;
                  }

                  if( fileIsExist( fullPath ) )
                  {
                     cout << "Warning unknow link: " << srcLink << endl ;
                  }
               }

               string destPath ;
               string filename ;

               getPath( aLink, destPath ) ;
               getFile( aLink, filename ) ;
               ReplaceAll( destPath, "/", "." ) ;
               ReplaceAll( destPath, "\\", "." ) ;
               ReplaceAll( filename, ".md", ".html" ) ;
               map<string, string>::iterator iter = _pCnMap->find( destPath ) ;
               if( iter == _pCnMap->end() && filename != "Readme.html" )
               {
                  cout << "Warning link dest path Not find: " << aLink << endl ;
               }
               else
               {
                  //destPath = iter->second ;
                  //destPath = destPath + filename ;
                  destPath = aLink ;
                  ReplaceAll( destPath, ".md", ".html" ) ;
                  bufput(ob, destPath.c_str(), destPath.size() );
               }
            }
            else if( _convertMode == "offline" )
            {
               string destPath ;
               string filename ;

               getPath( aLink, destPath ) ;
               getFile( aLink, filename ) ;
               ReplaceAll( destPath, "/", "." ) ;
               ReplaceAll( destPath, "\\", "." ) ;
               ReplaceAll( filename, ".md", ".html" ) ;
               map<string, string>::iterator iter = _pCnMap->find( destPath ) ;
               if( iter == _pCnMap->end() && filename != "Readme.html" )
               {
                  cout << "Warning link dest path Not find: " << aLink << endl ;
               }
               else
               {
                  //destPath = iter->second ;
                  //destPath = destPath + filename ;
                  destPath = aLink ;
                  ReplaceAll( destPath, ".md", ".html" ) ;
                  for( INT32 i = 1; i < _level; ++i )
                  {
                     destPath = "../" + destPath ;
                  }
                  bufput(ob, destPath.c_str(), destPath.size() );
               }
            }
            else if( _convertMode == "single" )
            {
               ReplaceAll( aLink, ".md", "_html" ) ;
               ReplaceAll( aLink, "/", "_" ) ;
               ReplaceAll( aLink, "#", "_" ) ;
               BUFPUTSL(ob, "#");
               houdini_escape_href(ob, (const uint8_t *)aLink.c_str(), aLink.size() );
            }
            else if( _convertMode == "word" )
            {
               if( aLink.find( ".html" ) != aLink.npos )
               {
                  houdini_escape_href( ob, link->data, link->size );
               }
               else
               {
                  string anchor ;
                  if( aLink.find( "#" ) != aLink.npos )
                  {
                     anchor = aLink.substr( aLink.find( "#" ) + 1 ) ;
                     aLink = aLink.substr( 0, aLink.find( "#" ) ) ;
                     //cout << "anchor " << anchor << endl ;
                  }
                  ReplaceAll( aLink, ".md", "" ) ;
                  ReplaceAll( aLink, "/", "." ) ;
                  map<string, INT32>::iterator iter = _pFileMap->find( aLink ) ;
                  if( iter == _pFileMap->end() )
                  {
                     cout << "Warning Link Not find: " << srcLink << endl ;
                  }
                  else
                  {
                     CHAR catIdStr[30] ;
                     sprintf( catIdStr, "%d", iter->second ) ;
                     //cout << "find: " << id << "   " << aLink << endl ;
                     aLink = "#" ;
                     aLink += catIdStr ;
                     if( anchor.size() > 0 )
                     {
                        //aLink = aLink + "_" + anchor ;
                     }
                     //cout << "link " << aLink << endl ;
                     houdini_escape_href(ob, (const uint8_t *)aLink.c_str(), aLink.size() );
                  }
               }
            }
            else if( _convertMode == "website" )
            {
               if( aLink.find( ".html" ) != aLink.npos )
               {
                  cout << "Warning unknow link: " << srcLink << endl ;
                  houdini_escape_href(ob, (const uint8_t *)aLink.c_str(), aLink.size() );
               }
               else
               {
                  string anchor ;
                  string filename ;

                  if( aLink.find( "#" ) != aLink.npos )
                  {
                     anchor = aLink.substr( aLink.find( "#" ) + 1 ) ;
                     aLink = aLink.substr( 0, aLink.find( "#" ) ) ;
                     //cout << "anchor " << anchor << endl ;
                  }

                  getFile( aLink, filename ) ;
                  if( filename == "Readme.md" )
                  {
                     string destPath ;

                     getPath( aLink, destPath ) ;

                     if( destPath[destPath.length()-1] == '/' )
                     {
                        destPath = destPath.substr( 0, destPath.length() - 1 ) ;
                     }

                     aLink = destPath ;
                  }

                  ReplaceAll( aLink, ".md", "" ) ;
                  ReplaceAll( aLink, "/", "." ) ;
                  map<string, INT32>::iterator iter = _pFileMap->find( aLink ) ;
                  if( iter == _pFileMap->end() )
                  {
                     cout << "Warning Link Not find: " << srcLink << endl ;
                  }
                  else
                  {
                     CHAR catIdStr[30] ;
                     CHAR editionIdStr[30] ;
                     sprintf( catIdStr, "%d", iter->second ) ;
                     sprintf( editionIdStr, "%d", _edition ) ;
                     //cout << "find: " << id << "   " << aLink << endl ;
                     aLink = "/cn/sequoiadb-cat_id-" ;
                     aLink += catIdStr ;
                     aLink += "-edition_id-" ;
                     aLink += editionIdStr ;
                     if( anchor.size() > 0 )
                     {
                        aLink = aLink + "#" + anchor ;
                     }
                     //cout << "link " << aLink << endl ;
                     houdini_escape_href(ob, (const uint8_t *)aLink.c_str(), aLink.size() );
                  }
               }
            }
         }
      }
      else
      {
         houdini_escape_href( ob, link->data, link->size );
      }
   }

   if( title && title->size )
   {
      BUFPUTSL( ob, "\" title=\"" ) ;
      houdini_escape_html0( ob, title->data, title->size, 0 ) ;
   }

   if( options->link_attributes )
   {
      bufputc( ob, '\"' ) ;
      options->link_attributes( ob, link, opaque ) ;
      bufputc( ob, '>' ) ;
   }
   else
   {
      BUFPUTSL( ob, "\">" ) ;
   }

   if( content && content->size )
   {
      bufput( ob, content->data, content->size );
   }
   BUFPUTSL( ob, "</a>" ) ;
   return 1 ;
}

static INT32 parseImage( struct buf *ob,
                         const struct buf *link,
                         const struct buf *title,
                         const struct buf *alt,
                         void *opaque )
{
   struct html_renderopt *options = (struct html_renderopt *)opaque ;
   if (!link || !link->size)
   {
      return 0 ;
   }

   BUFPUTSL( ob, "<div class=\"figure\" style=\"text-align:center;\"><img src=\"" ) ;

   if( link->size > 0 )
   {
      if( _convertMode == "chm" || _convertMode == "offline" )
      {
         string filename ;
         string imgData ;
         string destPath ;
         string imgPath ;
         string srcPath( (const char *)link->data, link->size ) ;
         string old( (const char *)link->data, link->size ) ;
         string imgLink( (const char *)link->data, link->size ) ;

         if( old.find( "images/" ) == 0 )
         {
            old = old.substr( 7 ) ;
         }

         imgPath = old ;

         if( imgLink.find( "/" ) != imgLink.npos )
         {
            imgLink = imgLink.substr( imgLink.rfind( "/" ) + 1,
                                      imgLink.size() - imgLink.rfind( "/" ) - 1 ) ;
         }

         houdini_escape_href( ob, (const uint8_t *)imgLink.c_str(), imgLink.size() ) ;
         imgPath = "/" + imgPath ;
         imgPath = _pImagePath + imgPath ;
         imgPath = _pRootPath + imgPath ;

         if( fileIsExist( imgPath ) != SDB_OK )
         {
            cout << "Warning image dest path Not find: " << srcPath << endl ;
         }

         if( !checkFilePath( imgPath, old ) )
         {
            cout << "Warning image document path and image file path are not the same: " << srcPath << endl ;
         }

         getPath( old, destPath ) ;
         getFile( old, filename ) ;
         ReplaceAll( destPath, "/", "." ) ;
         ReplaceAll( destPath, "\\", "." ) ;
         map<string, string>::iterator iter = _pCnMap->find( destPath ) ;
         if( iter == _pCnMap->end() )
         {
            cout << "Warning image and document path are not the same: " << srcPath << endl ;
         }
         else
         {
            /*
            destPath = iter->second ;
            destPath = "build/mid/" + destPath ;
            destPath = _pRootPath + destPath + filename ;
            */
            
            string imgDestPath = _pRootPath ;

            imgDestPath += "build/mid/" + old ;
            
            //cout << "source: " << imgPath << endl ;
            //cout << "dest  : " << destPath << endl << endl ;
            
            file_get_contents( imgPath, imgData ) ;
            file_put_contents( imgDestPath, imgData ) ;
         }
      }
      else
      {
         if( _isFullPath &&
            ( link->size < 4 || ( link->size >= 4 &&
                                  ( link->data[0] != 'h' ||
                                    link->data[1] != 't' ||
                                    link->data[2] != 't' ||
                                    link->data[3] != 'p' ) ) ) )
         {
            string destPath( (const char *)link->data, link->size ) ;
            string fullPath = _pRootPath ;

            if( destPath.find( "images/" ) == 0 )
            {
               destPath = destPath.substr( 7 ) ;
            }

            fullPath += _pImagePath ;
            fullPath += destPath ;
            
            if( fileIsExist( fullPath ) )
            {
               cout << "Warning unknow img : " << destPath << endl
                    << "full path : " << fullPath << endl ;
            }
            else
            {
               if( _convertMode == "single" )
               {
                  fullPath = "file:///" + fullPath ;
               }

               bufput( ob, fullPath.c_str(), fullPath.length() ) ;
            }
         }
         else
         {
            if( _convertMode == "website" )
            {
               CHAR editionIdStr[30] ;
               sprintf( editionIdStr, "%d", _edition ) ;
               string urlPath ;
               string aLink( (const char *)link->data, link->size ) ;

               if( aLink.find( "images/" ) == 0 )
               {
                  aLink = aLink.substr( 7 ) ;
               }

               urlPath = "__DOC_IMG__/" ;
               urlPath += editionIdStr ;
               urlPath += "/" ;
               aLink = urlPath + aLink ;
               houdini_escape_href(ob, (const uint8_t *)aLink.c_str(), aLink.size() );
            }
            else
            {
               houdini_escape_href( ob, link->data, link->size ) ;
            }
         }
      }
   }
	
   BUFPUTSL( ob, "\" alt=\"" ) ;

   if( alt && alt->size )
   {
      houdini_escape_html0( ob, alt->data, alt->size, 0 ) ;
   }

   if( title && title->size )
	  {
      BUFPUTSL( ob, "\" title=\"" ) ;
      houdini_escape_html0(ob, title->data, title->size,0);
   }

   bufputs( ob, ( options->flags & HTML_USE_XHTML ) ? "\"/></div>" : "\"></div>" ) ;
   return 1 ;
}

static void parseCode( struct buf *ob,
                       const struct buf *text,
                       const struct buf *lang,
                       void *opaque )
{
   size_t len ;
   if( ob->size )
   {
      bufputc( ob, '\n' ) ;
   }

   if( lang && lang->size )
   {
      size_t i, cls ;
      BUFPUTSL(ob, "<pre class=\"");

      for( i = 0, cls = 0; i < lang->size; ++i, ++cls )
      {
         while( i < lang->size && isspace( lang->data[i] ) )
         {
            i++;
         }

         if( i < lang->size)
         {
            size_t org = i ;
            while( i < lang->size && !isspace( lang->data[i] ) )
            {
               i++;
            }

            if( lang->data[org] == '.' )
            {
               org++;
            }

            if( cls )
            {
               bufputc(ob, ' ');
            }
            len = i - org ;
            //len = len > 1 ? len - 1 : len ;
            houdini_escape_html0(ob, lang->data + org, len, 0);
         }
      }
      BUFPUTSL(ob, "\">");
   }
   else
   {
      BUFPUTSL(ob, "<pre>");
   }

   if( text )
   {
      len = text->size ;
      len = len > 1 ? len - 1 : len ;
      houdini_escape_html0(ob, text->data, len, 0);
   }

   BUFPUTSL(ob, "</pre>\n");
}

static void parseHeader( struct buf *ob, const struct buf *text, int level, void *opaque )
{
   struct html_renderopt *options = (struct html_renderopt *)opaque;

   if (ob->size)
   {
      bufputc(ob, '\n');
   }

   if( level == 1 )
   {
      ++_titleNum ;
   }

   if( _convertMode == "normal" || _convertMode == "website" || _convertMode == "chm" )
   {
      if( options->flags & HTML_TOC )
      {
         bufprintf( ob, "<h%d id=\"", level ) ;
         houdini_escape_href( ob, text->data, text->size ) ;
         bufprintf( ob, "\" name=\"" ) ;
         houdini_escape_href( ob, text->data, text->size ) ;
         bufprintf( ob, "\">" ) ;
      }
      else
      {
         bufprintf( ob, "<h%d>", level ) ;
      }

      if( text )
      {
         bufput( ob, text->data, text->size ) ;

         if( level > 1 )
         {
            if( text->size > 0 )
            {
               BUFPUTSL( _pNavOb, "<li><a href=\"#" );

               houdini_escape_href( _pNavOb, text->data, text->size );

               BUFPUTSL( _pNavOb, "\">" );

               bufput( _pNavOb, text->data, text->size ) ;

               BUFPUTSL( _pNavOb, "</a></li>\n" );
            }
         }
      }

      bufprintf( ob, "</h%d>\n", level ) ;

      if( level == 1 && _titleNum == 1 && ( _convertMode == "normal" || _convertMode == "website" ) )
      {
         BUFPUTSL( ob, "<!--Nav Position-->" );
      }
   }
   else if( _convertMode == "single" )
   {
      string anchor( (const char *)text->data, text->size ) ;
      string filePath = _filePath ;
      ReplaceAll( filePath, ".md", "_html" ) ;
      ReplaceAll( filePath, "/", "_" ) ;
      ReplaceAll( filePath, "#", "_" ) ;
      anchor = filePath + "_" + anchor ;

      if( level == 1 )
      {
         bufprintf( ob, "<p id=\"" ) ;
         houdini_escape_href( ob, (const uint8_t *)filePath.c_str(), filePath.size() ) ;
         bufprintf( ob, "\" name=\"" ) ;
         houdini_escape_href( ob, (const uint8_t *)filePath.c_str(), filePath.size() ) ;
         bufprintf( ob, "\" class=\"subsection_header_%d\">", level ) ;
         bufprintf( ob, "</p>\n" ) ;
      }

      bufprintf( ob, "<p id=\"" ) ;
      houdini_escape_href( ob, (const uint8_t *)anchor.c_str(), anchor.size() ) ;
      bufprintf( ob, "\" name=\"" ) ;
      houdini_escape_href( ob, (const uint8_t *)anchor.c_str(), anchor.size() ) ;
      bufprintf( ob, "\" class=\"subsection_header_%d\"><b>", level ) ;
      if( text )
      {
         bufput( ob, text->data, text->size ) ;
      }
      bufprintf( ob, "</b></p>\n" ) ;
   }
   else if( _convertMode == "word" )
   {
      string anchor( (const char *)text->data, text->size ) ;
      string filePath = _filePath ;

      ReplaceAll( filePath, ".md", "" ) ;
      ReplaceAll( filePath, "/", "." ) ;
      map<string, INT32>::iterator iter = _pFileMap->find( filePath ) ;
      if( iter == _pFileMap->end() )
      {
         cout << "Warning toc.json Not find: " << _filePath << endl ;
      }
      else
      {
         CHAR catIdStr[30] ;
         sprintf( catIdStr, "%d", iter->second ) ;
         filePath = catIdStr ;
         anchor = filePath ; //+ "_" + anchor ;
         bufprintf( ob, "<p class=\"subsection_header_%d\">", level ) ;
         if( text )
         {
            bufput( ob, text->data, text->size ) ;
         }
         bufprintf( ob, "</p>\n" ) ;
      }
   }

}

void ReplaceAll( string &source, string oldStr, string newStr )
{
   INT32 nPos = 0 ;
   while( ( nPos = source.find( oldStr, nPos ) ) != source.npos )
   {
      source.replace( nPos, oldStr.length(), newStr ) ;
      nPos += newStr.length() ;
   }
}