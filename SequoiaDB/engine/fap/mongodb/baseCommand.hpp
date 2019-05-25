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

   Source File Name = baseCommand.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_FAP_MONGO_BASE_COMMAND_HPP_
#define _SDB_FAP_MONGO_BASE_COMMAND_HPP_

#include <map>
#include "util.hpp"
#include "mongodef.hpp"
#include "parser.hpp"

class baseCommand : public SDBObject
{
public:
   baseCommand( const CHAR *name, const CHAR *secondName = NULL ) ;

   virtual ~baseCommand() {} ;

   const CHAR *name() const
   {
      return _name ;
   }

   virtual INT32 convert( msgParser &parser )
   {
      return SDB_OK ;
   }

   virtual INT32 buildMsg( msgParser &parser, msgBuffer &sdbMsg )
   {
      return SDB_OK ;
   }

   virtual INT32 doCommand( void *pData = NULL )
   {
      return SDB_OK ;
   }

private:
   const CHAR *_name ;
} ;


class commandMgr : public SDBObject
{
public:
   static commandMgr *instance() ;

   void addCommand( const std::string &name, baseCommand *cmd )
   {
      baseCommand *tmp = _cmdMap[name] ;
      if ( NULL != tmp )
      {
      }
      _cmdMap[name] = cmd ;
   }

   baseCommand *findCommand( const std::string &name )
   {
      baseCommand *cmd = NULL ;
      std::map< std::string, baseCommand* >::iterator it = _cmdMap.find( name ) ;
      if ( _cmdMap.end() != it )
      {
         cmd = it->second ;
      }
      return cmd ;
   }

private:

   commandMgr()
   {
      _cmdMap.clear() ;
   }

   ~commandMgr()
   {
      _cmdMap.clear() ;
   }

private:
   std::map< std::string, baseCommand* > _cmdMap ;
} ;


#endif
