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

   Source File Name = rtnTBScanner.hpp

   Descriptive Name = RunTime Table Scanner Header

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_TB_SCANNER_HPP__
#define RTN_TB_SCANNER_HPP__

#include "rtnScanner.hpp"
#include "interface/ICursor.hpp"

namespace engine
{

   /*
      _rtnTBScanner define
    */
   class _rtnTBScanner : public _rtnScanner
   {
   public:
      _rtnTBScanner( _dmsStorageUnit  *su,
                     _dmsMBContext    *mbContext,
                     const dmsRecordID &startRID,
                     BOOLEAN           isAfterStartRID,
                     INT32             direction,
                     BOOLEAN           isAsync,
                     _pmdEDUCB        *cb )
      : _rtnScanner( su, mbContext, direction, isAsync, cb ),
        _init( FALSE ),
        _startRID( startRID ),
        _isAfterStartRID( isAfterStartRID )
      {
      }

      virtual ~_rtnTBScanner() = default ;

      virtual rtnScannerStorageType getStorageType() const
      {
         return SCANNER_TYPE_DATA ;
      }

      virtual INT32 getIdxLockModeByType( rtnScannerType type ) const
      {
         return -1 ;
      }

      virtual BOOLEAN removeDuplicatRID( const dmsRecordID &rid )
      {
         return TRUE ;
      }

      virtual dmsExtentID getIdxLID() const
      {
         return DMS_INVALID_EXTENT ;
      }

      virtual INT32 init()
      {
         return SDB_OK ;
      }

   public:
      BOOLEAN isEOF() const
      {
         return _isEOF ;
      }

      virtual INT32 getCurrentRID( dmsRecordID &nextRID ) = 0 ;
      virtual INT32 getCurrentRecord( dmsRecordData &recordData ) = 0 ;

      virtual INT32 relocateRID( const dmsRecordID &rid ) = 0 ;
      virtual INT32 relocateRID( const dmsRecordID &rid, BOOLEAN &isFound ) = 0 ;

      const dmsRecordID &getSavedRID() const
      {
         return _savedRID ;
      }

      void resetSavedRID()
      {
         _savedRID.reset() ;
         _relocatedRID.reset() ;
      }

      BOOLEAN isInit() const
      {
         return _init ;
      }

   protected:
      BOOLEAN _init ;
      dmsRecordID _startRID ;
      BOOLEAN _isAfterStartRID ;
      dmsRecordID _savedRID ;
      dmsRecordID _relocatedRID ;
   } ;

   typedef class _rtnTBScanner rtnTBScanner ;

}

#endif // RTN_TB_SCANNER_HPP__
