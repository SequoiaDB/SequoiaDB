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

   Source File Name = catDCLogMgr.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =     XJH Opt

*******************************************************************************/
#ifndef CAT_DCLOGMGR_HPP__
#define CAT_DCLOGMGR_HPP__

#include "pmd.hpp"
#include "netDef.hpp"
#include "dpsLogWrapper.hpp"
#include "pmdEDU.hpp"
#include <vector>
#include <string>

using namespace bson ;
using namespace std ;

namespace engine
{
   class sdbCatalogueCB ;
   class _SDB_RTNCB ;
   class _SDB_DMSCB ;

   /*
      _catDCLogItem define
   */
   class _catDCLogItem : public SDBObject
   {
      public:
         _catDCLogItem( UINT32 pos, const string &clname ) ;
         ~_catDCLogItem() ;

         string         toString() const ;

         const CHAR*    getCLName() const { return _clName.c_str() ; }
         UINT32         getPos() const { return _pos ; }
         UINT64         getCount() const ;
         DPS_LSN        getFirstLSN() const ;
         DPS_LSN        getLastLSN() const ;
         DPS_LSN        getComingLSN() const ;
         UINT32         getLID() const ;

         BOOLEAN        isFull() const ;
         BOOLEAN        isEmpty() const ;

         INT32          truncate( _pmdEDUCB *cb ) ;
         INT32          restore( _pmdEDUCB *cb ) ;
         INT32          writeData( BSONObj &obj, const DPS_LSN &lsn,
                                   _pmdEDUCB *cb ) ;
         INT32          readData( const BSONObj &match,
                                  _dpsMessageBlock *mb,
                                  _pmdEDUCB *cb,
                                  const BSONObj &orderby = BSONObj(),
                                  INT64 limit = 1,
                                  INT32 maxTime = -1,
                                  INT32 maxSize = 5242880 ) ;
         INT32          removeDataToLow( DPS_LSN_OFFSET lowOffset,
                                         _pmdEDUCB *cb ) ;

      protected:
         void           _reset() ;
         INT32          _parseLsn( const BSONObj &orderby,
                                   _pmdEDUCB *cb,
                                   DPS_LSN &lsn ) ;
         INT32          _parseMeta( _pmdEDUCB *cb ) ;

      private:
         _SDB_DMSCB                 *_pDmsCB;
         _dpsLogWrapper             *_pDpsCB;
         _SDB_RTNCB                 *_pRtnCB;
         sdbCatalogueCB             *_pCatCB;

         string                     _clName ;
         UINT32                     _pos ;
         UINT32                     _clLID ;

         UINT64                     _count ;
         DPS_LSN                    _first ;
         DPS_LSN                    _last ;
         DPS_LSN                    _coming ;

   } ;
   typedef _catDCLogItem catDCLogItem ;

   /*
      _catDCLogMgr define
   */
   class _catDCLogMgr : public SDBObject, public ILogAccessor
   {
      public:
         _catDCLogMgr() ;
         ~_catDCLogMgr() ;

         void  attachCB( _pmdEDUCB *cb ) ;
         void  detachCB( _pmdEDUCB *cb ) ;

         INT32 init() ;
         INT32 restore() ;

         INT32 saveSysLog( dpsLogRecordHeader *pHeader,
                           DPS_LSN *pRetLSN = NULL ) ;

      public:
         virtual INT32     search( const DPS_LSN &minLsn,
                                   _dpsMessageBlock *mb,
                                   UINT8 type = DPS_SEARCH_ALL,
                                   INT32 maxNum = 1,
                                   INT32 maxTime = -1,
                                   INT32 maxSize = 5242880 ) ;

         virtual INT32     searchHeader( const DPS_LSN &lsn,
                                         _dpsMessageBlock *mb,
                                         UINT8 type = DPS_SEARCH_ALL ) ;

         virtual DPS_LSN   getStartLsn ( BOOLEAN logBufOnly = FALSE ) ;

         virtual DPS_LSN   getCurrentLsn() ;
         virtual DPS_LSN   expectLsn() ;
         virtual DPS_LSN   commitLsn() ;

         virtual void      getLsnWindow( DPS_LSN &beginLsn,
                                         DPS_LSN &memBeginLsn,
                                         DPS_LSN &endLsn,
                                         DPS_LSN *pExpectLsn,
                                         DPS_LSN *committed ) ;

         virtual void      getLsnWindow( DPS_LSN &beginLsn,
                                         DPS_LSN &endLsn,
                                         DPS_LSN *pExpectLsn,
                                         DPS_LSN *committed ) ;

         virtual INT32     move( const DPS_LSN_OFFSET &offset,
                                 const DPS_LSN_VER &version ) ;

         virtual INT32     recordRow( const CHAR *row, UINT32 len ) ;

      protected:
         UINT32 _incFileID( UINT32 fileID ) ;
         UINT32 _decFileID( UINT32 fileID ) ;
         void   _setVersion( DPS_LSN_VER version ) ;

         // caller must hold the _latch
         INT32  _writeData( BSONObj &obj, const DPS_LSN &lsn ) ;
         INT32  _readData( const BSONObj &match,
                           catDCLogItem *pLog,
                           _dpsMessageBlock *mb,
                           const BSONObj &orderby = BSONObj(),
                           INT64 limit = 1,
                           INT32 maxTime = -1,
                           INT32 maxSize = 5242880 ) ;

         DPS_LSN   _getStartLsn() ;

         INT32     _filterLog( dpsLogRecordHeader *pHeader,
                               BOOLEAN &valid ) ;

      private:
         _pmdEDUCB                  *_pEduCB;
         vector< catDCLogItem* >    _vecLogCL ;

         UINT32                     _begin ;
         UINT32                     _work ;

         DPS_LSN                    _curLsn ;
         DPS_LSN                    _expectLsn ;
         ossSpinSLatch              _latch ;

   } ;
   typedef _catDCLogMgr catDCLogMgr ;


}

#endif // CAT_DCLOGMGR_HPP__

