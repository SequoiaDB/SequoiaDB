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

using namespace bson ;

namespace engine
{

   class _dpsLogWrapper ;
   class sdbCatalogueCB ;
   class _SDB_DMSCB ;
   class _rtnAlterJob ;

   enum CAT_ASSIGNGROUP_TYPE
   {
      ASSIGN_FOLLOW     = 1,
      ASSIGN_RANDOM     = 2
   } ;

   #define CAT_MASK_CLNAME          0x00000001
   #define CAT_MASK_SHDKEY          0x00000002
   #define CAT_MASK_REPLSIZE        0x00000004
   #define CAT_MASK_SHDIDX          0x00000008
   #define CAT_MASK_SHDTYPE         0x00000010
   #define CAT_MASK_SHDPARTITION    0x00000020
   #define CAT_MASK_COMPRESSED      0x00000040
   #define CAT_MASK_ISMAINCL        0x00000080
   #define CAT_MASK_AUTOASPLIT      0x00000100
   #define CAT_MASK_AUTOREBALAN     0x00000200
   #define CAT_MASK_AUTOINDEXID     0x00000400
   #define CAT_MASK_COMPRESSIONTYPE 0x00000800
   #define CAT_MASK_CAPPED          0x00001000
   #define CAT_MASK_CLMAXRECNUM     0x00002000
   #define CAT_MASK_CLMAXSIZE       0x00004000
   #define CAT_MASK_CLOVERWRITE     0x00008000
   #define CAT_MASK_STRICTDATAMODE  0x00010000

   struct _catCollectionInfo
   {
      const CHAR  *_pCLName ;
      BSONObj     _shardingKey ;
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

      std::vector<std::string>   _subCLList;

      _catCollectionInfo()
      {
         _pCLName             = NULL ;
         _replSize            = 1 ;
         _enSureShardIndex    = TRUE ;
         _pShardingType       = CAT_SHARDING_TYPE_HASH ;
         _shardPartition      = CAT_SHARDING_PARTITION_DEFAULT ;
         _isHash              = FALSE ;
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
      }
   };
   typedef _catCollectionInfo catCollectionInfo ;

   struct _catCSInfo
   {
      const CHAR  *_pCSName ;
      INT32       _pageSize ;
      const CHAR  *_domainName ;
      INT32       _lobPageSize ;
      DMS_STORAGE_TYPE _type ;

      _catCSInfo()
      {
         _pCSName = NULL ;
         _pageSize = DMS_PAGE_SIZE_DFT ;
         _domainName = NULL ;
         _lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
         _type = DMS_STORAGE_NORMAL ;
      }

      BSONObj toBson()
      {
         BSONObjBuilder builder ;
         builder.append( CAT_COLLECTION_SPACE_NAME, _pCSName ) ;
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

   protected:
      void  _fillRspHeader( MsgHeader *rspMsg, const MsgHeader *reqMsg ) ;

      INT32 _createCS( BSONObj & createObj, UINT32 &groupID ) ;

      INT32 _checkCSObj( const BSONObj &infoObj,
                         catCSInfo &csInfo ) ;

      INT32 _assignGroup( vector< UINT32 > *pGoups, UINT32 &groupID ) ;

   private:
      INT16 _majoritySize() ;

      INT32 _buildAlterGroups( const BSONObj &domain,
                               const BSONElement &ele,
                               BSONObjBuilder &builder ) ;

   private:
      sdbCatalogueCB       *_pCatCB;
      _SDB_DMSCB           *_pDmsCB;
      _dpsLogWrapper       *_pDpsCB;
      pmdEDUCB             *_pEduCB;
      clsTaskMgr           _taskMgr ;

   } ;
}

#endif // CATCATALOGUEMANAGER_HPP_

