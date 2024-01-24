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

   Source File Name = rtnScanner.hpp

   Descriptive Name = RunTime Scanner Header

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_SCANNER_HPP__
#define RTN_SCANNER_HPP__

#include "oss.hpp"
#include "dms.hpp"
#include "utilPooledObject.hpp"
#include "ossMemPool.hpp"
#include "interface/IStorageSession.hpp"

namespace engine
{

   // forward declaration
   class _dmsStorageUnit ;
   class _dmsMBContext ;
   class _pmdEDUCB ;

   // define type of scanners
   enum rtnScannerType
   {
      SCANNER_TYPE_DISK      = 0,
      SCANNER_TYPE_MEM_TREE,
      SCANNER_TYPE_MERGE
   } ;

   enum rtnScannerStorageType
   {
      SCANNER_TYPE_DATA = 0,
      SCANNER_TYPE_INDEX,
   } ;

   /*
      RTN_SUB_SCAN_TYPE define
   */
   enum RTN_SUB_SCAN_TYPE
   {
      SCAN_NONE,
      SCAN_LEFT,
      SCAN_RIGHT
   } ;

   typedef ossPoolSet<dmsRecordID> SET_RECORDID ;

   /*
      _rtnScanner define
    */
   class _rtnScanner : public _utilPooledObject
   {
   public:
      _rtnScanner( _dmsStorageUnit  *su,
                   _dmsMBContext    *mbContext,
                   INT32             direction,
                   BOOLEAN           isAsync,
                   _pmdEDUCB        *cb )
      : _su( su ),
        _mbContext( mbContext ),
        _direction( direction ),
        _isAsync( isAsync ),
        _isEOF( FALSE ),
        _cb( cb )
      {
      }

      virtual ~_rtnScanner()
      {
      }

   public:
      BOOLEAN isEOF() const
      {
         return _isEOF ;
      }

      virtual INT32 init() = 0 ;
      virtual INT32 advance( dmsRecordID &rid ) = 0 ;
      virtual INT32 resumeScan( BOOLEAN &isCursorSame ) = 0 ;
      virtual INT32 pauseScan() = 0 ;

      virtual INT32 checkSnapshotID( BOOLEAN &isCursorSame ) = 0 ;
      virtual BOOLEAN removeDuplicatRID( const dmsRecordID &rid ) = 0 ;
      virtual dmsExtentID getIdxLID() const = 0 ;
      /*
         return : -1, SHARED or EXCLUSIVE
      */
      virtual INT32 getIdxLockModeByType( rtnScannerType type ) const = 0 ;

      virtual rtnScannerStorageType getStorageType() const = 0 ;
      virtual rtnScannerType  getType() const = 0 ;
      virtual rtnScannerType  getCurScanType() const = 0 ;
      virtual void            disableByType( rtnScannerType type ) = 0 ;
      virtual BOOLEAN         isTypeEnabled( rtnScannerType type ) const = 0 ;
      virtual BOOLEAN         canPrefetch() const = 0 ;
      virtual IStorageSession *getSession() = 0 ;

      _pmdEDUCB *getEDUCB()
      {
         return _cb ;
      }

   protected:
      _dmsStorageUnit *_su ;
      _dmsMBContext *_mbContext ;
      INT32 _direction ;
      BOOLEAN _isAsync ;
      BOOLEAN _isEOF ;
      _pmdEDUCB *_cb ;
   } ;

   typedef class _rtnScanner rtnScanner ;

}

#endif // RTN_SCANNER_HPP__