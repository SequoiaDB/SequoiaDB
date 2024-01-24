/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = utilLinenoiseWrapper.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilLinenoiseWrapper.hpp"
#include <boost/algorithm/string/trim.hpp>
#include "utilTrace.h"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include <sstream>

using namespace std ;
#define LINENOISE_MAX_LINE          (65535)
#define LINENOISE_MAX_INPUT_LEN     (16781312)

extern INT32 history_len ;
string historyFile ;
boost::function< BOOLEAN( const CHAR* ) > userdefCanContinueFuncObj ;

// PD_TRACE_DECLARE_FUNCTION ( SDB__LINENOISECMDBLD__RELSNODE, "_linenoiseCmdBuilder::_releaseNode" )
void _linenoiseCmdBuilder::_releaseNode( _linenoiseCmd * node )
{
   PD_TRACE_ENTRY ( SDB__LINENOISECMDBLD__RELSNODE );
   if ( node->next )
   {
      _releaseNode ( node->next ) ;
      node->next = NULL ;
   }
   if ( node->sub )
   {
      _releaseNode ( node->sub ) ;
      node->sub = NULL ;
   }

   if ( node->parent )
   {
      if ( node->parent->next == node )
      {
         node->parent->next = NULL ;
      }
      else
      {
         node->parent->sub = NULL ;
      }
   }

   SDB_OSS_DEL node ;
   PD_TRACE_EXIT ( SDB__LINENOISECMDBLD__RELSNODE );
}

UINT32 _linenoiseCmdBuilder::_near( const CHAR * str1, const CHAR * str2 )
{
   UINT32 same = 0 ;
   while ( str1[same] && str2[same] && str1[same] == str2[same] )
   {
      ++same ;
   }

   return same ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__LINENOISECMDBLD__LOADCMD, "_linenoiseCmdBuilder::loadCmd" )
INT32 _linenoiseCmdBuilder::loadCmd( const CHAR *filename )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB__LINENOISECMDBLD__LOADCMD );

   ifstream fin ;
   fin.open( filename ) ;
   CHAR buf[LINENOISE_MAX_LINE+1] = {0} ;
   while( fin.getline(buf, LINENOISE_MAX_LINE) )
   {
      rc = addCmd( buf ) ;
      if( rc )
         break ;
   }
   fin.close() ;

   PD_TRACE_EXITRC ( SDB__LINENOISECMDBLD__LOADCMD, rc );
   return rc ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__LINENOISECMDBLD__ADDCMD, "_linenoiseCmdBuilder::addCmd" )
INT32 _linenoiseCmdBuilder::addCmd( const CHAR * cmd )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB__LINENOISECMDBLD__ADDCMD );

   if ( NULL == cmd || 0 == ossStrlen( cmd ) )
   {
      goto done ;
   }

   if ( !_pRoot )
   {
      _pRoot = SDB_OSS_NEW _linenoiseCmd ;
      _pRoot->cmdName = cmd ;
      _pRoot->nameSize = ossStrlen ( cmd ) ;
      _pRoot->next = NULL ;
      _pRoot->sub = NULL ;
      _pRoot->leaf = TRUE ;
      _pRoot->parent = NULL ;
   }
   else
   {
      rc = _insert ( _pRoot, cmd ) ;
   }

 done:
   PD_TRACE_EXITRC ( SDB__LINENOISECMDBLD__ADDCMD, rc );
   return rc ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__LINENOISECMDBLD__DELCMD, "_linenoiseCmdBuilder::delCmd" )
INT32 _linenoiseCmdBuilder::delCmd( const CHAR * cmd )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB__LINENOISECMDBLD__DELCMD );
   UINT32 sameNum = 0 ;
   if ( !cmd )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   {
   _linenoiseCmd *node = _prefixFind( cmd, sameNum ) ;

   if ( !node || sameNum != node->nameSize || !node->leaf )
   {
      // not found
      rc = SDB_OK ;
      goto done ;
   }
   else if ( node->next )
   {
      rc = SDB_OK ;
      goto done ;
   }
   else if ( node == _pRoot )
   {
      _pRoot = _pRoot->sub ;
      if ( _pRoot )
      {
         _pRoot->parent = NULL ;
      }
   }
   else
   {
      if ( node->parent->next == node )
      {
         node->parent->next = node->sub ;
      }
      else
      {
         node->parent->sub = node->sub ;
      }

      if ( node->sub )
      {
         node->sub->parent = node->parent ;
      }
   }
   node->parent = NULL ;
   node->sub = NULL ;
   _releaseNode( node ) ;

   }
