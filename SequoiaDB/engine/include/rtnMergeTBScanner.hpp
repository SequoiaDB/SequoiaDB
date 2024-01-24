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

   Source File Name = rtnMergeTBScanner.hpp

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

#ifndef RTN_MERGE_TB_SCANNER_HPP__
#define RTN_MERGE_TB_SCANNER_HPP__

#include "rtnTBScanner.hpp"
#include "interface/ICursor.hpp"

namespace engine
{

   /*
      _rtnMergeTBScanner define
    */
   class _rtnMergeTBScanner : public _rtnTBScanner
   {
   public:
      _rtnMergeTBScanner( _dmsStorageUnit  *su,
                          _dmsMBContext    *mbContext,
                          const dmsRecordID &startRID,
                          BOOLEAN           isAfterStartRID,
                          INT32             direction,
                          _pmdEDUCB        *cb ) ;
      virtual ~_rtnMergeTBScanner() ;

   public:
      // for _rtnScanner
      virtual INT32 init() ;
      virtual INT32 advance( dmsRecordID &rid ) ;
      virtual INT32 resumeScan( BOOLEAN &isCursorSame ) ;
      virtual INT32 pauseScan() ;
      virtual INT32 checkSnapshotID( BOOLEAN &isCursorSame ) ;

      virtual rtnScannerType getType() const
      {
         return SCANNER_TYPE_MERGE ;
      }

      virtual rtnScannerType getCurScanType() const
      {
         SDB_ASSERT( SCAN_NONE != _fromDir, "Invalid scann from" ) ;
         return ( SCAN_LEFT == _fromDir ) ? _leftType : _rightType ;
      }

      virtual void disableByType( rtnScannerType type )
      {
         if ( _leftType == type )
         {
            _leftEnabled = FALSE ;
         }
         else if ( _rightType == type )
         {
            _rightEnabled = FALSE ;
         }
      }

      virtual BOOLEAN isTypeEnabled( rtnScannerType type ) const
      {
         BOOLEAN isEnabled = FALSE ;
         if ( _leftType == type )
         {
            isEnabled = _leftEnabled ;
         }
         else if ( _rightType == type )
         {
            isEnabled = _rightEnabled ;
         }
         return isEnabled ;
      }

      // for _rtnTBScanner
      virtual INT32 getCurrentRID( dmsRecordID &nextRID ) ;
      virtual INT32 getCurrentRecord( dmsRecordData &recordData ) ;

      virtual INT32 relocateRID( const dmsRecordID &rid ) ;

      virtual INT32 relocateRID( const dmsRecordID &rid, BOOLEAN &isFound )
      {
         return _relocateRID( rid, isFound ) ;
      }

      virtual BOOLEAN canPrefetch() const
      {
         return FALSE ;
      }

      virtual IStorageSession *getSession()
      {
         IStorageSession *lSess = _leftEnabled ? _leftTBScanner->getSession() : nullptr ;
         if ( lSess )
         {
            return lSess ;
         }
         return _rightEnabled ? _rightTBScanner->getSession() : nullptr ;
      }

   protected:
      INT32 _firstInit() ;
      INT32 _advance() ;
      INT32 _relocateRID( const dmsRecordID &rid, BOOLEAN &isFound ) ;

      INT32 _createScanner( rtnScannerType type, rtnTBScanner *&scanner ) ;

      const dmsRecordID &_getSavedRIDFromChild() const ;

      INT32 _chooseFromDir( BOOLEAN isLeftEOF,
                            BOOLEAN isRightEOF,
                            BOOLEAN isLeftFound,
                            BOOLEAN isRightFound,
                            const dmsRecordID &leftRID,
                            const dmsRecordID &rightRID,
                            INT32 &fromDir ) ;

   protected:
      INT32 _fromDir ;
      INT32 _savedDir ;
      rtnTBScanner *_leftTBScanner ;
      rtnTBScanner *_rightTBScanner ;
      rtnScannerType _leftType ;
      rtnScannerType _rightType ;
      BOOLEAN        _leftEnabled ;
      BOOLEAN        _rightEnabled ;
   } ;

   typedef class _rtnMergeTBScanner rtnMergeTBScanner ;

}

#endif // RTN_MERGE_TB_SCANNER_HPP__