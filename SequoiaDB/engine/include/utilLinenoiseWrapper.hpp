/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = utilLinenoiseWrapper.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_LINENOISE_WRAPPER_HPP__
#define UTIL_LINENOISE_WRAPPER_HPP__

#include "core.hpp"
#include "oss.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <boost/function.hpp>
#include "../util/linenoise.h"

extern std::string historyFile ;

struct _linenoiseCmd : public SDBObject
{
   std::string    cmdName ;
   UINT32         nameSize ;
   BOOLEAN        leaf ;
   _linenoiseCmd  *sub ;
   _linenoiseCmd  *next ;
   _linenoiseCmd  *parent;
};
typedef _linenoiseCmd linenoiseCmd ;

typedef BOOLEAN (*canContinueNextLineCallback)( const CHAR * str ) ;

class _linenoiseCmdBuilder : public SDBObject
{
   public:
      _linenoiseCmdBuilder() { _pRoot = NULL ; }
      ~_linenoiseCmdBuilder()
      {
         if ( _pRoot )
         {
            _releaseNode( _pRoot ) ;
            _pRoot = NULL ;
          }
      }

   public:
      INT32    loadCmd( const CHAR *filename ) ;
      INT32    addCmd( const CHAR *cmd ) ;
      INT32    delCmd( const CHAR *cmd ) ;
      UINT32   getCompletions( const CHAR *cmd, CHAR *&fill, CHAR **&vec,
                               UINT32 &maxStrLen,
                               UINT32 maxSize = 50 ) ;
   public:
      INT32    _insert( _linenoiseCmd *node, const CHAR *cmd ) ;
      void     _releaseNode( _linenoiseCmd *node ) ;
      UINT32   _near( const CHAR *str1, const CHAR *str2 ) ;

      _linenoiseCmd *_prefixFind( const CHAR *cmd, UINT32 &sameNum ) ;
      UINT32   _getCompleteions( _linenoiseCmd *node, const std::string &prefix,
                                 BOOLEAN getSub,
                                 UINT32 &maxStrLen,
                                 std::vector<CHAR*> &vec,
                                 UINT32 maxSize ) ;

   private:
      _linenoiseCmd           *_pRoot ;

};
typedef _linenoiseCmdBuilder linenoiseCmdBuilder ;


linenoiseCmdBuilder* getLinenoiseCmdBuilder() ;
#define g_lnBuilder (*getLinenoiseCmdBuilder())

void lineComplete( const char *buf, linenoiseCompletions *lc ) ;

BOOLEAN canContinueNextLine ( const CHAR * str ) ;

BOOLEAN getNextCommand ( const CHAR *prompt, CHAR ** cmd,
                         BOOLEAN continueEnable = TRUE ) ;

BOOLEAN historyClear ( void ) ;
BOOLEAN historyInit ( void ) ;
void clearInputBuffer( void ) ;
BOOLEAN isStdinEmpty( void ) ;
void setCanContinueNextLineCallback( boost::function< BOOLEAN( const CHAR* ) >  funcObj ) ;
#endif //UTIL_LINENOISE_WRAPPER_HPP__