done :
   PD_TRACE_EXITRC ( SDB__LINENOISECMDBLD__DELCMD, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__LINENOISECMDBLD__INSERT, "_linenoiseCmdBuilder::_insert" )
INT32 _linenoiseCmdBuilder::_insert( _linenoiseCmd * node, const CHAR * cmd )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB__LINENOISECMDBLD__INSERT );
   _linenoiseCmd *newNode = NULL ;
   UINT32 sameNum = _near ( node->cmdName.c_str(), cmd ) ;

   if ( sameNum == 0 )
   {
      if ( !node->sub )
      {
         newNode = SDB_OSS_NEW _linenoiseCmd ;
         newNode->cmdName = cmd ;
         newNode->nameSize = ossStrlen(cmd) ;
         newNode->next = NULL ;
         newNode->sub = NULL ;
         newNode->leaf = TRUE ;

         node->sub = newNode ;
         newNode->parent = node ;
      }
      else
      {
         rc = _insert ( node->sub, cmd ) ;
      }
   }
   else if ( sameNum == node->nameSize )
   {
      if ( sameNum == ossStrlen ( cmd ) )
      {
         if ( !node->leaf )
         {
            node->leaf = TRUE ;
         }
         else
         {
            rc = SDB_SYS ; //already exist
         }
      }
      else if ( !node->next )
      {
         newNode = SDB_OSS_NEW _linenoiseCmd ;
         newNode->cmdName = &cmd[sameNum] ;
         newNode->nameSize = ossStrlen(&cmd[sameNum]) ;
         newNode->next = NULL ;
         newNode->sub = NULL ;
         newNode->leaf = TRUE ;

         node->next = newNode ;
         newNode->parent = node ;
      }
      else
      {
         rc = _insert ( node->next, &cmd[sameNum] ) ;
      }
   }
   else
   {
      //split next node first
      newNode = SDB_OSS_NEW _linenoiseCmd ;
      newNode->cmdName = node->cmdName.substr( sameNum ) ;
      newNode->nameSize = node->nameSize - sameNum ;
      newNode->next = node->next ;
      if ( node->next )
      {
         node->next->parent = newNode ;
      }
      newNode->sub = NULL ;
      newNode->leaf = node->leaf ;

      node->next = newNode ;
      newNode->parent = node ;

      //change cur node
      node->cmdName = node->cmdName.substr ( 0, sameNum ) ;
      node->nameSize = sameNum ;

      if ( sameNum != ossStrlen ( cmd ) )
      {
         node->leaf = FALSE ;

         rc = _insert ( newNode, &cmd[sameNum] ) ;
      }
      else
      {
         node->leaf = TRUE ;
      }
   }
   PD_TRACE_EXITRC ( SDB__LINENOISECMDBLD__INSERT, rc );
   return rc ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__LINENOISECMDBLD__GETCOMPLETIONS2, "_linenoiseCmdBuilder::getCompletions" )
UINT32 _linenoiseCmdBuilder::getCompletions( const CHAR * cmd,
                                             CHAR *&fill,
                                             CHAR **&vec,
                                             UINT32 &maxStrLen,
                                             UINT32 maxSize )
{
   UINT32 cnt = 0 ;
   PD_TRACE_ENTRY ( SDB__LINENOISECMDBLD__GETCOMPLETIONS2 );
   UINT32 sameNum = 0 ;
   _linenoiseCmd *node = NULL ;
   BOOLEAN getSub = FALSE ;

   if ( cmd && *cmd == 0 )
   {
      node = _pRoot ;
      getSub = TRUE ;
   }
   else if ( cmd )
   {
      node = _prefixFind( cmd, sameNum ) ;
      getSub = FALSE ;
   }

   if ( !node )
   {
      goto done ;
   }

   {
      UINT32 prefixLen = ossStrlen( cmd ) - sameNum ;
      std::string prefix = std::string(cmd).substr( 0, prefixLen ) ;

      // the cmd is not full the same the node name
      if ( *cmd && sameNum != node->nameSize )
      {
         std::string fillStr = prefix + node->cmdName ;
         UINT32 fillLen = ossStrlen( fillStr.c_str() ) + 1 ;
         fill = (CHAR*)malloc(fillLen) ;
         if ( !fill )
         {
            goto done ;
         }
         ossStrncpy(fill, fillStr.c_str(), fillLen-1) ;
         fill[fillLen-1] = 0 ;
      }

      std::vector<CHAR*> vecCompletions ;
      UINT32 count = _getCompleteions( node, prefix, getSub, maxStrLen,
                                       vecCompletions, maxSize ) ;
      if ( count > 0 )
      {
         vec = (CHAR**)malloc( count * sizeof(CHAR*) ) ;
         if ( !vec )
         {
            goto done ;
         }
         for ( UINT32 i = 0 ; i < count ; i++ )
         {
            vec[i] = vecCompletions[i] ;
         }
         vecCompletions.clear() ;
      }
      cnt = count ;
   }
done :
   PD_TRACE1 ( SDB__LINENOISECMDBLD__GETCOMPLETIONS2, PD_PACK_UINT(cnt) );
   PD_TRACE_EXIT ( SDB__LINENOISECMDBLD__GETCOMPLETIONS2 );
   return cnt ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__LINENOISECMDBLD__PREFIND, "_linenoiseCmdBuilder::_prefixFind" )
