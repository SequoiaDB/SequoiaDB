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

   Source File Name = catCMDBase.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains base class of catalog
   command class.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2019/10/02  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/

#include "catCMDBase.hpp"
#include "catTrace.hpp"

using namespace bson ;

namespace engine
{
   _catCmdBuilder::_catCmdBuilder ()
   {
   }

   _catCmdBuilder::~_catCmdBuilder ()
   {
      _mapCommand.clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDBUILDER__REGISTER, "_catCmdBuilder::_register" )
   INT32 _catCmdBuilder::_register ( const CHAR *name, CAT_CMD_NEW_FUNC pFunc )
   {
      INT32 rc = SDB_OK ;

      if ( !name || !(*name) || !pFunc )
      {
         SDB_ASSERT( FALSE, "Invalid parameters" ) ;
         rc = SDB_SYS ;
      }
      else
      {
         MAP_COMMAND_IT it = _mapCommand.find( name ) ;
         if ( it != _mapCommand.end() )
         {
            SDB_ASSERT( FALSE, "Command already exist" ) ;
            rc = SDB_SYS ;
         }
         else
         {
            try
            {
               _mapCommand.insert( MAP_COMMAND::value_type( name, pFunc ) ) ;
            }
            catch( std::exception &e )
            {
               rc = ossException2RC( &e ) ;
            }
         }
      }
      return rc ;
   }

   INT32 _catCmdBuilder::create ( const CHAR *name,
                                  _catCMDBase *&pCommand )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL == pCommand, "pCommand must be NULL" ) ;

      if ( !name || !(*name) )
      {
         SDB_ASSERT( FALSE, "Invalid parameters" ) ;
         rc = SDB_SYS ;
      }
      else
      {
         MAP_COMMAND_IT it = _mapCommand.find( name ) ;
         if ( it != _mapCommand.end() )
         {
            CAT_CMD_NEW_FUNC &pFunc = it->second ;
            pCommand = (*pFunc)() ;
            if ( !pCommand )
            {
               rc = SDB_OOM ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
      }
      return rc ;
   }

   void _catCmdBuilder::release ( _catCMDBase *&pCommand )
   {
      if ( pCommand )
      {
         SDB_OSS_DEL pCommand ;
         pCommand = NULL ;
      }
   }

   _catCmdBuilder *getCatCmdBuilder ()
   {
      static _catCmdBuilder cmdBuilder ;
      return &cmdBuilder ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDASSIT__CATCMDASSIT, "_catCmdAssit::_catCmdAssit" )
   _catCmdAssit::_catCmdAssit ( CAT_CMD_NEW_FUNC pFunc )
   {
      PD_TRACE_ENTRY ( SDB__CATCMDASSIT__CATCMDASSIT ) ;
      if ( pFunc )
      {
         _catCMDBase *pCommand = (*pFunc)() ;
         if ( pCommand )
         {
            getCatCmdBuilder()->_register ( pCommand->name(), pFunc ) ;
            SDB_OSS_DEL pCommand ;
            pCommand = NULL ;
         }
      }
      PD_TRACE_EXIT ( SDB__CATCMDASSIT__CATCMDASSIT ) ;
   }

}
