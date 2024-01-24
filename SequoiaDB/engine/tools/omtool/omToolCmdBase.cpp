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

   Source File Name = omToolCmdBase.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/09/2019  HJW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "omToolCmdBase.hpp"

namespace omTool
{
   omToolCmdBase::omToolCmdBase()
   {
   }

   omToolCmdBase::~omToolCmdBase()
   {
   }

   void omToolCmdBase::setOptions( omToolOptions *options )
   {
      _options = options ;
   }

   void omToolCmdBase::_setErrorMsg( const CHAR *pMsg )
   {
      _errMsg = pMsg ;
   }

   void omToolCmdBase::_setErrorMsg( const string &msg )
   {
      _errMsg = msg.c_str() ;
   }

   _omToolCmdAssit::_omToolCmdAssit( OMTOOL_NEW_FUNC pFunc )
   {
      if ( pFunc )
      {
         omToolCmdBase *pCommand = (*pFunc)() ;
         if ( pCommand )
         {
            getOmToolCmdBuilder()->_register ( pCommand->name(), pFunc ) ;
            SDB_OSS_DEL pCommand ;
            pCommand = NULL ;
         }
      }
   }

   _omToolCmdAssit::~_omToolCmdAssit()
   {
   }

   _omToolCmdBuilder::_omToolCmdBuilder ()
   {
   }

   _omToolCmdBuilder::~_omToolCmdBuilder ()
   {
   }

   omToolCmdBase* _omToolCmdBuilder::create( const CHAR *command )
   {
      OMTOOL_NEW_FUNC pFunc = _find ( command ) ;

      if ( pFunc )
      {
         return (*pFunc)() ;
      }

      return NULL ;
   }

   void _omToolCmdBuilder::release( omToolCmdBase *&pCommand )
   {
      if ( pCommand )
      {
         SDB_OSS_DEL pCommand ;
         pCommand = NULL ;
      }
   }

   INT32 _omToolCmdBuilder::_register ( const CHAR *name, OMTOOL_NEW_FUNC pFunc )
   {
      INT32 rc = SDB_OK ;
      pair< MAP_CMD_IT, BOOLEAN > ret ;

      if ( ossStrlen( name ) == 0 )
      {
         goto done ;
      }

      ret = _cmdMap.insert( pair<const CHAR*, OMTOOL_NEW_FUNC>(name, pFunc) ) ;
      if ( FALSE == ret.second )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   OMTOOL_NEW_FUNC _omToolCmdBuilder::_find( const CHAR *name )
   {
      if ( name )
      {
         MAP_CMD_IT it = _cmdMap.find( name ) ;
         if ( it != _cmdMap.end() )
         {
            return it->second ;
         }
      }
      return NULL ;
   }

   _omToolCmdBuilder* getOmToolCmdBuilder()
   {
      static _omToolCmdBuilder cmdBuilder ;
      return &cmdBuilder ;
   }
}