_linenoiseCmd* _linenoiseCmdBuilder::_prefixFind( const CHAR * cmd,
                                                  UINT32 & sameNum )
{
   PD_TRACE_ENTRY ( SDB__LINENOISECMDBLD__PREFIND );
   _linenoiseCmd *pNode = _pRoot ;
   UINT32 index = 0 ;
   BOOLEAN findRoot = FALSE ;
   UINT32 cmdLen = ossStrlen( cmd ) ;
   sameNum = 0 ;
   UINT32 nearNum = 0 ;

   while ( pNode )
   {
      findRoot = FALSE ;
      if ( pNode->cmdName.at ( 0 ) != cmd[index] )
      {
         pNode = pNode->sub ;
      }
      else
      {
         findRoot = TRUE ;
      }

      if ( findRoot )
      {
         nearNum = _near( pNode->cmdName.c_str(), &cmd[index] ) ;

         if ( nearNum != cmdLen - index &&
              nearNum != pNode->nameSize )
         {
            break ;
         }

         if ( nearNum == pNode->nameSize )
         {
            index += pNode->nameSize ;
         }
         else if ( nearNum == cmdLen - index )
         {
            index = cmdLen ;
         }

         if ( cmd[index] == 0 ) //find
         {
            sameNum = nearNum ;
            PD_TRACE_EXIT ( SDB__LINENOISECMDBLD__PREFIND );
            return pNode ;
         }

         pNode = pNode->next ;
      }
   }
   PD_TRACE_EXIT ( SDB__LINENOISECMDBLD__PREFIND );
   return NULL ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__LINENOISECMDBLD__GETCOMPLETIONS, "_linenoiseCmdBuilder::_getCompleteions" )
UINT32 _linenoiseCmdBuilder::_getCompleteions( _linenoiseCmd * node,
                                               const std::string &prefix,
                                               BOOLEAN getSub,
                                               UINT32 &maxStrLen,
                                               vector <CHAR *> &vec,
                                               UINT32 maxSize )
{
   UINT32 count = 0 ;
   PD_TRACE_ENTRY ( SDB__LINENOISECMDBLD__GETCOMPLETIONS );
   if ( !node || vec.size() >= maxSize )
   {
      goto done ;
   }

   if ( node->leaf )
   {
      std::string completion = prefix + node->cmdName ;
      UINT32 len = ossStrlen( completion.c_str() ) + 1 ;
      CHAR *str = (CHAR*)malloc( len ) ;
      if ( !str )
      {
         goto done ;
      }
      ossStrncpy( str, completion.c_str(), len-1 ) ;
      str[len-1] = 0 ;
      vec.push_back( str ) ;
      count++ ;
      if ( len > maxStrLen )
      {
         maxStrLen = len ;
      }
   }

   if ( node->next )
   {
      count += _getCompleteions( node->next, prefix+node->cmdName,
                                 TRUE, maxStrLen, vec, maxSize ) ;
   }

   if ( getSub && node->sub )
   {
      count += _getCompleteions( node->sub, prefix, TRUE, maxStrLen,
                                 vec, maxSize ) ;
   }
done :
   PD_TRACE1 ( SDB__LINENOISECMDBLD__GETCOMPLETIONS, PD_PACK_UINT(count) );
   PD_TRACE_EXIT ( SDB__LINENOISECMDBLD__GETCOMPLETIONS );
   return count ;
}

/// Tool functions

linenoiseCmdBuilder* getLinenoiseCmdBuilder()
{
   static linenoiseCmdBuilder s_lnCmdBuilder ;
   return &s_lnCmdBuilder ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_LINECOMPLETE, "lineComplete" )
