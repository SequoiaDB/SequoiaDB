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

   Source File Name = dpsTransLockDef.hpp

   Descriptive Name = Operating System Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSTRANSLOCKDEF_HPP_
#define DPSTRANSLOCKDEF_HPP_

#include "ossTypes.h"
#include <map>
#include <string>
#include "dms.hpp"
#include "ossAtomic.hpp"
#include "msg.h"
#include "../util/fromjson.hpp"

namespace engine
{
   class _pmdEDUCB;

   enum DPS_TRANSLOCK_TYPE
   {
      DPS_TRANSLOCK_IS = 0,
      DPS_TRANSLOCK_IX,
      DPS_TRANSLOCK_S,
      DPS_TRANSLOCK_X
   };

   /*enum DPS_TRANSLOCK_STATUS
   {
      DPS_TRANSLOCK_GOT = 0,
      DPS_TRANSLOCK_WAIT
   };*/

   typedef std::map<UINT32, MsgRouteID>      DpsTransNodeMap;

   class dpsTransLockId : public SDBObject
   {
   public:
      dpsTransLockId( UINT32 logicCSID,
                      UINT16 collectionID,
                      const _dmsRecordID *recordID );
      dpsTransLockId();
      ~dpsTransLockId();

      BOOLEAN operator<( const dpsTransLockId &rhs ) const;

      BOOLEAN operator==( const dpsTransLockId &rhs ) const;

      dpsTransLockId & operator=( const dpsTransLockId & rhs ) ;

      std::string toString() const;

      bson::BSONObj toBson() const ;

      void        reset() ;
      BOOLEAN     isValid() const ;

   public:
      UINT32               _logicCSID;
      dmsExtentID          _recordExtentID;
      dmsOffset            _recordOffset;
      UINT16               _collectionID;
   };

   class dpsTransCBLockInfo : public SDBObject
   {
   public:
      dpsTransCBLockInfo( DPS_TRANSLOCK_TYPE lockType );
      ~dpsTransCBLockInfo();
      INT64 incRef();
      INT64 decRef();
      BOOLEAN isLockMatch( DPS_TRANSLOCK_TYPE type );
      DPS_TRANSLOCK_TYPE getType();
      void setType( DPS_TRANSLOCK_TYPE lockType );
      _pmdEDUCB *getNextWaitCB();
      void setNextWaitCB( _pmdEDUCB *pWaitCB );
   private:
      _pmdEDUCB                  *_pNextWaitCB ;
      DPS_TRANSLOCK_TYPE         _lockType ;
      ossAtomicSigned64          *_pRef;
   };

   typedef std::map< dpsTransLockId, dpsTransCBLockInfo * >    DpsTransCBLockList;

}

#endif // DPSTRANSLOCKDEF_HPP_
