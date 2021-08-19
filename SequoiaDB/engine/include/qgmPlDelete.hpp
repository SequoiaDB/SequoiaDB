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

   Source File Name = qgmPlDelete.hpp

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

#ifndef QGMPLDELETE_HPP_
#define QGMPLDELETE_HPP_

#include "qgmPlan.hpp"
#include "utilInsertResult.hpp"

namespace engine
{
   struct _qgmConditionNode ;

   class _qgmPlDelete : public _qgmPlan
   {
   public:
      _qgmPlDelete( const qgmDbAttr &collection,
                    _qgmConditionNode *condition ) ;
      virtual ~_qgmPlDelete() ;
   public:
      virtual string toString() const ;
      virtual BOOLEAN needRollback() const ;
      virtual BOOLEAN canUseTrans() const { return TRUE ; }
      void buildRetInfo( BSONObjBuilder &builder ) const ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext( qgmFetchOut &next )
      {
         return SDB_SYS ;
      }

   protected:
      INT32 _deleteVCS( const CHAR *fullName,
                        const BSONObj &deletor,
                        _pmdEDUCB *cb ) ;

   private:
      qgmDbAttr         _collection ;
      BSONObj           _condition ;
      utilDeleteResult  _delResult ;
   } ;

   typedef class _qgmPlDelete qgmPlDelete ;
}

#endif

