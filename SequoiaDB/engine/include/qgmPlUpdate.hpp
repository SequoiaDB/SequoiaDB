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

   Source File Name = qgmPlUpdate.hpp

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

#ifndef QGMPLUPDATE_HPP_
#define QGMPLUPDATE_HPP_

#include "qgmPlan.hpp"
#include "qgmUtil.hpp"
#include "utilInsertResult.hpp"

namespace engine
{
   struct _qgmConditionNode;

   class _qgmPlUpdate : public _qgmPlan
   {
   public:
      _qgmPlUpdate( const _qgmDbAttr &collection,
                    const BSONObj &modifer,
                    _qgmConditionNode *condition,
                    INT32 flag ) ;

      virtual ~_qgmPlUpdate() ;

   public:
      virtual string toString() const
      {
         stringstream ss ;
         ss << "Type:" << qgmPlanType( _type ) << '\n';
         ss << "Updator:" << _updater.toString() << '\n';
         ss << "Condition:" << _condition.toString() << '\n';
         ss << "Flag:" << _flag << '\n';
         return ss.str() ;
      }

      virtual BOOLEAN needRollback() const ;
      virtual BOOLEAN canUseTrans() const ;
      virtual void    buildRetInfo( BSONObjBuilder &builder ) const ;
      virtual void    setClientVersion( INT32 version ) ;
      virtual INT32   getCatalogVersion() const ;

   protected:
      INT32 _updateVCS( const CHAR *fullName,
                        const BSONObj &updator,
                        _pmdEDUCB *cb ) ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext ( qgmFetchOut &next )
      {
         return SDB_SYS ;
      }

   private:
      _qgmDbAttr  _collection ;
      BSONObj     _updater ;
      BSONObj     _condition ;
      INT32       _flag ;

      utilUpdateResult     _upResult ;
      INT32       _clientVersion ;
      INT32       _catalogVersion ;
   } ;

   typedef class _qgmPlUpdate qgmPlUpdate ;
}

#endif

