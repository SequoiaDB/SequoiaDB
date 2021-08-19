/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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
*******************************************************************************/

#include "ossTypes.hpp"
#include <gtest/gtest.h>
#define BOOST_SPIRIT_DEBUG
#include "sqlGrammar.hpp"
#include "sqlUtil.hpp"
#include "qgmBuilder.hpp"
#include "optQgmOptimizer.hpp"
#include "optQgmStrategy.hpp"
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include "qgmPlan.hpp"
#include "utilStr.hpp"
#include "qgmPlanContainer.hpp"
#include "utilLinenoiseWrapper.hpp"

#include "pmd.hpp"

#include <stdio.h>
#include <string>
#include <iostream>


using namespace engine;
using namespace std;

void dump( _qgmPlan *node, INT32 indent )
{
   cout << string( indent * 4, ' ') << "|--" << node->toString() << endl ;
   for ( UINT32 i = 0; i < node->inputSize(); i++ )
   {
      dump( node->input(i), indent+1) ;
   }
}

void dump(_qgmPlan *root)
{
   dump( root, 0 ) ;
}

BOOLEAN readLine ( const char* prompt, char* p, int length )
{
   BOOLEAN ret = TRUE ;
   char *readstr = NULL ;

   ret = getNextCommand( prompt, &readstr ) ;
   if ( readstr )
   {
      if ( ossStrlen( readstr ) >= (unsigned int)length )
      {
         cout << "input is to long" << std::endl ;
         ret = FALSE ;
      }
      else
      {
         ossStrncpy( p, readstr, length -1 ) ;
         p[ length-1 ] = 0 ;
      }

      SDB_OSS_FREE( readstr ) ;
      readstr = NULL ;
   }
   else
   {
      p[0] = 0 ;
   }

   return ret ;
}

string promptStr( INT32 numIndent, const char *prompt )
{
   string promptStr ;
   for ( INT32 i = 0; i< numIndent ; i++ )
   {
      promptStr += "    " ;
   }
   promptStr += prompt ;
   promptStr += "> " ;

   return promptStr ;
}

INT32 readInput ( const CHAR *pPrompt, INT32 numIndent,
                  CHAR *pInBuff, UINT32 buffLen )
{
   memset ( pInBuff, 0, buffLen ) ;

   string strPrompt = promptStr( numIndent, pPrompt ) ;
   string strPrompt1 = promptStr( numIndent, "" ) ;

   if ( !readLine ( strPrompt.c_str(), pInBuff, buffLen ) )
   {
      cout << "quit" << std::endl ;
      return SDB_APP_FORCED ;
   }

   // do a loop if the input end with '\\' character
   while ( pInBuff[ strlen(pInBuff)-1 ] == '\\' &&
           buffLen - strlen( pInBuff ) > 0 )
   {
      if ( !readLine ( strPrompt1.c_str(),
                       &pInBuff[ strlen(pInBuff)-1 ],
                       buffLen - strlen( pInBuff ) ) )
      {
         memset( pInBuff, 0, buffLen ) ;
         cout << "quit" << std::endl ;
         return SDB_APP_FORCED ;
      }
   }
   return SDB_OK ;
}

#define SQL_READLINE_LEN         ( 1024 * 1024 )
#define SQL_DUMPBUFF_LEN         ( 1024 * 1024 * 10 )

