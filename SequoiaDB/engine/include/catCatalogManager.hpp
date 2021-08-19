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
#include "catSplit.hpp"
#include "rtnContextBuff.hpp"
#include "utilCompressor.hpp"
#include "utilArguments.hpp"

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

      _catCollectionInfo()
      {
         reset() ;
      }

      void reset()
      {
         _pCLName             = NULL ;
         _clUniqueID          = UTIL_UNIQUEID_NULL ;
         _replSize            = 1 ;
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
         _autoIncSet.clear() ;
      }
   };
   typedef _catCollectionInfo catCollectionInfo ;

   struct _catCSInfo
   {
      const CHAR*       _pCSName ;
      utilCSUniqueID    _csUniqueID ;
      utilCLUniqueID    _clUniqueHWM ;
      INT32             _pageSize ;
      const CHAR*       _domainName ;
      INT32             _lobPageSize ;
      DMS_STORAGE_TYPE  _type ;

      _catCSInfo()
      {
         reset() ;
      }

      void reset()
      {
         _pCSName = NULL ;
         _csUniqueID = UTIL_UNIQUEID_NULL ;
         _clUniqueHWM = UTIL_UNIQUEID_NULL ;
         _pageSize = DMS_PAGE_SIZE_DFT ;
         _domainName = NULL ;
         _lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
         _type = DMS_STORAGE_NORMAL ;
      }

      BSONObj toBson()
      {
         BSONObjBuilder builder ;
         builder.append( CAT_COLLECTION_SPACE_NAME, _pCSName ) ;
         builder.append( CAT_CS_UNIQUEID, _csUniqueID ) ;
         builder.append( CAT_CS_CLUNIQUEHWM, (INT64)_clUniqueHWM ) ;
         builder.append( CAT_PAGE_SIZE_NAME, _pageSize ) ;
         if ( _domainName )
         {
            builder.append( CAT_DOMAIN_NAME, _domainName ) ;
         }
         builder.append( CAT_LOB_PAGE_SZ_NAME, _lobPageSize ) ;
         builder.append( CAT_TYPE_NAME, _type ) ;
         return builder.obj() ;
      }
   } ;
   typedef _catCSInfo catCSInfo ;

   /*
      catCatalogueManager define
   */
   class catCatalogueManager : public SDBObject
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

   // message process functions
   protected:
      INT32 processCommandMsg( const NET_HANDLE &handle, MsgHeader *pMsg,
                               BOOLEAN writable ) ;

      INT32 processCmdCreateCS( const CHAR *pQuery,
                                rtnContextBuf &ctxBuf ) ;
      INT32 processCmdSplit( const CHAR *pQuery,
                             INT32 opCode,
                             rtnContextBuf &ctxBuf ) ;
      INT32 processCmdQuerySpaceInfo( const CHAR *pQuery,
                                      rtnContextBuf &ctxBuf ) ;
      INT32 processQueryCatalogue ( const NET_HANDLE &handle,
                                    MsgHeader *pMsg ) ;
      INT32 processQueryTask ( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 processCmdCrtProcedures( void *pMsg ) ;
      INT32 processCmdRmProcedures( void *pMsg ) ;
      INT32 processCmdCreateDomain ( const CHAR *pQuery ) ;
      INT32 processCmdDropDomain ( const CHAR *pQuery ) ;
      INT32 processCmdAlterDomain ( const CHAR *pQuery ) ;
      INT32 processCmdTruncate ( const CHAR *pQuery ) ;

   // tool functions
   protected:
      void  _fillRspHeader( MsgHeader *rspMsg, const MsgHeader *reqMsg ) ;

      INT32 _createCS( BSONObj & createObj, UINT32 &groupID ) ;

      INT32 _checkAndGetCSInfo( const BSONObj &infoObj, catCSInfo &csInfo ) ;

      INT32 _assignGroup( vector< UINT32 > *pGoups, UINT32 &groupID ) ;

      INT32 _checkAllCSCLUniqueID() ;
   private:
      INT16 _majoritySize() ;
      INT32 _setCSCLUniqueID( string csName, const BSONObj& boCollections,
                              UINT32 csUniqueID ) ;

   private:
      sdbCatalogueCB       *_pCatCB;
      _SDB_DMSCB           *_pDmsCB;
      _dpsLogWrapper       *_pDpsCB;
      pmdEDUCB             *_pEduCB;
      clsTaskMgr           _taskMgr ;

   } ;
}

#endif // CATCATALOGUEMANAGER_HPP_

