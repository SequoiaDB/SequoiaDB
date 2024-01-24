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

   Source File Name = rtnMemTBScanner.hpp

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

#ifndef RTN_MEM_TB_SCANNER_HPP__
#define RTN_MEM_TB_SCANNER_HPP__

#include "rtnTBScanner.hpp"
#include "dpsTransVersionCtrl.hpp"

namespace engine
{

   /*
      _rtnMemTBScanner define
    */
   class _rtnMemTBScanner : public _rtnTBScanner
   {
   public:
      _rtnMemTBScanner( _dmsStorageUnit  *su,
                        _dmsMBContext    *mbContext,
                        const dmsRecordID &startRID,
                        BOOLEAN           isAfterStartRID,
                        INT32             direction,
                        _pmdEDUCB        *cb ) ;
      virtual ~_rtnMemTBScanner() ;

   public:
      // for _rtnScanner
      virtual INT32 advance( dmsRecordID &rid ) ;
      virtual INT32 resumeScan( BOOLEAN &isCursorSame ) ;
      virtual INT32 pauseScan() ;
      virtual INT32 checkSnapshotID( BOOLEAN &isCursorSame ) ;

      virtual rtnScannerType getType() const
      {
         return SCANNER_TYPE_MEM_TREE ;
      }

      virtual rtnScannerType getCurScanType() const
      {
         return SCANNER_TYPE_MEM_TREE ;
      }

      virtual void disableByType( rtnScannerType type )
      {
      }

      virtual BOOLEAN isTypeEnabled( rtnScannerType type ) const
      {
         return SCANNER_TYPE_MEM_TREE == type ;
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
         return nullptr ;
      }

   protected:
      INT32 _firstInit() ;
      INT32 _relocateRID( const dmsRecordID &rid, BOOLEAN &isFound ) ;
      INT32 _initOldVerUnit() ;

   protected:
      dpsTransCB * _transCB ;
      dmsRecordID _curRID ;
      oldVersionUnitPtr _oldVerUnit ;
   } ;

   typedef class _rtnMemTBScanner rtnMemTBScanner ;

}

#endif // RTN_MEM_TB_SCANNER_HPP__