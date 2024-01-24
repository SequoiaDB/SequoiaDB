/******************************************************************************


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

   Source File Name = dpsTransLRB.hpp

   Descriptive Name = DPS LRB ( lock request block )

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/10/2018  JT  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSTRANSLRB_HPP_
#define DPSTRANSLRB_HPP_

#include "ossLatch.hpp"         // ossSpinXLatch
#include "dpsTransLockDef.hpp"  // DPS_TRANSLOCK_TYPE, dpsTransLockId
#include "ossAtomic.hpp"
#include "utilPooledObject.hpp"
#include "pmdDef.hpp"           // PMD_INVALID_EDUID

namespace engine
{
   class _dpsTransExecutor ;
   class dpsTransLRBHeader ;

   #define DPS_LRB_STATUS_NONE  ( (UINT8) 0x00 )
   #define DPS_LRB_STATUS_AWAKE ( (UINT8) 0x01 )


   // Lock Request Block ( LRB )
   class dpsTransLRB : public utilPooledObject
   {
   public :
      _dpsTransExecutor * dpsTxExectr ;
      dpsTransLRB       * eduLrbNext ; // next LRB in chain EDU owning in tx
      dpsTransLRB       * eduLrbPrev ; // prev LRB in chain EDU owning in tx
      dpsTransLRBHeader * lrbHdr ;     // the LRB Header
      dpsTransLRB       * nextLRB ;    // next LRB in the owner/waiter chain
      dpsTransLRB       * prevLRB ;    // prev LRB in the owner/waiter chain
      ossTick             beginTick ;  // timestamp( ossTick ) of owning/waiting
      UINT32              refCounter ; // lock reference counter
      DPS_TRANSLOCK_TYPE  lockMode ;   // lock mode owned/request, UINT8, 1 byte
      DPS_TRANSLOCK_TYPE  originMode ; // origin lock mode before upgrade
      UINT8               status ;     // 1 byte for status, wake up
      UINT8               pad[1] ;     // 1 byte for padding

      dpsTransLRB( _dpsTransExecutor  *_dpsTxExectr,
                   DPS_TRANSLOCK_TYPE  _lockMode,
                   dpsTransLRBHeader  *_lrbHdr )
      : dpsTxExectr ( _dpsTxExectr ),
        eduLrbNext( NULL ),
        eduLrbPrev( NULL ),
        lrbHdr( _lrbHdr ),
        nextLRB( NULL ),
        prevLRB( NULL ),
        refCounter( 1 ),
        lockMode( _lockMode ),
        originMode( DPS_TRANSLOCK_MAX ),
        status( DPS_LRB_STATUS_NONE )
      {
      }

      void reset()
      {
         dpsTxExectr = NULL ;
         eduLrbNext  = NULL ;
         eduLrbPrev  = NULL ;
         lrbHdr      = NULL ;
         nextLRB     = NULL ;
         prevLRB     = NULL ;
         refCounter  = 0 ;
         lockMode    = DPS_TRANSLOCK_MAX ;
         originMode  = DPS_TRANSLOCK_MAX ;
         status      = DPS_LRB_STATUS_NONE ;
      }
   } ;  // 64 bytes in total

   class dpsLRBExtData ;

   typedef BOOLEAN (*DPS_EXTDATA_VALID_FUNC)( const dpsLRBExtData *pExtData ) ;
   typedef BOOLEAN (*DPS_EXTDATA_RELEASE_CHECK)(const dpsLRBExtData *pExtData) ;
   typedef void    (*DPS_EXTDATA_RELEASE)( dpsLRBExtData *pExtData ) ;
   typedef void    (*DPS_EXTDATA_ON_LOCKRELEASE)( const dpsTransLockId &lockId,
                                                  DPS_TRANSLOCK_TYPE lockMode,
                                                  UINT32 refCounter,
                                                  dpsLRBExtData *pExtData,
                                                  INT32 idxLID,
                                                  BOOLEAN hasLock ) ;

   class dpsLRBExtData : public utilPooledObject
   {
   public:
      UINT64      _data ;

      DPS_EXTDATA_VALID_FUNC     _validFunc ;
      DPS_EXTDATA_RELEASE_CHECK  _releaseCheckFunc ;
      DPS_EXTDATA_RELEASE        _releaseFunc ;
      DPS_EXTDATA_ON_LOCKRELEASE _onLockReleaseFunc ;

      dpsLRBExtData()
      {
         _data = 0 ;
         _validFunc = NULL ;
         _releaseCheckFunc = NULL ;
         _releaseFunc = NULL ;
         _onLockReleaseFunc = NULL ;
      }

      void  setValidFunc( DPS_EXTDATA_VALID_FUNC pFunc )
      {
         _validFunc = pFunc ;
      }
      void  setReleaseCheckFunc( DPS_EXTDATA_RELEASE_CHECK pFunc )
      {
         _releaseCheckFunc = pFunc ;
      }
      void  setReleaseFunc( DPS_EXTDATA_RELEASE pFunc )
      {
         _releaseFunc = pFunc ;
      }
      void  setOnLockReleaseFunc( DPS_EXTDATA_ON_LOCKRELEASE pFunc )
      {
         _onLockReleaseFunc = pFunc ;
      }

      BOOLEAN  canRelease() const
      {
         if ( _releaseCheckFunc )
         {
            return _releaseCheckFunc( this ) ;
         }
         return TRUE ;
      }

      BOOLEAN  isValid() const
      {
         if ( _validFunc )
         {
            return _validFunc( this ) ;
         }
         return FALSE ;
      }

      BOOLEAN  release()
      {
         if ( _releaseFunc )
         {
            _releaseFunc( this ) ;
            return TRUE ;
         }
         return FALSE ;
      }

      void reset()
      {
         _data = 0 ;
         _validFunc = NULL ;
         _releaseCheckFunc = NULL ;
         _releaseFunc = NULL ;
         _onLockReleaseFunc = NULL ;
      }
   } ;

   // Lock Request Block Header ( LRB Header )
   class dpsTransLRBHeader : public utilPooledObject
   {
   public :
      dpsTransLRBHeader * nextLRBHdr ;   // next LRB Header in the chain
      dpsTransLRB       * ownerLRB ;     // the first owner LRB in its chain
      dpsTransLRB       * waiterLRB ;    // the first waiter LRB in its chain
      dpsTransLRB       * upgradeLRB;    // the first upgrader LRB in its chain
      dpsTransLRB       * newestIXOwner ;// the newest IX LRB in owner list
      dpsTransLRB       * newestISOwner ;// the newest IS LRB in owner list
      dpsTransLockId      lockId ;       // lockId, 16 bytes
      UINT32              bktIdx ;       // hash bucket index
      /// user data
      dpsLRBExtData extData ;

   public :
      dpsTransLRBHeader( dpsTransLockId lock, UINT32 _bktIdx )
      : nextLRBHdr( NULL ),
        ownerLRB( NULL ),
        waiterLRB( NULL ),
        upgradeLRB( NULL ),
        newestIXOwner( NULL ),
        newestISOwner( NULL ),
        lockId( lock ),
        bktIdx( _bktIdx )
      {
      }

      ~dpsTransLRBHeader()
      {
         // if extData hasn't been setup, isValid() will return FALSE
         // and canRelease() will return TRUE
         if ( extData.isValid() )
         {
            /// release check
            SDB_ASSERT( extData.canRelease(),
                        "Extend data can't be released" ) ;

            /// release
            if ( !extData.release() )
            {
               PD_LOG( PDWARNING,
                       "Extend data[%llu] doesn't clear in LRBHdr[%s]",
                       extData._data,
                       lockId.toString().c_str() ) ;
            }
         }
      }

      void reset()
      {
         nextLRBHdr    = NULL ;
         ownerLRB      = NULL ;
         waiterLRB     = NULL ;
         upgradeLRB    = NULL ;
         newestIXOwner = NULL ;
         newestISOwner = NULL ;
         bktIdx        = ( (UINT32) -1 ) ;
         lockId.reset() ;
         extData.reset() ;
      }
   } ;

   class dpsTransLRBHeaderHash : public SDBObject
   {
   public :
      dpsTransLRBHeader *lrbHdr  ;     // 1st LRB Header in the chain
      ossSpinXLatch     hashHdrLatch ; // ossSpinXLatch, 48 bytes
   public :
      dpsTransLRBHeaderHash()
      : lrbHdr(NULL)
      {
      }
   } ; // 56 bytes in total


   /*
    * trans executor info for deadlock detection
    */
   class dpsTxWaitLRB : public utilPooledObject
   {
   public:
       void reset()
       {
          eduID  = PMD_INVALID_EDUID ;
          pLRB   = NULL ;
          lockId.reset() ;
       }

       dpsTxWaitLRB()
       {
          reset();
       }

       dpsTxWaitLRB( const dpsTxWaitLRB & rhs )
       {
          eduID  = rhs.eduID ;
          pLRB   = rhs.pLRB ;
          lockId = rhs.lockId ;
       }

       dpsTxWaitLRB( EDUID                  eduId,
                     dpsTransLRB          * plrb,
                     const dpsTransLockId & lockID )
       {
          eduID  = eduId ;
          pLRB   = plrb ;
          lockId = lockID ;
       }

       virtual ~dpsTxWaitLRB() { }

       dpsTxWaitLRB & operator= ( const dpsTxWaitLRB & rhs )
       {
          eduID  = rhs.eduID ;
          pLRB   = rhs.pLRB ;
          lockId = rhs.lockId ;
          return *this ;
       }

       BOOLEAN operator== ( const dpsTxWaitLRB &rhs ) const
       {
          return ( eduID == rhs.eduID ) ;
       }

       BOOLEAN operator< ( const dpsTxWaitLRB &rhs ) const
       {
          return ( eduID < rhs.eduID ) ;
       }

   public:
       EDUID          eduID;
       dpsTransLRB  * pLRB ;
       dpsTransLockId lockId ;
   } ;
   typedef ossPoolSet< dpsTxWaitLRB >     DPS_TX_WAIT_LRB_SET ;
   typedef DPS_TX_WAIT_LRB_SET::iterator  DPS_TX_WAIT_LRB_SET_IT ;

}

#endif // DPSTRANSLRB_HPP_