void lineComplete( const char *buf, linenoiseCompletions *lc )
{
   PD_TRACE_ENTRY ( SDB_LINECOMPLETE );
   UINT32 maxStrLen = 0 ;
   lc->len = g_lnBuilder.getCompletions( buf, lc->fill, lc->cvec,
                                         maxStrLen ) ;
   lc->maxStrLen = (size_t)maxStrLen ;
   PD_TRACE_EXIT ( SDB_LINECOMPLETE );
}


// PD_TRACE_DECLARE_FUNCTION ( SDB_CANCONTINUENXTLINE, "canContinueNextLine" )
BOOLEAN canContinueNextLine ( const CHAR * str )
{

   BOOLEAN  ret         = FALSE ;
   UINT32 strlen        = 0 ;
   CHAR ch              = '\0' ;
   const CHAR *mark     = str ;
   BOOLEAN flag1        = FALSE ;   // for ""
   BOOLEAN flag2        = FALSE ;   // for ''

   SDB_ASSERT ( str , "invalid argument" ) ;
   PD_TRACE_ENTRY ( SDB_CANCONTINUENXTLINE );
   if( !userdefCanContinueFuncObj.empty() )
   {
      ret = userdefCanContinueFuncObj( str ) ;
   }
   else
   {
      try
      {
         vector< CHAR > parens ;
         while ( ( ch = *str ) != '\0' )
         {
         	++strlen ;
            // we won't check the "()\[]\{}" in '' or ""
            if ( ( ch == '\"' ) && flag2 == FALSE )
            {
                // skip "\"", because "\"" can use as content
                if ( str != mark )
                {
                    const CHAR *temp_mark = str ;
                    BOOLEAN flag = TRUE ; // need to set flag1 to be !flag1 or not
                    while( (--str) != mark )
                    {
                        if ( *str == '\\' )
                        {
                            flag = !flag ;
                        }
                        else
                        {
                            break ;
                        }
                    }
                    if ( TRUE == flag )
                    {
                        flag1 = !flag1 ;
                    }
                    str = temp_mark ;
                }
                else
                {
                      // the first time we meed "
                      flag1 = TRUE ;
                }
            }
            if ( ( ch == '\'' ) && flag1 == FALSE )
            {
                // skip "\'", because "\'" can use as content
                if ( str != mark )
                {
                    const CHAR *temp_mark = str ;
                    BOOLEAN flag = TRUE ; // need to set flag2 to be !flag2 or not
                    while( (--str) != mark )
                    {
                        if ( *str == '\\' )
                        {
                            flag = !flag ;
                        }
                        else
                        {
                            break ;
                        }
                    }
                    if ( TRUE == flag )
                    {
                        flag2 = !flag2 ;
                    }
                    str = temp_mark ;
                }
                else
                {
                    // the first time we meed '
                    flag2 = TRUE ;
                }
            }
            str++ ;
            if ( flag1 == TRUE || flag2 == TRUE )
            {
               continue ;
            }

            switch ( ch )
            {
            case '{' :
            case '[' :
            case '(' :
               parens.push_back ( ch ) ;
               break ;

            case '}' :
               if ( ! parens.empty() && '{' == parens.back() )
                  parens.pop_back() ;
               else
                  goto error ;
               break ;

            case ']':
               if ( ! parens.empty() && '[' == parens.back() )
                  parens.pop_back() ;
               else
                  goto error ;
               break ;

            case ')' :
               if ( ! parens.empty() && '(' == parens.back() )
                  parens.pop_back() ;
               else
                  goto error ;
               break ;
            }
         }

         if ( strlen > LINENOISE_MAX_INPUT_LEN )
         {
            ret = FALSE ;
         }
         else if ( flag1 == TRUE || flag2 == TRUE || !parens.empty() )
         {
            ret = TRUE ;
         }
      }
      catch ( bad_alloc & )
      {
         ret = FALSE ;
      }
   }
done :
   PD_TRACE_EXITRC ( SDB_CANCONTINUENXTLINE, ret );
   return ret ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_HISTORYCLEAR, "historyClear" )
