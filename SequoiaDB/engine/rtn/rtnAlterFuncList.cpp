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

   Source File Name = rtnAlterFuncList.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnAlterFuncList.hpp"
#include "dpsLogWrapper.hpp"
#include "rtnAlterFuncs.hpp"

namespace engine
{
   _rtnAlterFuncList::_rtnAlterFuncListInter _rtnAlterFuncList::_fl ;
   _rtnAlterFuncList::_rtnAlterFuncList()
   {

   }

   _rtnAlterFuncList::~_rtnAlterFuncList()
   {

   }

   INT32 _rtnAlterFuncList::getFuncObj( RTN_ALTER_TYPE type,
                                        const CHAR *name,
                                        _rtnAlterFuncObj &obj )
   {
      return _fl.getFuncObj( type, name, obj ) ;
   }

   INT32 _rtnAlterFuncList::getFuncObj( RTN_ALTER_FUNC_TYPE type,
                                        _rtnAlterFuncObj &obj )
   {
      return _fl.getFuncObj( type, obj ) ;
   }

   INT32 _rtnAlterFuncList::_rtnAlterFuncListInter::
         getFuncObj( RTN_ALTER_TYPE type,
                     const CHAR *name,
                     _rtnAlterFuncObj &obj )
   {
      INT32 rc = SDB_OK ;
      string lower ;
      rc = init() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      lower.assign( name ) ;
      boost::algorithm::to_lower( lower ) ;
      for ( FOBJ_LIST::const_iterator itr = _fl.begin();
            itr != _fl.end();
            ++itr )
      {
         if ( type == itr->objType &&
              0 == lower.compare( itr->name ) )
         {
            obj = *itr ;
            goto done ;
         }
      }

      rc = SDB_INVALIDARG ;
      goto error ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnAlterFuncList::_rtnAlterFuncListInter::
         getFuncObj( RTN_ALTER_FUNC_TYPE type,
                     _rtnAlterFuncObj &obj )
   {
      INT32 rc = SDB_OK ;
      rc = init() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      for ( FOBJ_LIST::const_iterator itr = _fl.begin();
            itr != _fl.end();
            ++itr )
      {
         if ( type == itr->type )
         {
            obj = *itr ;
            goto done ;
         }
      }

      rc = SDB_INVALIDARG ;
      goto error ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnAlterFuncList::_rtnAlterFuncListInter::init()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      if ( _inited )
      {
         goto done ;
      }

      _latch.get() ;
      locked = TRUE ;

      if ( _inited )
      {
         goto done ;
      }

      _fl.push_front( _rtnAlterFuncObj( SDB_ALTER_CRT_ID_INDEX,
                                        RTN_ALTER_TYPE_CL,
                                        RTN_ALTER_CL_CRT_ID_IDX,
                                        &rtnCreateIDIndex,
                                        &rtnCreateIDIndexVerify ) ) ;

      _fl.push_front( _rtnAlterFuncObj( SDB_ALTER_DROP_ID_INDEX,
                                        RTN_ALTER_TYPE_CL,
                                        RTN_ALTER_CL_DROP_ID_IDX,
                                        &rtnDropIDIndex,
                                        &rtnDropIDIndexVerify ) ) ;

      _inited = TRUE ;
   done:
      if ( locked )
      {
         _latch.release() ;
      }
      return rc ;
   }

}

