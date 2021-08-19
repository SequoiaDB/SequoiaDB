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

   Source File Name = qgmOptiUpdate.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef QGMOPTIUPDATE_HPP_
#define QGMOPTIUPDATE_HPP_

#include "qgmOptiTree.hpp"
#include "qgmHintDef.hpp"
#include "qgmUtil.hpp"

namespace engine
{
   class _qgmOptiUpdate : public _qgmOptiTreeNode
   {
   public:
      _qgmOptiUpdate( _qgmPtrTable *table, _qgmParamTable *param )
      :_qgmOptiTreeNode( QGM_OPTI_TYPE_UPDATE,
                         table, param ),
       _condition( NULL )
      {

      }

      virtual ~_qgmOptiUpdate()
      {
         SAFE_OSS_DELETE( _condition ) ;
      }

   public:
      virtual INT32 outputStream( qgmOpStream &stream )
      {
         return SDB_SYS ;
      }

      virtual string toString() const
      {
         return "update" ;
      }

      INT32 getHint( INT32 &flag ) const
      {
         INT32 rc = SDB_OK ;
         INT32 tmpFlag = 0 ;
         flag = 0 ;

         QGM_HINS::const_iterator itr = _hints.begin() ;
         for( ; itr != _hints.end(); ++itr )
         {
            if ( QGM_HINT_USEFLAG_SIZE == itr->value.size() &&
                 0 == ossStrncmp( itr->value.begin(), QGM_HINT_USEFLAG,
                                  itr->value.size() ) )
            {
               rc = qgmUseHintToFlag( *itr, tmpFlag ) ;
               if( rc )
               {
                  goto error ;
               }
               flag |= tmpFlag ;
            }
         }

      done :
         return rc ;
      error :
         goto done ;
      }

   public:
      _qgmDbAttr           _collection ;
      BSONObj              _modifer ;
      _qgmConditionNode    *_condition ;
   } ;
   typedef class _qgmOptiUpdate qgmOptiUpdate ;
}

#endif