BOOLEAN historyClear ( void )
{
   BOOLEAN  ret   = FALSE ;
   INT32 i = 0 ;
   const CHAR *firstHistory = NULL ;
   PD_TRACE_ENTRY ( SDB_HISTORYCLEAR );
   // clear the history used for completions
   for ( i=0; i<history_len; i++ )
   {
         firstHistory = linenoiseHistoryGet( i ) ;
         if ( firstHistory )
         {
               g_lnBuilder.delCmd( firstHistory ) ;
         }
   }
   // clear the history in linenoise
   linenoiseHistoryClear() ;
   ret = TRUE ;
   PD_TRACE_EXITRC ( SDB_HISTORYCLEAR, ret );
   return ret ;
}

// Return false if the use presses Ctrl+c when typing first line, that means
// "has NO next command". *cmd is guaranteed to be NULL.
//
// Otherwise return true regardless of error occuring.
// You should test whether *cmd is null or an empty string on this case.
// And free *cmd if not null.
// PD_TRACE_DECLARE_FUNCTION ( SDB_GETNXTCMD, "getNextCommand" )
BOOLEAN getNextCommand ( const CHAR *prompt, CHAR ** cmd,
                         BOOLEAN continueEnable )
{
   BOOLEAN        firstline   = TRUE ;
   char *         line        = NULL ;
   BOOLEAN        ret         = TRUE ;
   static std::string firstCmd = "" ;

   SDB_ASSERT ( cmd , "invalid argument" ) ;
   SDB_ASSERT ( prompt, "invalid argument" ) ;
   PD_TRACE_ENTRY ( SDB_GETNXTCMD );

   *cmd = NULL ;

   try
   {
      string input = "" ;
      while ( TRUE )
      {
         // line is guarenteed by linenoise library that it doesn't contain
         // trailing \n or \r. It is freed after added to input or at end of
         // this function.
         line = linenoise ( firstline ? prompt : "... " ) ;
         if ( line )
         {
            firstline = FALSE ;
            input += line ;
            // line is allocated by linenoise, so we have to free using C
            // function
            free ( line ) ;
            line = NULL ;

            if ( continueEnable && canContinueNextLine ( input.c_str() ) )
            {
               continue ;
            }
            else
            {
               break ;
            }
         }
         else
         {
            if ( EAGAIN == errno || ECANCELED == errno ) // Ctrl + c
            {
               if ( EAGAIN == errno )
                  ret = FALSE ;
               input = "" ;
               linenoiseHistorySave( historyFile.c_str() ) ;
               break ;
            }
         }
      }

      boost::algorithm::trim ( input ) ;
      if ( input.size() > 0 )
      {
         const CHAR *firstHistory = linenoiseHistoryGet( 0 ) ;
         if ( firstHistory )
         {
            if ( 0 != ossStrcmp( firstCmd.c_str(), firstHistory ) )
            {
               g_lnBuilder.delCmd( firstCmd.c_str() ) ;
               firstCmd = "" ;
            }
         }

         if ( firstCmd.empty() && linenoiseHistoryGet( 0 ) )
         {
            firstCmd = linenoiseHistoryGet( 0 ) ;
         }
      }
      *cmd = ossStrdup ( input.c_str() ) ;
   }
   catch ( bad_alloc & )
   {
      *cmd = NULL ;
   }

   // line is allocated by linenoise, so we have to free using C
   // function
   if ( line )
   {
      free ( line ) ;
      line = NULL ;
   }
   PD_TRACE_EXITRC ( SDB_GETNXTCMD, ret );
   return ret ;
}

// initialize the history
// PD_TRACE_DECLARE_FUNCTION ( SDB_HISTORYINIT, "historyInit" )
BOOLEAN historyInit ( void )
{
   BOOLEAN  ret   = FALSE ;

   PD_TRACE_ENTRY( SDB_HISTORYINIT ) ;

   stringstream sstream ;
   const CHAR *pName = ".sequoiadb_shell_history" ;
   const char *pPath = NULL ;
#ifdef _WIN32
   pPath = getenv ( "USERPROFILE" ) ;
#else
   pPath = getenv ( "HOME" ) ;
#endif
   if ( !pPath )
      goto error ;
   sstream << pPath << '/' << pName ;
   historyFile = sstream.str() ;
   ret = TRUE ;

done :
   PD_TRACE_EXITRC ( SDB_HISTORYINIT, ret );
   return ret ;

error :
   goto done ;
}

void clearInputBuffer( void )
{
   linenoiseClearInputBuffer() ;
}

BOOLEAN isStdinEmpty( void )
{
   return linenoiseIsStdinEmpty() ;
}

void setCanContinueNextLineCallback( boost::function< BOOLEAN( const CHAR* ) >  funcObj )
{
   userdefCanContinueFuncObj = funcObj ;
}

