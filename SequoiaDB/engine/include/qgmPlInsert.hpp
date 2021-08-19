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

   Source File Name = qgmPlInsert.hpp

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

#ifndef QGMPLINSERT_HPP_
#define QGMPLINSERT_HPP_

#include "qgmPlan.hpp"
#include "msgDef.h"
#include "utilInsertResult.hpp"

namespace engine
{
   class _qgmPlInsert : public _qgmPlan
   {
   public:
      _qgmPlInsert( const qgmDbAttr &collection,
                    const BSONObj &record = BSONObj() ) ;

      virtual ~_qgmPlInsert() ;

   public:
      virtual string toString() const ;

      virtual BOOLEAN needRollback() const ;
      virtual BOOLEAN canUseTrans() const { return TRUE ; }
      virtual void    buildRetInfo( BSONObjBuilder &builder ) const ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext ( qgmFetchOut &next ) ;

      OSS_INLINE BOOLEAN _directInsert() const
      {
         return _input.size() == 0 ? TRUE : FALSE ;
      }

      INT32 _nextRecord( _pmdEDUCB *eduCB, BSONObj &obj ) ;

   protected:
      INT32 _insertVCS( const CHAR *fullName,
                        const BSONObj &insertor,
                        _pmdEDUCB *cb ) ;

   private:
      ossPoolString  _fullName ;
      BSONObj        _insertor ;
      BOOLEAN        _got ;
      SDB_ROLE       _role ;

      utilInsertResult  _inResult ;
   } ;

   typedef class _qgmPlInsert qgmPlInsert ;
}

#endif

