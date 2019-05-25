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

   Source File Name = rtnAlterDef.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_ALTERDEF_HPP_
#define RTN_ALTERDEF_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   class _pmdEDUCB ;
   class _dpsLogWrapper ;

   enum RTN_ALTER_TYPE
   {
      RTN_ALTER_INVALID = 0,
      RTN_ALTER_TYPE_DB = 1,
      RTN_ALTER_TYPE_CL = 2,
      RTN_ALTER_TYPE_CS = 3,
      RTN_ALTER_TYPE_DOMAIN = 4,
      RTN_ALTER_TYPE_GROUP = 5,
      RTN_ALTER_TYPE_NODE = 6
   } ;

   class _rtnAlterOptions : public SDBObject
   {
   public:
      BOOLEAN ignoreException ;

      _rtnAlterOptions()
      :ignoreException( FALSE )
      {

      }

      void reset()
      {
         ignoreException = FALSE ;
      }
   } ;

   typedef INT32 ( *RTN_ALTER_FUNC )( const CHAR *name,
                                      const bson::BSONObj &pubArgs,
                                      const bson::BSONObj &args,
                                      _pmdEDUCB *cb,
                                      _dpsLogWrapper *dpsCB ) ;

   typedef INT32 ( *RTN_ALTER_VERIFY )( const bson::BSONObj &args ) ;


   enum RTN_ALTER_FUNC_TYPE
   {
      RTN_ALTER_FUNC_INVALID = 0,
      RTN_ALTER_CL_CRT_ID_IDX = 1,
      RTN_ALTER_CL_DROP_ID_IDX = 2
   } ;

   class _rtnAlterFuncObj : public SDBObject
   {
   public:
      const CHAR *name ;
      RTN_ALTER_TYPE objType ;
      RTN_ALTER_FUNC_TYPE type ;
      RTN_ALTER_FUNC func ;
      RTN_ALTER_VERIFY verify ;

      _rtnAlterFuncObj()
      :name( NULL ),
       objType( RTN_ALTER_INVALID ),
       type( RTN_ALTER_FUNC_INVALID ),
       func( NULL ),
       verify( NULL )
      {

      }

      _rtnAlterFuncObj( const CHAR *n,
                        RTN_ALTER_TYPE ot,
                        RTN_ALTER_FUNC_TYPE t,
                        RTN_ALTER_FUNC f,
                        RTN_ALTER_VERIFY v )
      :name( n ),
       objType( ot ),
       type( t ),
       func( f ),
       verify( v )
      {

      }

      BOOLEAN isValid() const
      {
         return NULL != name &&
                RTN_ALTER_INVALID != objType &&
                RTN_ALTER_FUNC_INVALID != type &&
                NULL != func &&
                NULL != verify ;
      }
   } ;
}

#endif

