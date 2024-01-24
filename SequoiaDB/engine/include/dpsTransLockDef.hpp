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
          11/08/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPS_TRANSLOCK_DEF_HPP__
#define DPS_TRANSLOCK_DEF_HPP__

#include "ossTypes.h"
#include "oss.hpp"
#include "dms.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"     // SDB_ASSERT
#include <sstream>
#include "../bson/bson.h"

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      DEFINE LOCK TYPE
   */
   typedef UINT8 DPS_TRANSLOCK_TYPE ;

   //
   // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
   //
   // Do NOT modify the value of lock type defined below before
   // you verify the compatibility of the functions :
   //   isLockCompatible
   //   upgradeCheck
   //   lockCoverage
   //
   // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
   //
   #define   DPS_TRANSLOCK_IS  ((UINT8)0)
   #define   DPS_TRANSLOCK_IX  ((UINT8)1)
   #define   DPS_TRANSLOCK_S   ((UINT8)2)
   #define   DPS_TRANSLOCK_SIX ((UINT8)3)
   #define   DPS_TRANSLOCK_U   ((UINT8)4)
   #define   DPS_TRANSLOCK_X   ((UINT8)5)
   #define   DPS_TRANSLOCK_Z   ((UINT8)6)
   #define   DPS_TRANSLOCK_MAX ((UINT8)( DPS_TRANSLOCK_Z + 1 ))

   //
   // Lock compatibilities
   //
   //                            State of Held
   // -------------------------------------------------------
   // State          IS    IX    S     SIX   U     X     Z
   // Being
   // requested
   // -------------------------------------------------------
   // IS             Y     Y     Y     Y     Y     N     N
   // IX             Y     Y     N     N     N     N     N
   // S              Y     N     Y     N     Y     N     N
   // SIX            Y     N     N     N     N     N     N
   // U              Y     N     Y     N     N     N     N
   // X              N     N     N     N     N     N     N
   // Z              N     N     N     N     N     N     N
   static const UINT8 _LockCompatibilityMatrix[DPS_TRANSLOCK_MAX]
                                              [DPS_TRANSLOCK_MAX] =
     {{ 1,  1,  1,  1,  1,  0,  0 },
      { 1,  1,  0,  0,  0,  0,  0 },
      { 1,  0,  1,  0,  1,  0,  0 },
      { 1,  0,  0,  0,  0,  0,  0 },
      { 1,  0,  1,  0,  0,  0,  0 },
      { 0,  0,  0,  0,  0,  0,  0 },
      { 0,  0,  0,  0,  0,  0,  0 } } ;
   OSS_INLINE BOOLEAN dpsIsLockCompatible
   (
      const DPS_TRANSLOCK_TYPE current,
      const DPS_TRANSLOCK_TYPE request
   )
   {
      SDB_ASSERT( ( ( current < DPS_TRANSLOCK_MAX ) &&
                    ( request < DPS_TRANSLOCK_MAX ) ),
                  "Invalid arguments" ) ;
      return ( _LockCompatibilityMatrix[request][current] ? TRUE : FALSE ) ;
   }

   //
   //  lock upgrade check
   //
   // x: upgrade is not allowed or needed
   // v: upgrade is needed
   //
   // request     current mode
   // mode      IS   IX   S    SIX  U    X    Z
   // ------------------------------------------
   // IS        x    x    x    x    x    x    x
   // IX        v    x    x    x    x    x    x
   // S         v    x    x    x    x    x    x
   // SIX       v    v    v    x    x    x    x
   // U         v    x    v    x    x    x    x
   // X         v    v    v    v    v    x    x
   // Z         x    x    x    x    x    x    x
   //
   // Valid upgrade :
   //     IS --> IX
   //     IS --> S
   //     IS --> SIX
   //     IS --> U
   //     IX --> X
   //     IX --> SIX
   //      S --> X
   //      S --> U
   //    SIX --> X
   //      U --> X
   // WARNING: could not upgrade to Z
   //   when IUD, IX lock or X lock after lock escalation is applied on
   //   collection; if later drop collection is allowed( Z lock on collection ),
   //   then no way to rollback.
   static const UINT8 _LockUpgradeCheckMatrix[DPS_TRANSLOCK_MAX]
                                             [DPS_TRANSLOCK_MAX]=
     {{ 0, 0, 0, 0, 0, 0, 0 },
      { 1, 0, 0, 0, 0, 0, 0 },
      { 1, 0, 0, 0, 0, 0, 0 },
      { 1, 1, 1, 0, 0, 0, 0 },
      { 1, 0, 1, 0, 0, 0, 0 },
      { 1, 1, 1, 1, 1, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0 }} ;
   OSS_INLINE INT32 dpsUpgradeCheck
   (
      const DPS_TRANSLOCK_TYPE current,
      DPS_TRANSLOCK_TYPE &request
   )
   {
      SDB_ASSERT( ( ( current < DPS_TRANSLOCK_MAX ) &&
                    ( request < DPS_TRANSLOCK_MAX ) ),
                  "Invalid arguments" ) ;
      if ( ( DPS_TRANSLOCK_IX == current &&
             DPS_TRANSLOCK_S == request ) ||
           ( DPS_TRANSLOCK_S == current &&
             DPS_TRANSLOCK_IX == request ) )
      {
         request = DPS_TRANSLOCK_SIX ;
      }
      if ( _LockUpgradeCheckMatrix[request][current] )
      {
         return SDB_OK ;
      }
      return SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST ;
   }


   // Lock coverage
   //                   State of Held
   // -------------------------------------------------------
   // State          IS    IX    S     SIX   U     X     Z
   // Being
   // requested
   // -------------------------------------------------------
   // IS             Y     Y     Y     Y     Y     Y     Y
   // IX             N     Y     N     Y     N     Y     Y
   // S              N     N     Y     Y     Y     Y     Y
   // SIX            N     N     N     Y     N     Y     Y
   // U              N     N     N     N     Y     Y     Y
   // X              N     N     N     N     N     Y     Y
   // Z              N     N     N     N     N     N     Y
   static const UINT8 _LockCoverageMatrix[DPS_TRANSLOCK_MAX]
                                         [DPS_TRANSLOCK_MAX]=
     {{ 1, 1, 1, 1, 1, 1, 1 },
      { 0, 1, 0, 1, 0, 1, 1 },
      { 0, 0, 1, 1, 1, 1, 1 },
      { 0, 0, 0, 1, 0, 1, 1 },
      { 0, 0, 0, 0, 1, 1, 1 },
      { 0, 0, 0, 0, 0, 1, 1 },
      { 0, 0, 0, 0, 0, 0, 1 }} ;
   OSS_INLINE BOOLEAN dpsLockCoverage
   (
      const DPS_TRANSLOCK_TYPE current,
      const DPS_TRANSLOCK_TYPE request
   )
   {
      SDB_ASSERT( ( ( current < DPS_TRANSLOCK_MAX ) &&
                    ( request < DPS_TRANSLOCK_MAX ) ),
                  "Invalid arguments" ) ;
      return ( _LockCoverageMatrix[request][current] ? TRUE : FALSE ) ;
   }


   // get intent lock mode
   static DPS_TRANSLOCK_TYPE _intentLockMatrix[DPS_TRANSLOCK_MAX+1] =
   {
      DPS_TRANSLOCK_IS,  // <-- DPS_TRANSLOCK_IS
      DPS_TRANSLOCK_IX,  // <-- DPS_TRANSLOCK_IX
      DPS_TRANSLOCK_IS,  // <-- DPS_TRANSLOCK_S
      DPS_TRANSLOCK_IX,  // <-- DPS_TRANSLOCK_SIX
      DPS_TRANSLOCK_IX,  // <-- DPS_TRANSLOCK_U
      DPS_TRANSLOCK_IX,  // <-- DPS_TRANSLOCK_X
      DPS_TRANSLOCK_IX,  // <-- DPS_TRANSLOCK_Z
      DPS_TRANSLOCK_MAX
   };
   // get intent lock mode after lock escalation
   // only escalate for intent locks of S, U, X
   static DPS_TRANSLOCK_TYPE _escalateLockMatrix[ DPS_TRANSLOCK_MAX+1] =
   {
      DPS_TRANSLOCK_IS,  // <-- DPS_TRANSLOCK_IS
      DPS_TRANSLOCK_IX,  // <-- DPS_TRANSLOCK_IX
      DPS_TRANSLOCK_S,   // <-- DPS_TRANSLOCK_S, intent lock is IS
      DPS_TRANSLOCK_IX,  // <-- DPS_TRANSLOCK_SIX
      DPS_TRANSLOCK_X,   // <-- DPS_TRANSLOCK_U, intent lock is IX
      DPS_TRANSLOCK_X,   // <-- DPS_TRANSLOCK_X, intent lock is IX
      DPS_TRANSLOCK_IX,  // <-- DPS_TRANSLOCK_Z
      DPS_TRANSLOCK_MAX
   } ;
   OSS_INLINE DPS_TRANSLOCK_TYPE dpsIntentLockMode
   (
      const DPS_TRANSLOCK_TYPE request,
      BOOLEAN lockEscalated = FALSE
   )
   {
      SDB_ASSERT( ( request < (DPS_TRANSLOCK_MAX + 1) ), "Invalid argument" ) ;
      if ( lockEscalated )
      {
         return ( _escalateLockMatrix[ request ] ) ;
      }
      return ( _intentLockMatrix[request] ) ;
   }

   // check if owned intent lock is cover request lock in lower level
   // if covered, no need to acquire locks of lower level
   //
   // Lock cover lower
   //                   State of Held in upper level
   // -------------------------------------------------------
   // State          IS    IX    S     SIX   U     X     Z
   // Being
   // requested in
   // lower level
   // -------------------------------------------------------
   // IS             N     N     Y     Y     Y     Y     Y
   // IX             N     N     N     N     N     Y     Y
   // S              N     N     Y     Y     Y     Y     Y
   // SIX            N     N     N     N     N     Y     Y
   // U              N     N     N     N     N     Y     Y
   // X              N     N     N     N     N     Y     Y
   // Z              N     N     N     N     N     N     Y
   // NOTE: intent lock for U lock is IX lock, so only X lock or Z lock in
   //       upper level can cover U lock in lower level
   static const UINT8 _LockCoverLowerMatrix[DPS_TRANSLOCK_MAX]
                                           [DPS_TRANSLOCK_MAX]=
     {{ 0, 0, 1, 1, 1, 1, 1 },
      { 0, 0, 0, 0, 0, 1, 1 },
      { 0, 0, 1, 1, 1, 1, 1 },
      { 0, 0, 0, 0, 0, 1, 1 },
      { 0, 0, 0, 0, 0, 1, 1 },
      { 0, 0, 0, 0, 0, 1, 1 },
      { 0, 0, 0, 0, 0, 0, 1 }} ;
   OSS_INLINE BOOLEAN dpsIsCoverLowerLock( DPS_TRANSLOCK_TYPE ownedModeInUpper,
                                           DPS_TRANSLOCK_TYPE requestModeInLower )
   {
      SDB_ASSERT( ( ( ownedModeInUpper <= DPS_TRANSLOCK_MAX ) &&
                    ( requestModeInLower < DPS_TRANSLOCK_MAX ) ),
                  "Invalid arguments" ) ;
      return ( ( ( DPS_TRANSLOCK_MAX != ownedModeInUpper ) &&
                 ( _LockCoverLowerMatrix[ requestModeInLower ]
                                        [ ownedModeInUpper ] ) ) ?
                                                             TRUE : FALSE ) ;
   }

   // Convert lock mode to string
   static const CHAR* _lockModeString[DPS_TRANSLOCK_MAX] =
   { "IS",
     "IX",
     "S",
     "SIX",
     "U",
     "X",
     "Z"
   } ;
   OSS_INLINE const CHAR* lockModeToString ( const DPS_TRANSLOCK_TYPE lockMode )
   {
      SDB_ASSERT( ( lockMode < DPS_TRANSLOCK_MAX ), "Invalid argument" ) ;
      return _lockModeString[lockMode] ;
   }

   /*
      Name define
   */
   #define DPS_LOCKID_CSID          "CSID"
   #define DPS_LOCKID_CLID          "CLID"
   #define DPS_LOCKID_EXTENTID      "ExtentID"
   #define DPS_LOCKID_OFFSET        "Offset"

   #define DPS_LOCKID_STRING_MAX_SIZE      ( 128 )

   // Note :
   // In order to implement index page lock through record locking mechanism,
   // we construct a special lockId to delinetate
   // . an index page :
   //    ( CSId, -2, indexPageNumber, -1 )
   //
   // When DPS_LOCKID_CLSID field is filled with -2, we can tell a lock is
   // an index lock.
   #define DPS_LOCKID_IDX_COLLECTION ((UINT16)( -2 ))

   /*
      _dpsTransLockId define
   */
   class _dpsTransLockId : public SDBObject
   {
      public:
         _dpsTransLockId( UINT32 logicCSID,
                          UINT16 collectionID,
                          const dmsRecordID *recordID )
         {
            _logicCSID        = logicCSID ;
            _collectionID     = collectionID ;
            if ( recordID )
            {
               _recordExtentID= recordID->_extent ;
               _recordOffset  = recordID->_offset ;
            }
            else
            {
               _recordExtentID= DMS_INVALID_EXTENT ;
               _recordOffset  = DMS_INVALID_OFFSET ;
            }
         }

         _dpsTransLockId( const _dpsTransLockId &id )
         {
            operator=( id ) ;
         }

         _dpsTransLockId()
         {
            reset() ;
         }

         ~_dpsTransLockId()
         {
         }

      public:

         OSS_INLINE BOOLEAN   operator<  ( const _dpsTransLockId &rhs ) const ;
         OSS_INLINE BOOLEAN   operator== ( const _dpsTransLockId &rhs ) const ;
         OSS_INLINE _dpsTransLockId& operator= ( const _dpsTransLockId &rhs ) ;

         OSS_INLINE void            reset() ;

         OSS_INLINE BOOLEAN         isValid() const ;
         OSS_INLINE UINT32          lockIdHash() const ;
         OSS_INLINE BOOLEAN         isLeafLevel() const ;
         OSS_INLINE BOOLEAN         isRootLevel() const ;
         OSS_INLINE BOOLEAN         isSupportEscalation() const ;
         OSS_INLINE _dpsTransLockId upOneLevel() const ;

         /*
            Access functions
         */
         UINT32         csID() const { return _logicCSID ; }
         UINT16         clID() const { return _collectionID ; }
         dmsExtentID    extentID() const { return _recordExtentID ; }
         dmsOffset      offset() const { return _recordOffset ; }

         /*
            Format functions
         */
         OSS_INLINE ossPoolString toString() const ;
         OSS_INLINE const CHAR * toString( CHAR *buffer,
                                           UINT32 bufferSize ) const ;
         OSS_INLINE BSONObj      toBson() const ;
         OSS_INLINE void         toBson( BSONObjBuilder &builder ) const ;

      private:
         UINT32               _logicCSID ;
         UINT16               _collectionID ;
         dmsExtentID          _recordExtentID ;
         dmsOffset            _recordOffset ;
   } ;
   typedef _dpsTransLockId dpsTransLockId ;

   /*
      INLINE FUCTION
   */
   OSS_INLINE BOOLEAN _dpsTransLockId::operator< ( const _dpsTransLockId &rhs ) const
   {
      if ( _logicCSID < rhs._logicCSID )
      {
         return TRUE ;
      }
      else if ( _logicCSID > rhs._logicCSID )
      {
         return FALSE ;
      }
      if ( _collectionID < rhs._collectionID )
      {
         return TRUE ;
      }
      else if ( _collectionID > rhs._collectionID )
      {
         return FALSE ;
      }
      if ( _recordExtentID < rhs._recordExtentID )
      {
         return TRUE ;
      }
      else if ( _recordExtentID > rhs._recordExtentID )
      {
         return FALSE ;
      }
      if ( _recordOffset < rhs._recordOffset )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   OSS_INLINE BOOLEAN _dpsTransLockId::operator== ( const _dpsTransLockId &rhs ) const
   {
      if ( _logicCSID == rhs._logicCSID &&
           _collectionID == rhs._collectionID &&
           _recordExtentID == rhs._recordExtentID &&
           _recordOffset == rhs._recordOffset )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   OSS_INLINE _dpsTransLockId& _dpsTransLockId::operator= ( const _dpsTransLockId &rhs )
   {
      _logicCSID        = rhs._logicCSID ;
      _collectionID     = rhs._collectionID ;
      _recordExtentID   = rhs._recordExtentID ;
      _recordOffset     = rhs._recordOffset ;

      return *this ;
   }

   OSS_INLINE void _dpsTransLockId::reset()
   {
      _logicCSID        = ~0 ;
      _collectionID     = DMS_INVALID_MBID ;
      _recordExtentID   = DMS_INVALID_EXTENT ;
      _recordOffset     = DMS_INVALID_OFFSET ;
   }

   OSS_INLINE _dpsTransLockId _dpsTransLockId::upOneLevel() const
   {
      if ( DMS_INVALID_OFFSET != _recordOffset )
      {
         return _dpsTransLockId( _logicCSID, _collectionID, NULL ) ;
      }
      else
      {
         return _dpsTransLockId( _logicCSID, DMS_INVALID_MBID, NULL ) ;
      }
   }

   OSS_INLINE BOOLEAN _dpsTransLockId::isValid() const
   {
      if ( (UINT32)~0 == _logicCSID )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   OSS_INLINE UINT32 _dpsTransLockId::lockIdHash() const
   {
      UINT64 b = 0 ;
      if ( DPS_LOCKID_IDX_COLLECTION == _collectionID )
      {
         // index page lock only use CS and extent field
         b |= (UINT64)(_logicCSID & 0xFFFFFFFF) << 32 ;
         b |= (_recordExtentID & 0xFFFFFFFF) ;
      }
      else
      {
         if ( DMS_INVALID_OFFSET != _recordOffset )
         {
            // recordExtentID is unique within a CS, so no
            // need to use collectionID
            // 12 bits for CS, 24 bits for extentID, 28 bits for offset
            b |= (UINT64)(_logicCSID & 0xFFF) << 52 ;
            b |= (UINT64)(_recordExtentID & 0xFFFFFF) << 28 ;
            b |= (_recordOffset & 0xFFFFFFF) ;
         }
         else
         {
            b |= (UINT64)(_logicCSID & 0xFFFFFFFF) << 32 ;
            b |= (_collectionID & 0xFFFFFFF) ;
         }
      }

      // ossHash use DJB Hash ( Daniel J. Bernstein ) algorithm :
      //   h(i) = h(i-1) * 33 + str[i]
      // bitwise multiplication x << 5 + x it equivalent to x * 33,
      // where the magic 5 comes. However, there is no adequate
      // explaination on why 33 is choosed as multiplier
      return ossHash( (CHAR*)&( b ), (sizeof( b )), 5 ) ;
   }

   OSS_INLINE BOOLEAN _dpsTransLockId::isLeafLevel() const
   {
      if (    isValid()
           && ( DMS_INVALID_MBID   != _collectionID )
           && ( DMS_INVALID_EXTENT != _recordExtentID )
           && ( DMS_INVALID_OFFSET != _recordOffset ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   OSS_INLINE BOOLEAN _dpsTransLockId::isRootLevel() const
   {
      if (    isValid()
           && ( DMS_INVALID_MBID   == _collectionID )
           && ( DMS_INVALID_EXTENT == _recordExtentID )
           && ( DMS_INVALID_OFFSET == _recordOffset ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   OSS_INLINE BOOLEAN _dpsTransLockId::isSupportEscalation() const
   {
      if (    isValid()
           && ( DMS_INVALID_MBID   != _collectionID )
           && ( DMS_INVALID_EXTENT == _recordExtentID )
           && ( DMS_INVALID_OFFSET == _recordOffset ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   OSS_INLINE ossPoolString _dpsTransLockId::toString() const
   {
      CHAR lockIDStr[ DPS_LOCKID_STRING_MAX_SIZE + 1 ] = { 0 } ;
      return toString( lockIDStr, DPS_LOCKID_STRING_MAX_SIZE ) ;
   }

   OSS_INLINE const CHAR *_dpsTransLockId::toString( CHAR *buffer,
                                                     UINT32 bufferSize ) const
   {
      ossSnprintf( buffer, bufferSize,
                   DPS_LOCKID_CSID ":%u, "
                   DPS_LOCKID_CLID ":%u, "
                   DPS_LOCKID_EXTENTID ":%d, "
                   DPS_LOCKID_OFFSET ":%d",
                   csID(), clID(), extentID(), offset() ) ;
      return buffer ;
   }

   OSS_INLINE BSONObj _dpsTransLockId::toBson() const
   {
      BSONObjBuilder builder( 128 ) ;
      toBson( builder ) ;
      return builder.obj () ;
   }

   OSS_INLINE void _dpsTransLockId::toBson( BSONObjBuilder &builder ) const
   {
      builder.append( DPS_LOCKID_CSID, (INT32)csID() ) ;
      builder.append( DPS_LOCKID_CLID, (INT32)clID() ) ;
      builder.append( DPS_LOCKID_EXTENTID, (INT32)extentID() ) ;
      builder.append( DPS_LOCKID_OFFSET, (INT32)offset() ) ;
   }

   /*
      _dpsTransResult define
   */
   struct _dpsTransRetInfo
   {
      dpsTransLockId       _lockID ;
      DPS_TRANSLOCK_TYPE   _lockType ;
      EDUID                _eduID ;
      UINT32               _tid ;

      _dpsTransRetInfo()
      {
         _lockType      = DPS_TRANSLOCK_IS ;
         _eduID         = 0 ;
         _tid           = 0 ;
      }
   } ;
   typedef _dpsTransRetInfo dpsTransRetInfo ;

}

#endif // DPS_TRANSLOCK_DEF_HPP__

