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

   Source File Name = rtnSQLFunc.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTNSQLFUNC_HPP_
#define RTNSQLFUNC_HPP_

#include "core.hpp"
#include "qgmDef.hpp"
#include "pd.hpp"
#include "../bson/bson.h"
#include <vector>

using namespace bson ;

namespace engine
{
   typedef std::vector<BSONElement> RTN_FUNC_PARAMS ;

   class _rtnSQLFunc : public SDBObject
   {
   public:
      _rtnSQLFunc( const CHAR *pName = "" )
      {
         if ( pName )
         {
            _name = pName ;
         }
      }
      virtual ~_rtnSQLFunc()
      {
      }

   public:
      INT32 push( const RTN_FUNC_PARAMS &param  )
      {
         INT32 rc = SDB_OK ;

         if ( _alias.empty() )
         {
            PD_LOG( PDERROR, "not initialized yet." ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         if ( _param.size() != param.size() )
         {
            PD_LOG( PDERROR, "number of param should be %d rather than %d",
                    _param.size(), param.size() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         rc = _push( param ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      done:
         return rc ;
      error:
        goto done ;
      }

      INT32 init( const _qgmField &alias,
                  const vector<qgmOpField> &param )
      {
         _alias = alias ;
         _param = param ;
         return SDB_OK ;
      }

      const vector<qgmOpField> &param() const
      {
         return _param ;
      }

      const CHAR *name() const
      {
         return _name.c_str() ;
      }

      virtual BOOLEAN isAggr() const { return TRUE ; }
      virtual BOOLEAN isStat() const { return FALSE ; }

      virtual INT32 result( BSONObjBuilder &builder ) = 0 ;

      virtual void clear(){ return ; }

      string toString() const ;

   private:
      virtual INT32 _push( const RTN_FUNC_PARAMS &param ) = 0 ;


   protected:
      std::string _name ;
      _qgmField _alias ;
      vector<qgmOpField> _param ;
   } ;

   typedef class _rtnSQLFunc rtnSQLFunc ;
}

#endif

