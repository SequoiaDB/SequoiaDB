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

   Source File Name = catCatalogManager.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#ifndef CATCATALOGUEMANAGER_HPP_
#define CATCATALOGUEMANAGER_HPP_

#include "pmd.hpp"
#include "catTask.hpp"
#include "catEventHandler.hpp"
#include "rtnContextBuff.hpp"
#include "utilCompressor.hpp"
#include "utilArguments.hpp"
#include "utilUniqueID.hpp"

using namespace bson ;

namespace engine
{

   class _dpsLogWrapper ;
   class sdbCatalogueCB ;
   class _SDB_DMSCB ;
   class _rtnAlterJob ;

   // create collection assign group type
   enum CAT_ASSIGNGROUP_TYPE
   {
      ASSIGN_FOLLOW     = 1,
      ASSIGN_RANDOM     = 2
   } ;

   struct _catCollectionInfo
   {
      const CHAR  *_pCLName ;
      utilCLUniqueID _clUniqueID ;
      BSONObj     _shardingKey ;
      const CHAR  *_lobShardingKeyFormat ;
      INT32       _replSize ;
      SDB_CONSISTENCY_STRATEGY _consistencyStrategy ;
      BOOLEAN     _enSureShardIndex ;
      const CHAR  *_pShardingType ;
      INT32       _shardPartition ;
      BOOLEAN     _isHash ;
      BOOLEAN     _isSharding ;
      BOOLEAN     _isCompressed ;
      BOOLEAN     _isMainCL;
      BOOLEAN     _autoSplit ;
      BOOLEAN     _autoRebalance ;
      BOOLEAN     _strictDataMode ;
      BOOLEAN     _noTrans ;
      const CHAR * _gpSpecified ;
      INT32       _version ;
      INT32       _assignType ;
      BOOLEAN     _autoIndexId ;
      UTIL_COMPRESSOR_TYPE _compressorType ;
      BOOLEAN     _capped ;
      INT64       _maxRecNum ;
      INT64       _maxSize ;
      BOOLEAN     _overwrite ;
      BSONObj     _autoIncFields ;
      clsAutoIncSet _autoIncSet ;
      UTIL_DS_UID _dsUID ;
      CHAR        _fullMapping[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;

      _catCollectionInfo()
      {
         reset() ;
      }

      void reset()
      {
         _pCLName             = NULL ;
         _clUniqueID          = UTIL_UNIQUEID_NULL ;
         _replSize            = 1 ;
         _consistencyStrategy = SDB_CONSISTENCY_PRY_LOC_MAJOR ;
         _enSureShardIndex    = TRUE ;
         _pShardingType       = CAT_SHARDING_TYPE_HASH ;
         _shardPartition      = CAT_SHARDING_PARTITION_DEFAULT ;
         _isHash              = TRUE ;
         _isSharding          = FALSE ;
         _isCompressed        = FALSE ;
         _isMainCL            = FALSE ;
         _autoSplit           = FALSE ;
         _autoRebalance       = FALSE ;
         _strictDataMode      = FALSE ;
         _noTrans             = FALSE ;
         _gpSpecified         = NULL ;
         _version             = 0 ;
         _assignType          = ASSIGN_RANDOM ;
         _autoIndexId         = TRUE ;
         _compressorType      = UTIL_COMPRESSOR_INVALID ;
         _capped              = FALSE ;
         _maxRecNum           = 0 ;
         _maxSize             = 0 ;
         _overwrite           = FALSE ;
         _lobShardingKeyFormat = NULL ;
         _dsUID               = UTIL_INVALID_DS_UID ;
         _autoIncSet.clear() ;
         ossMemset( _fullMapping, 0, DMS_COLLECTION_FULL_NAME_SZ + 1 ) ;
      }
   };
   typedef _catCollectionInfo catCollectionInfo ;

   /*
      catCatalogueManager define
   */
   class catCatalogueManager : public SDBObject,
                               public _catEventHandler
   {
   public:
      catCatalogueManager() ;

      INT32 init() ;
      INT32 fini() ;

      void  attachCB( _pmdEDUCB *cb ) ;
      void  detachCB( _pmdEDUCB *cb ) ;

      INT32 processMsg( const NET_HANDLE &handle, MsgHeader *pMsg ) ;

      INT32 active() ;
      INT32 deactive() ;

      UINT64 assignTaskID () ;

      virtual const CHAR *getHandlerName() { return "catCatalogueManager" ; }
      virtual INT32 onUpgrade( UINT32 version ) ;

   // message process functions
   protected:
      INT32 processCommandMsg( const NET_HANDLE &handle, MsgHeader *pMsg,
                               BOOLEAN writable ) ;

      INT32 processCmdCreateCS( const CHAR *pQuery,
                                rtnContextBuf &ctxBuf ) ;
      INT32 processCmdCreateIndex( const CHAR *pQuery,
                                   const CHAR *pHint,
                                   rtnContextBuf &ctxBuf ) ;
      INT32 processCmdDropIndex( const CHAR *pQuery,
                                 const CHAR *pHint,
                                 rtnContextBuf &ctxBuf ) ;
      INT32 processCmdTask( const CHAR *pQuery,
                            INT32 opCode,
                            rtnContextBuf &ctxBuf ) ;
      INT32 processCmdQuerySpaceInfo( const CHAR *pQuery,
                                      rtnContextBuf &ctxBuf ) ;
      INT32 processQueryCatalogue ( const NET_HANDLE &handle,
                                    MsgHeader *pMsg ) ;
      INT32 processQueryTask ( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 processCmdCrtProcedures( const CHAR *pMsg ) ;
      INT32 processCmdRmProcedures( const CHAR *pMsg ) ;
      INT32 processCmdCreateDomain ( const CHAR *pQuery ) ;
      INT32 processCmdDropDomain ( const CHAR *pQuery ) ;
      INT32 processCmdAlterDomain ( const CHAR *pQuery, rtnContextBuf &ctxBuf ) ;

   // tool functions
   protected:
      void  _fillRspHeader( MsgHeader *rspMsg, const MsgHeader *reqMsg ) ;

      INT32 _checkTaskHWM() ;

      INT32 _checkAllCSCLUniqueID() ;

      /**
       * Check and upgrade data source and collection information. It will only
       * happen when upgrading from sequoiadb 3.2.8 to newer versions, and if
       * data source is used.
       */
      INT32 _checkAndUpgradeDSCLInfo() ;

      INT32 _checkAndUpgradeUserRole() ;

   private:
      INT16 _majoritySize() ;

      INT32 _setCSCLUniqueID( string csName, const BSONObj& boCollections,
                              UINT32 csUniqueID ) ;

      INT32 _checkPureMappingCS( const CHAR *clFullName, MsgOpReply *&reply ) ;

   private:
      sdbCatalogueCB       *_pCatCB;
      _SDB_DMSCB           *_pDmsCB;
      _dpsLogWrapper       *_pDpsCB;
      pmdEDUCB             *_pEduCB;
   } ;
}

#endif // CATCATALOGUEMANAGER_HPP_