TEST(sqlTest, parse_1)
{
   INT32 rc = SDB_OK ;
   SQL_GRAMMAR grammar ;
   qgmOptiTreeNode *qgm = NULL ;
   CHAR *line = (CHAR*)SDB_OSS_MALLOC( SQL_READLINE_LEN ) ;
   CHAR *dumpBuf = (CHAR*)SDB_OSS_MALLOC( SQL_DUMPBUFF_LEN ) ;

   sdbEnablePD( NULL ) ;
   setPDLevel( PDDEBUG ) ;
   getQgmStrategyTable()->init() ;

   if ( !dumpBuf )
   {
      cout << "alloc memory failed" << std::endl ;
      goto done ;
   }

   linenoiseSetCompletionCallback( (linenoiseCompletionCallback*)lineComplete ) ;

   while ( TRUE )
   {
      _qgmPlanContainer container ;
      qgmBuilder builder( container.ptrTable(), container.paramTable() ) ;

      ossMemset( line, 0, SQL_READLINE_LEN ) ;
      ossMemset( dumpBuf, 0, SQL_DUMPBUFF_LEN ) ;

      if ( qgm )
      {
         SDB_OSS_DEL qgm ;
         qgm = NULL ;
      }

      rc = readInput( "sql", 0, line, SQL_READLINE_LEN ) ;
      if ( rc )
      {
         break ;
      }
      else if ( 0 == ossStrlen( line ) )
      {
         continue ;
      }

      if ( 0 == ossStrcasecmp( line, "quit" ) )
      {
         break ;
      }
      else if ( 0 == ossStrcasecmp( line, "clear" ) )
      {
         linenoiseClearScreen() ;
         continue ;
      }

      const CHAR *sql = NULL ;
      utilStrTrim( line, sql ) ;
      SQL_AST &ast = container.ast() ;
      ast = SQL_PARSE( sql, grammar ) ;
      _qgmPlan *plan = NULL ;

      cout << "match:"<< ast.match << " full:" << ast.full << endl ;

      if ( ast.match && ast.full )
      {
         sqlDumpAst( ast.trees ) ;
         cout << endl ;
         rc = builder.build( ast.trees, qgm ) ;
         if ( SDB_OK == rc )
         {
            qgm->dump() ;
            cout << "**************" << endl ;

            qgmOptiTreeNode *e = NULL ;
            rc = qgm->extend( e ) ;
            if ( e )
            {
               qgm = e ;
            }

            if ( SDB_OK != rc )
            {
               cout << "***ERROR*** Extend failed, rc = " << rc << endl ;
               continue ;
            }
            else
            {
               cout << "After extent, tree dump:" << endl ;
               e->dump() ;
            }

            // optimizer
            qgmOptTree tree( qgm ) ;
            optQgmOptimizer optimizer ;
            rc = optimizer.adjust( tree ) ;
            if ( qgm != tree.getRoot() )
            {
               qgm = tree.getRoot() ;
            }

            if ( SDB_OK != rc )
            {
               cout << "***ERROR*** Optimize failed, rc = " << rc << endl ;
               continue ;
            }
            else
            {
               cout << "After optimizer, tree dump:" << endl ;
               qgm->dump() ;
            }

            rc = builder.build( tree.getRoot(), plan ) ;
            if ( SDB_OK == rc )
            {
               cout << endl << "plan tree:" << endl ;
               container.plan() = plan ;
               rc = qgmDump( &container, dumpBuf, 1024*1024*10) ;
               if ( SDB_OK == rc )
               {
                  cout << dumpBuf << endl ;
               }
            }
            else
            {
               cout << "***ERROR*** Build plan tree failed, rc = "
                    << rc << endl ;
               continue ;
            }

            SDB_OSS_DEL qgm ;
            qgm = NULL ;
         }
      }
      else
      {
         cout << sql << endl ;
         cout << string( ast.stop - sql, ' ' ) << '^' << endl ;
      }
   }

done:
   if ( line )
   {
      SDB_OSS_FREE( line ) ;
   }
   if ( dumpBuf )
   {
      SDB_OSS_FREE( dumpBuf ) ;
   }
}

/*
TEST(sqlTest, abc1)
{
   boost::filesystem::path fullpath( boost::filesystem::initial_path()) ;
   string p("../abc/cba") ;
   fullpath = boost::filesystem::system_complete( boost::filesystem::path(p))  ;
   std::cout << fullpath.string() << endl ;
}
*/
