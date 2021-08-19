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

   Source File Name = clsCatalogAgent.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsCatalogAgent.hpp"
#include "msgCatalog.hpp"
#include "ossUtil.hpp"
#include "clsShardMgr.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "clsCatalogMatcher.hpp"
#include "catDef.hpp"
#include "utilBsonHash.hpp"
#include "utilCommon.hpp"
#include "ossMemPool.hpp"
#include "utilLobID.hpp"
#include "mthCommon.hpp"

#include "../bson/lib/md5.hpp"
#include "../bson/lib/md5.h"


using namespace bson ;

namespace engine
{
   #define CLS_CA_SHARDINGTYPE_NONE       ( 0 )
   #define CLS_CA_SHARDINGTYPE_RANGE      ( 1 )
   #define CLS_CA_SHARDINGTYPE_HASH       ( 2 )

   #define CLS_CATSET_GROUPSZ_DFT         ( 8 )
   #define CLS_CATSET_SUBCLSZ_DFT         ( 8 )

   /*
   note: _clsCataItemKey implement
   */
   _clsCataItemKey::_clsCataItemKey ( const CHAR * pUpBoundData,
                                      const Ordering * ordering )
   {
      _keyType = _clsCataItemKey::CLS_KEY_BSON ;
      _keyData._pUpBoundData = pUpBoundData ;
      _ordering = ordering ;
   }

   _clsCataItemKey::_clsCataItemKey( INT32 number )
   {
      _keyType = _clsCataItemKey::CLS_KEY_NUM ;
      _keyData._number = number ;
      _ordering = NULL ;
   }

   _clsCataItemKey::_clsCataItemKey ( const _clsCataItemKey &right )
   {
      _keyType = right._keyType ;
      if ( _clsCataItemKey::CLS_KEY_BSON == _keyType )
      {
         _keyData._pUpBoundData = right._keyData._pUpBoundData ;
      }
      else
      {
         _keyData._number = right._keyData._number ;
      }
      _ordering = right._ordering ;
   }

   _clsCataItemKey::~_clsCataItemKey ()
   {
      _ordering = NULL ;
   }

   bool _clsCataItemKey::operator< ( const _clsCataItemKey &right ) const
   {
      if ( _clsCataItemKey::CLS_KEY_NUM == _keyType )
      {
         return _keyData._number < right._keyData._number ? true : false ;
      }
      else
      {
         BSONObj meUpBound ( _keyData._pUpBoundData ) ;
         BSONObj rightUpBound ( right._keyData._pUpBoundData ) ;
         INT32 compare = 0 ;

         if ( _ordering )
         {
            compare = meUpBound.woCompare( rightUpBound, *_ordering, false ) ;
         }
         else
         {
            compare = meUpBound.woCompare( rightUpBound, BSONObj(), false ) ;
         }
         return compare < 0 ? true : false ;
      }
   }

   bool _clsCataItemKey::operator<= ( const _clsCataItemKey &right ) const
   {
      if ( _clsCataItemKey::CLS_KEY_NUM == _keyType )
      {
         return _keyData._number <= right._keyData._number ? true : false ;
      }
      else
      {
         BSONObj meUpBound ( _keyData._pUpBoundData ) ;
         BSONObj rightUpBound ( right._keyData._pUpBoundData ) ;
         INT32 compare = 0 ;

         if ( _ordering )
         {
            compare = meUpBound.woCompare( rightUpBound, *_ordering, false ) ;
         }
         else
         {
            compare = meUpBound.woCompare( rightUpBound, BSONObj(), false ) ;
         }
         return compare <= 0 ? true : false ;
      }
   }

   bool _clsCataItemKey::operator> ( const _clsCataItemKey &right ) const
   {
      return right < *this ;
   }

   bool _clsCataItemKey::operator>= ( const _clsCataItemKey &right ) const
   {
      return right <= *this ;
   }

   bool _clsCataItemKey::operator!= ( const _clsCataItemKey &right ) const
   {
      return !(*this == right) ;
   }

   bool _clsCataItemKey::operator== ( const _clsCataItemKey &right ) const
   {
      if ( _clsCataItemKey::CLS_KEY_NUM == _keyType )
      {
         return _keyData._number == right._keyData._number ? true : false ;
      }
      else
      {
         BSONObj meUpBound ( _keyData._pUpBoundData ) ;
         BSONObj rightUpBound ( right._keyData._pUpBoundData ) ;

         INT32 compare = 0 ;
         if ( _ordering )
         {
            compare = meUpBound.woCompare( rightUpBound, *_ordering, false ) ;
         }
         else
         {
            compare = meUpBound.woCompare( rightUpBound, BSONObj(), false ) ;
         }
         return compare == 0 ? true : false ;
      }
   }

   _clsCataItemKey &_clsCataItemKey::operator= ( const _clsCataItemKey &right )
   {
      _keyType = right._keyType ;
      if ( _clsCataItemKey::CLS_KEY_BSON == _keyType )
      {
         _keyData._pUpBoundData = right._keyData._pUpBoundData ;
      }
      else
      {
         _keyData._number = right._keyData._number ;
      }
      _ordering = right._ordering ;

      return *this ;
   }

   /*
   note: _clsCatalogItem implement
   */
   _clsCatalogItem::_clsCatalogItem ( BOOLEAN saveName,
                                      BOOLEAN hasSubCl )
   {
      _groupID = 0 ;
      _saveName = saveName ;
      _isHash  = FALSE ;
      _hasSubCl = hasSubCl ;
      _isLast  = FALSE ;
      _ID      = 0 ;
   }

   _clsCatalogItem::~_clsCatalogItem ()
   {
   }

   UINT32 _clsCatalogItem::getGroupID () const
   {
      return _groupID ;
   }

   const BSONObj& _clsCatalogItem::getLowBound () const
   {
      return _lowBound ;
   }

   const BSONObj& _clsCatalogItem::getUpBound () const
   {
      return _upBound ;
   }

   clsCataItemKey _clsCatalogItem::getLowBoundKey ( const Ordering* ordering ) const
   {
      if ( !_isHash )
      {
         return clsCataItemKey ( _lowBound.objdata(), ordering ) ;
      }
      else
      {
         return clsCataItemKey( _lowBound.firstElement().numberInt() ) ;
      }
   }

   clsCataItemKey _clsCatalogItem::getUpBoundKey ( const Ordering * ordering ) const
   {
      if ( !_isHash )
      {
         return clsCataItemKey ( _upBound.objdata(), ordering ) ;
      }
      else
      {
         return clsCataItemKey ( _upBound.firstElement().numberInt() ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTIM_UDIM, "_clsCatalogItem::updateItem" )
   INT32 _clsCatalogItem::updateItem( const BSONObj & obj,
                                      BOOLEAN isSharding,
                                      BOOLEAN isHash )
   {
      INT32 rc = SDB_OK ;
      _isHash = isHash ;

      PD_TRACE_ENTRY ( SDB__CLSCTIM_UDIM ) ;

      PD_LOG ( PDDEBUG, "Update Catalog Item: %s", obj.toString().c_str() ) ;

      try
      {
         /// get the id
         BSONElement idEle = obj.getField( FIELD_NAME_ID ) ;
         if ( idEle.isNumber() )
         {
            _ID = (UINT32)idEle.numberInt() ;
         }

         if ( _hasSubCl )
         {
            BSONElement eleSubCLName = obj.getField( CAT_SUBCL_NAME );
            if ( eleSubCLName.type() == String )
            {
               _subCLName = eleSubCLName.str();
            }
            PD_CHECK( !_subCLName.empty(), SDB_SYS, error, PDERROR,
                     "Parse catalog item failed, field[%s] error",
                     CAT_SUBCL_NAME );
         }
         else
         {
            // group id
            BSONElement eleGroupID = obj.getField ( CAT_CATALOGGROUPID_NAME ) ;
            if ( !eleGroupID.isNumber() )
            {
               PD_LOG ( PDERROR, "Parse catalog item failed, field[%s] error",
                        CAT_CATALOGGROUPID_NAME ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            _groupID = eleGroupID.numberInt () ;

            // group name
            if ( _saveName )
            {
               BSONElement eleGroupName = obj.getField( CAT_GROUPNAME_NAME ) ;
               PD_CHECK( String == eleGroupName.type(), SDB_SYS, error, PDERROR,
                         "Parse catalog item failed, field[%s] type error, "
                         "type: %d", CAT_GROUPNAME_NAME, eleGroupName.type() ) ;
               _groupName = eleGroupName.String() ;
            }
         }

         if ( isSharding )
         {
            // lowBound
            BSONElement eleLow = obj.getField( CAT_LOWBOUND_NAME ) ;
            if ( Object != eleLow.type() )
            {
               PD_LOG ( PDERROR, "Parse catalog item failed, field[%s] error",
                        CAT_LOWBOUND_NAME ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            _lowBound = eleLow.embeddedObject().copy () ;

            // upBound
            BSONElement eleUp = obj.getField( CAT_UPBOUND_NAME ) ;
            if ( Object != eleUp.type () )
            {
               PD_LOG ( PDERROR, "Parse catalog item failed, field[%s] error",
                        CAT_UPBOUND_NAME ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            _upBound = eleUp.embeddedObject().copy () ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "UpdateItem exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSCTIM_UDIM, rc ) ;
      return rc ;
   error:
      PD_LOG ( PDERROR, "Catalog Item: %s", obj.toString().c_str() ) ;
      goto done ;
   }

   BSONObj _clsCatalogItem::toBson ()
   {
      try
      {
         if ( !_hasSubCl )
         {
            if ( !_saveName )
            {
               return BSON ( FIELD_NAME_ID << ( INT32 )_ID <<
                             CAT_CATALOGGROUPID_NAME << _groupID <<
                             CAT_LOWBOUND_NAME << _lowBound <<
                             CAT_UPBOUND_NAME << _upBound ) ;
            }
            else
            {
               return BSON ( FIELD_NAME_ID << ( INT32 )_ID <<
                             CAT_CATALOGGROUPID_NAME << _groupID <<
                             CAT_GROUPNAME_NAME << _groupName <<
                             CAT_LOWBOUND_NAME << _lowBound <<
                             CAT_UPBOUND_NAME << _upBound ) ;
            }
         }
         else
         {
            return BSON ( FIELD_NAME_ID << ( INT32 )_ID <<
                          CAT_SUBCL_NAME << _subCLName <<
                          CAT_LOWBOUND_NAME << _lowBound <<
                          CAT_UPBOUND_NAME << _upBound ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR,
                  "Exception happened when converting to bson obj: %s",
                  e.what() ) ;
      }
      // only hit here for exception
      return BSONObj () ;
   }

   void _clsCatalogItem::renameSubClName( const string& subCLName )
   {
      _subCLName = subCLName ;
   }

   /*
   note: _clsCataOrder implement
   */
   _clsCataOrder::_clsCataOrder ( const Ordering & order )
   :_ordering ( order )
   {
   }

   _clsCataOrder::~_clsCataOrder ()
   {
   }

   const Ordering* _clsCataOrder::getOrdering () const
   {
      return &_ordering ;
   }

   /*
   note: _clsCatalogSet implement
   */
   _clsCatalogSet::_clsCatalogSet ( const CHAR * name,
                                    BOOLEAN saveName,
                                    UINT64 clUniqueID )
   {
      _saveName = saveName ;
      _name = name ;
      _clUniqueID = clUniqueID ;
      _version = -1 ;
      _w = 1 ;
      _next = NULL ;
      _lastItem = NULL ;
      _pOrder = NULL ;
      _pKeyGen = NULL ;
      _isWholeRange = TRUE ;
      _groupCount = 0 ;
      _autoSplit = -1 ;
      _shardingType = CLS_CA_SHARDINGTYPE_NONE ;
      _ensureShardingIndex = TRUE ;
      _partition = CAT_SHARDING_PARTITION_DEFAULT ;
      ossIsPowerOf2( _partition, &_square ) ;
      _attribute = 0 ;
      _isMainCL = FALSE ;
      _internalV = 0 ;
      _maxID = 0 ;
      _skSiteID = 0 ;
      _pSite = NULL ;
      _compressType = UTIL_COMPRESSOR_INVALID ;
      _maxSize = 0 ;
      _maxRecNum = 0 ;
      _overwrite = FALSE ;
      _lobShardingKeyFormat = SDB_TIME_INVALID ;
   }

   _clsCatalogSet::~_clsCatalogSet ()
   {
      if ( _next )
      {
         SDB_OSS_DEL _next ;
         _next = NULL ;
      }

      _clear() ;
   }

   BOOLEAN _clsCatalogSet::isHashSharding() const
   {
      return CLS_CA_SHARDINGTYPE_HASH == _shardingType ;
   }

   BOOLEAN _clsCatalogSet::isRangeSharding() const
   {
      return CLS_CA_SHARDINGTYPE_RANGE == _shardingType ;
   }

   INT32 _clsCatalogSet::getVersion () const
   {
      return _version ;
   }

   UINT32 _clsCatalogSet::getW () const
   {
      return _w ;
   }

   const CHAR *_clsCatalogSet::name () const
   {
      return _name.c_str() ;
   }

   const string& _clsCatalogSet::nameStr() const
   {
      return _name ;
   }

   utilCLUniqueID _clsCatalogSet::clUniqueID() const
   {
      return _clUniqueID ;
   }

   VEC_GROUP_ID *_clsCatalogSet::getAllGroupID ()
   {
      return &_vecGroupID ;
   }

   BOOLEAN _clsCatalogSet::isInGroup( UINT32 groupID ) const
   {
      VEC_GROUP_ID::const_iterator cit = _vecGroupID.begin() ;
      while( cit != _vecGroupID.end() )
      {
         if ( groupID == *cit )
         {
            return TRUE ;
         }
         ++cit ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_GETALLGPID, "_clsCatalogSet::getAllGroupID" )
   INT32 _clsCatalogSet::getAllGroupID ( VEC_GROUP_ID &vecGroup ) const
   {
      INT32 size = 0 ;
      PD_TRACE_ENTRY ( SDB__CLSCTSET_GETALLGPID ) ;
      vecGroup.clear() ;
      size = (INT32)_vecGroupID.size() ;

      try
      {
         vecGroup.reserve( _vecGroupID.size() ) ;
         for ( INT32 index = 0 ; index < size ; index++ )
         {
            vecGroup.push_back ( _vecGroupID[index] ) ;
         }
      }
      catch( std::exception &e )
      {
         size = SDB_OOM ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      PD_TRACE_EXIT ( SDB__CLSCTSET_GETALLGPID ) ;
      return size ;
   }

   UINT32 _clsCatalogSet::groupCount() const
   {
      return _groupCount ;
   }

   const Ordering *_clsCatalogSet::getOrdering () const
   {
      if ( _pOrder )
      {
         return _pOrder->getOrdering() ;
      }
      return NULL ;
   }

   const BSONObj& _clsCatalogSet::getShardingKey () const
   {
      return _shardingKey ;
   }

   BSONObj _clsCatalogSet::OwnedShardingKey () const
   {
      return _shardingKey.getOwned() ;
   }

   BOOLEAN _clsCatalogSet::isWholeRange () const
   {
      return _isWholeRange ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_ADDGPID, "_clsCatalogSet::_addGroupID" )
   INT32 _clsCatalogSet::_addGroupID ( UINT32 groupID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCTSET_ADDGPID ) ;
      VEC_GROUP_ID::iterator it =  _vecGroupID.begin() ;
      while ( it != _vecGroupID.end() )
      {
         if ( *it == groupID )
         {
            goto done ;
         }
         ++it ;
      }
      try
      {
         _vecGroupID.reserve( CLS_CATSET_GROUPSZ_DFT ) ;
         _vecGroupID.push_back ( groupID ) ;
         _groupCount = _vecGroupID.size() ;
      }
      catch( std::exception &e )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSCTSET_ADDGPID, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_ADDSUBCLNAME, "_clsCatalogSet::_addSubClName" )
   INT32 _clsCatalogSet::_addSubClName( UINT32 id,
                                        const std::string &strClName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCTSET_ADDSUBCLNAME ) ;

      try
      {
         _subCLList.insert( std::make_pair( id, strClName ) ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
      }
      PD_TRACE_EXITRC ( SDB__CLSCTSET_ADDSUBCLNAME, rc ) ;
      return rc ;
   }

   BOOLEAN _clsCatalogSet::isSharding () const
   {
       return CLS_CA_SHARDINGTYPE_RANGE == _shardingType
             || CLS_CA_SHARDINGTYPE_HASH == _shardingType ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_ISINSDKEY, "_clsCatalogSet::isIncludeShardingKey" )
   BOOLEAN _clsCatalogSet::isIncludeShardingKey( const BSONObj &record ) const
   {
      PD_TRACE_ENTRY ( SDB__CLSCTSET_ISINSDKEY ) ;
      BOOLEAN isInclude = FALSE ;
      try
      {
         BSONObjIterator iterKey( _shardingKey ) ;
         while ( iterKey.more() )
         {
            BSONElement beKeyField = iterKey.next() ;
            BSONElement beTmp = record.getField( beKeyField.fieldName() ) ;
            if ( !beTmp.eoo() )
            {
               isInclude = TRUE ;
               break ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Occur exception when analyze record whether "
                  "include sharding key or not: %s", e.what() ) ;
      }

      PD_TRACE_EXIT ( SDB__CLSCTSET_ISINSDKEY ) ;
      return isInclude ;
   }

   BOOLEAN _clsCatalogSet::isAutoSplit () const
   {
      return _autoSplit == 1 ;
   }

   BOOLEAN _clsCatalogSet::hasAutoSplit () const
   {
      return _autoSplit != -1 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET__CLEAR, "_clsCatalogSet::_clear" )
   void _clsCatalogSet::_clear ()
   {
      PD_TRACE_ENTRY ( SDB__CLSCTSET__CLEAR ) ;
      if ( _pOrder )
      {
         SDB_OSS_DEL _pOrder ;
         _pOrder = NULL ;
      }

      if ( _pKeyGen )
      {
         SDB_OSS_DEL _pKeyGen ;
         _pKeyGen = NULL ;
      }

      if ( _pSite && _skSiteID > 0 )
      {
         _pSite->unregisterKey( _shardingKey, _skSiteID ) ;
      }
      _skSiteID = 0 ;

      MAP_CAT_ITEM_IT it = _mapItems.begin () ;
      while ( it != _mapItems.end() )
      {
         SDB_OSS_DEL it->second ;
         ++it ;
      }
      _mapItems.clear() ;
      _lastItem = NULL ;

      _vecGroupID.clear() ;
      _groupCount = 0 ;
      _subCLList.clear() ;

      _isWholeRange = TRUE ;
      _w = 1 ;
      _version = -1 ;

      _autoSplit = -1 ;
      _shardingType = CLS_CA_SHARDINGTYPE_NONE ;
      _shardingKey = BSONObj() ;
      _maxSize = 0 ;
      _maxRecNum = 0 ;
      _overwrite = FALSE ;

      _autoIncSet.clear() ;

      _lobShardingKeyFormat = SDB_TIME_INVALID ;

      PD_TRACE_EXIT ( SDB__CLSCTSET__CLEAR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_GENKEYOBJ, "_clsCatalogSet::genKeyObj" )
   INT32 _clsCatalogSet::genKeyObj( const BSONObj & obj, BSONObj & keyObj )
   {
      PD_TRACE_ENTRY ( SDB__CLSCTSET_GENKEYOBJ ) ;
      if ( !isSharding() )
      {
         return SDB_COLLECTION_NOTSHARD ;
      }
      INT32 rc = SDB_OK ;
      BSONObjSet objSet ;
      if ( !_pKeyGen )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "KeyGen Object is null" ) ;
         goto error ;
      }

      rc = _pKeyGen->getKeys( obj , objSet, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Object[%s] gen sharding key obj failed[rc:%d]",
                  obj.toString().c_str(), rc ) ;
         goto error ;
      }
      if ( objSet.size() != 1 )
      {
         PD_LOG ( PDINFO, "More than one sharding key[%d] is detected",
                  objSet.size() ) ;
         rc = SDB_MULTI_SHARDING_KEY ;
         goto error ;
      }

      keyObj = *objSet.begin() ;

   done:
      PD_TRACE_EXITRC ( SDB__CLSCTSET_GENKEYOBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   UINT32 _clsCatalogSet::getItemNum () const
   {
      return _mapItems.size() ;
   }

   _clsCatalogSet::POSITION _clsCatalogSet::getFirstItem()
   {
      return _mapItems.begin() ;
   }

   clsCatalogItem* _clsCatalogSet::getNextItem( _clsCatalogSet::POSITION & pos )
   {
      clsCatalogItem *item = NULL ;

      if ( pos != _mapItems.end() )
      {
         item = pos->second ;
         ++pos ;
      }

      return item ;
   }

   // given a data record, find the node range that satisfy the key of the
   // record
   // obj [in]: data record
   // item [out]: node satisfy the key
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_FINDIM, "_clsCatalogSet::findItem" )
   INT32 _clsCatalogSet::findItem ( const BSONObj &obj,
                                    clsCatalogItem *& item )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCTSET_FINDIM ) ;
      item = NULL ;
      // special condition for non-sharded collection
      // even thou _findItem checks _isSharding, we still need to do check here
      // because we have to use _shardingKey to generate key
      if ( !isSharding() || ( _isWholeRange && 1 == _groupCount ) )
      {
         // there should always be one element in map for non-sharded collection
         PD_CHECK ( 1 == _mapItems.size(), SDB_SYS, error, PDERROR,
                    "When not sharding, the collection[%s] cataItem "
                    "number[%d] error", name(), _mapItems.size() ) ;
         item = _mapItems.begin()->second ;
         goto done ;
      }
      {
         BSONObjSet objSet ;
         PD_CHECK( _pKeyGen, SDB_SYS, error, PDSEVERE, "KeyGen is null" ) ;
         // extract the key from record
         rc = _pKeyGen->getKeys( obj , objSet ) ;
         PD_RC_CHECK ( rc, PDERROR, "Generate key failed, rc = %d", rc ) ;
         // in sharded collection, sharding key can't include array with more
         // than 1 element
         PD_CHECK ( 1 == objSet.size(), SDB_MULTI_SHARDING_KEY, error, PDINFO,
                    "More than one sharding key is detected" ) ;

         {
            // attempt to find the key, since objSet must be 1, we use
            // objSet.begin()
            if ( isHashSharding() )
            {
               clsCataItemKey findKey( _hash( *(objSet.begin()) ) ) ;
               rc = _findItem ( findKey, item ) ;
            }
            else
            {
               clsCataItemKey findKey ( (*objSet.begin()).objdata(),
                                        getOrdering() ) ;
               rc = _findItem ( findKey, item ) ;
            }
            if ( rc )
            {
               // findItem may return SDB_CAT_NO_MATCH_CATALOG as expected, so
               // don't log anything
               goto done ;
            }
         }
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSCTSET_FINDIM, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsCatalogSet::_hash( const BSONObj &key )
   {
      return clsPartition( key, _square, getInternalV() ) ;
   }

   // given a clsCataItemKey, find the node range that satisfy the key of the
   // record
   // key [in]: data key
   // item [out]: node satisfy the key
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET__FINDIM, "_clsCatalogSet::_findItem" )
   INT32 _clsCatalogSet::_findItem( const clsCataItemKey &findKey,
                                    clsCatalogItem *& item )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCTSET__FINDIM ) ;
      item = NULL ;
      // special condition for non-sharded env
      if ( !isSharding() || ( _isWholeRange && _groupCount == 1 ) )
      {
         // there should always be one element in map for non-sharded collection
         PD_CHECK ( 1 == _mapItems.size(), SDB_SYS, error, PDERROR,
                    "When not sharding, the collection[%s] cataItem "
                    "number[%d] error", name(), _mapItems.size() ) ;
         item = _mapItems.begin()->second ;
         goto done ;
      }
      {
         // find the upper bound of the requested key
         MAP_CAT_ITEM_IT it = _mapItems.upper_bound ( findKey ) ;
         // if we build this object by specifying group restricts, the
         // _mapItem may not always contains MaxKey, in this case
         // SDB_CAT_NO_MATCH_CATALOG represents the given groups does not
         // include ranges for the record
         if ( it == _mapItems.end () )
         {
            item = _lastItem ;
            rc = item ? SDB_OK : SDB_CAT_NO_MATCH_CATALOG ;
            goto done ;
         }

         // if we are searching specfic ranges ( not all partitions, we have
         // to compare the lowbound for the range
         if ( !_isWholeRange )
         {
            // if the key does not greater than the low bound, it also does
            // not fall into the range, this behavior could be expected and
            // we should not log anything
            if ( findKey < it->second->getLowBoundKey( getOrdering() ) )
            {
               rc = SDB_CAT_NO_MATCH_CATALOG ;
               goto error ;
            }
         }
         item = it->second ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSCTSET__FINDIM, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // given a data record, find the groupID satisfy the partition key
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_FINDGPID, "_clsCatalogSet::findGroupID" )
   INT32 _clsCatalogSet::findGroupID ( const BSONObj & obj, UINT32 &groupID )
   {
      INT32 rc              = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCTSET_FINDGPID ) ;
      _clsCatalogItem *item = NULL ;
      rc = findItem ( obj, item ) ;
      // if we can't find anything, it doesn't mean we hit error, it may simply
      // the record doesn't fall into the groups we want to filter, so we don't
      // log anything
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      groupID = item->getGroupID () ;
   done :
      PD_TRACE_EXITRC ( SDB__CLSCTSET_FINDGPID, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_FINDGPID2, "_clsCatalogSet::findGroupID" )
   INT32 _clsCatalogSet::findGroupID( const bson::OID &oid,
                                      UINT32 sequence,
                                      UINT32 &groupID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSCTSET_FINDGPID2 ) ;
      SDB_ASSERT( !isRangeSharding(), "can not be range sharded" ) ;
      clsCatalogItem *item = NULL ;
      INT32 range = clsPartition( oid, sequence, getPartitionBit() ) ;
      rc = _findItem( range, item ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to find item:%d", rc ) ;
         goto error ;
      }

      groupID = item->getGroupID() ;
   done:
      PD_TRACE_EXITRC( SDB__CLSCTSET_FINDGPID2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_FINDGPIDS, "_clsCatalogSet::findGroupIDS" )
   INT32 _clsCatalogSet::findGroupIDS ( const BSONObj &matcher,
                                        VEC_GROUP_ID &vecGroup )
   {
      PD_TRACE_ENTRY ( SDB__CLSCTSET_FINDGPIDS ) ;
      INT32 rc = SDB_OK ;

      PD_CHECK ( !_mapItems.empty(), SDB_SYS, error, PDERROR,
                 "The collection[%s]'s cataItem is empty", name() ) ;

      try
      {
         if ( !isSharding() || 1 == _groupCount ||
              ( isHashSharding() && getInternalV() < CAT_INTERNAL_VERSION_2 ) )
         {
            vecGroup.reserve( _groupCount ) ;
            VEC_GROUP_ID::iterator itGrp = _vecGroupID.begin() ;
            while ( itGrp != _vecGroupID.end() )
            {
               vecGroup.push_back( *itGrp ) ;
               ++itGrp ;
            }
         }
         else
         {
            CLS_SET_CATAITEM setItem ;
            CLS_SET_CATAITEM::iterator itSet ;
            clsCatalogMatcher clsMatcher( _shardingKey, isHashSharding() ) ;
            rc = clsMatcher.loadPattern( matcher );
            PD_RC_CHECK( rc, PDERROR, "Load matcher failed, rc: %d", rc ) ;

            if ( clsMatcher.isUniverse() )
            {
               vecGroup.reserve( _groupCount ) ;
               VEC_GROUP_ID::iterator itGrp = _vecGroupID.begin() ;
               while ( itGrp != _vecGroupID.end() )
               {
                  vecGroup.push_back( *itGrp ) ;
                  ++itGrp ;
               }
               goto done ;
            }

            rc = clsMatcher.calc( this, setItem ) ;
            if ( rc )
            {
               goto error ;
            }

            vecGroup.reserve( CLS_CATSET_GROUPSZ_DFT ) ;
            itSet = setItem.begin() ;
            while( itSet != setItem.end() )
            {
               vecGroup.push_back( (*itSet)->getGroupID() ) ;
               ++itSet ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSCTSET_FINDGPIDS, rc ) ;
      return rc;
   error:
      goto done;
   }

   INT32 _clsCatalogSet::getGroupLowBound( UINT32 groupID,
                                           BSONObj &lowBound ) const
   {
      clsCatalogItem *item = NULL ;
      MAP_CAT_ITEM::const_iterator it = _mapItems.begin() ;
      while ( it != _mapItems.end() )
      {
         item = it->second ;
         if ( item->getGroupID() == groupID || 0 == groupID )
         {
            lowBound = item->getLowBound() ;
            return SDB_OK ;
         }
         ++it ;
      }
      return SDB_CAT_NO_MATCH_CATALOG ;
   }

   INT32 _clsCatalogSet::getGroupUpBound( UINT32 groupID,
                                          BSONObj &upBound ) const
   {
      clsCatalogItem *item = NULL ;
      MAP_CAT_ITEM::const_reverse_iterator rit = _mapItems.rbegin() ;
      while ( rit != _mapItems.rend() )
      {
         item = rit->second ;
         if ( item->getGroupID() == groupID || 0 == groupID )
         {
            upBound = item->getUpBound() ;
            return SDB_OK ;
         }
         ++rit ;
      }
      return SDB_CAT_NO_MATCH_CATALOG ;
   }

   // check whether a given object in the specified group
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_ISOBJINGP, "_clsCatalogSet::isObjInGroup" )
   BOOLEAN _clsCatalogSet::isObjInGroup ( const BSONObj &obj, UINT32 groupID )
   {
      INT32 rc          = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCTSET_ISOBJINGP ) ;
      UINT32 retGroupID = 0 ;
      BOOLEAN result    = FALSE ;
      rc = findGroupID ( obj, retGroupID ) ;
      PD_RC_CHECK ( rc, PDDEBUG, "Failed to find object from group, rc = %d",
                    rc ) ;
      result = retGroupID == groupID ;
   done :
      PD_TRACE_EXITRC ( SDB__CLSCTSET_ISOBJINGP, rc ) ;
      return result ;
   error :
      result = FALSE ;
      goto done ;
   }

   // check whether a given key in the specified group
   // the key must be well organized by generated from sharding key before
   // calling the function.
   // for the original data record, please use isObjInGroup function instead
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_ISKEYINGP, "_clsCatalogSet::isKeyInGroup" )
   BOOLEAN _clsCatalogSet::isKeyInGroup ( const BSONObj &obj, UINT32 groupID )
   {
      INT32 rc              = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCTSET_ISKEYINGP ) ;
      clsCatalogItem *item  = NULL ;
      BOOLEAN result        = FALSE ;

      if ( isHashSharding() )
      {
         clsCataItemKey findKey ( obj.firstElement().numberInt() ) ;
         rc = _findItem( findKey, item ) ;
      }
      else
      {
         clsCataItemKey findKey ( obj.objdata(), getOrdering() ) ;
         rc = _findItem( findKey, item ) ;
      }

      if ( rc )
      {
         goto error ;
      }
      SDB_ASSERT ( item, "item can't be NULL" ) ;
      result = item->getGroupID() == groupID ;
   done :
      PD_TRACE_EXITRC ( SDB__CLSCTSET_ISKEYINGP, rc ) ;
      return result ;
   error :
      result = FALSE ;
      goto done ;
   }

   // check whether a given key is on boundary of any group
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_ISKEYONBD, "_clsCatalogSet::isKeyOnBoundary" )
   BOOLEAN _clsCatalogSet::isKeyOnBoundary ( const BSONObj &obj,
                                             UINT32* pGroupID )
   {
      PD_TRACE_ENTRY ( SDB__CLSCTSET_ISKEYONBD ) ;
      BOOLEAN result        = FALSE ;

      if ( !isSharding() )
      {
         // for non-sharded environment, we always return FALSE
         result = FALSE ;
         goto done ;
      }
      if ( _mapItems.size() < 1 )
      {
         PD_LOG( PDERROR, "Failed to check key boundary : "
                 "there must be at least 1 range for the collection" ) ;
         result = FALSE ;
         goto error ;
      }
      {
         // find the upper bound of the requested key
         // We have to use lower_bound here because {MaxKey} will not be matched
         // into any upper_bound ( since upper_bound is trying to find anything
         // greater than {MaxKey}, and obviously it's not possible
         // Therefore, we use lower_bound in order to keep track of all possible
         // ranges
         MAP_CAT_ITEM_IT it ;
         if ( isHashSharding() )
         {
            clsCataItemKey findKey ( obj.firstElement().numberInt() ) ;
            it = _mapItems.lower_bound ( findKey ) ;

            // if we build this object by specifying group restricts, the _mapItem
            // may not always contains MaxKey, in this case SDB_CAT_NO_MATCH_CATALOG
            // represents the given groups does not include ranges for the key
            if ( it == _mapItems.end() )
            {
               result = FALSE ;
               goto done ;
            }
            result = ( findKey == it->second->getLowBoundKey( getOrdering() ) ||
                       findKey == it->second->getUpBoundKey( getOrdering() ) ) ;
         }
         else
         {
            clsCataItemKey findKey ( obj.objdata(), getOrdering() ) ;
            it = _mapItems.lower_bound ( findKey ) ;

            // if we build this object by specifying group restricts, the _mapItem
            // may not always contains MaxKey, in this case SDB_CAT_NO_MATCH_CATALOG
            // represents the given groups does not include ranges for the key
            if ( it == _mapItems.end() )
            {
               result = FALSE ;
               goto done ;
            }
            result = ( findKey == it->second->getLowBoundKey( getOrdering() ) ||
                       findKey == it->second->getUpBoundKey( getOrdering() ) ) ;
         }

         if ( result && pGroupID )
         {
            result = *pGroupID == it->second->getGroupID() ? TRUE : FALSE ;
         }
      }
   done :
      PD_TRACE_EXIT ( SDB__CLSCTSET_ISKEYONBD ) ;
      return result ;
   error :
      goto done ;
   }

   // add a new range on a given group id
   // note a split must happen on full range
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_SPLIT, "_clsCatalogSet::split" )
   INT32 _clsCatalogSet::split ( const BSONObj &splitKey,
                                 const BSONObj &splitEndKey,
                                 UINT32 groupID, const CHAR *groupName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__CLSCTSET_SPLIT ) ;

      SDB_ASSERT ( _isWholeRange, "split must be done on whole range" ) ;

      // sanity check
      if ( isHashSharding() )
      {
         PD_CHECK( 1 == splitKey.nFields() &&
                   NumberInt == splitKey.firstElement().type(), SDB_SYS,
                   error, PDERROR, "Hash sharding split key[%s] must be Int",
                   splitKey.toString().c_str() ) ;
         PD_CHECK( splitEndKey.isEmpty() || ( 1 == splitEndKey.nFields() &&
                   NumberInt == splitEndKey.firstElement().type() ), SDB_SYS,
                   error, PDERROR, "Hash sharding split key[%s] must be Int",
                   splitEndKey.toString().c_str() ) ;
      }
      else
      {
         PD_CHECK ( splitKey.nFields() == _shardingKey.nFields(),
                    SDB_SYS, error, PDERROR,
                    "split key does not contains same number of field with "
                    "sharding key\n"
                    "splitKey: %s\n"
                    "shardingKey: %s",
                    splitKey.toString(false,false).c_str(),
                    _shardingKey.toString(false,false).c_str() ) ;
         PD_CHECK ( splitEndKey.isEmpty() ||
                    ( splitEndKey.nFields() == _shardingKey.nFields() ),
                    SDB_SYS, error, PDERROR,
                    "split key does not contains same number of field with "
                    "sharding key\n"
                    "splitKey: %s\n"
                    "shardingKey: %s",
                    splitEndKey.toString(false,false).c_str(),
                    _shardingKey.toString(false,false).c_str() ) ;
      }

      try
      {
         // Split items with different cases
         if ( isHashSharding() )
         {
            // Hash sharding
            clsCataItemKey findKey( splitKey.firstElement().numberInt() ) ;
            if ( !splitEndKey.isEmpty() )
            {
               clsCataItemKey endKey( splitEndKey.firstElement().numberInt() ) ;
               rc = _splitInternal( splitKey, splitEndKey, &findKey, &endKey,
                                    groupID, groupName ) ;
            }
            else
            {
               rc = _splitInternal( splitKey, splitEndKey, &findKey, NULL,
                                    groupID, groupName ) ;
            }
         }
         else
         {
            // Range sharding
            clsCataItemKey findKey( splitKey.objdata(), getOrdering() ) ;
            if ( !splitEndKey.isEmpty() )
            {
               clsCataItemKey endKey( splitEndKey.objdata(), getOrdering() ) ;
               rc = _splitInternal( splitKey, splitEndKey, &findKey, &endKey,
                                    groupID, groupName ) ;
            }
            else
            {
               rc = _splitInternal( splitKey, splitEndKey, &findKey, NULL,
                                    groupID, groupName ) ;
            }
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to split items, rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception happened during split: %s",
                       e.what() ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__CLSCTSET_SPLIT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET__SPLITINT, "_clsCatalogSet::_splitInternal" )
   INT32 _clsCatalogSet::_splitInternal ( const BSONObj &splitKey,
                                          const BSONObj &splitEndKey,
                                          clsCataItemKey *findKey,
                                          clsCataItemKey *endKey,
                                          UINT32 groupID,
                                          const CHAR *groupName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__CLSCTSET__SPLITINT ) ;

      SDB_ASSERT ( _isWholeRange, "split must be done on whole range" ) ;

      clsCatalogItem *item = NULL ;
      clsCatalogItem *itemEnd = NULL ;

      try
      {
         // find the upper bound for the given key
         MAP_CAT_ITEM_IT it = _mapItems.upper_bound ( *findKey ) ;

         // if we hit end of the map, let's use _lastItem
         // since this is always for whole range, so we should never hit
         // item=NULL
         if ( it == _mapItems.end () )
         {
            item = _lastItem ;
            --it ;
         }
         else
         {
            item = it->second ;
         }
         // make sure item is not NULL
         SDB_ASSERT ( item, "last item should never be NULL in split" ) ;
         // if the target and destination groups are the same, we don't need to
         // split at all, but why it happen?
         if ( item->getGroupID() == groupID )
         {
            PD_LOG ( PDERROR, "split got duplicate source and dest on group %d",
                     groupID ) ;
            goto done ;
         }

         {
            MAP_CAT_ITEM_IT tmpit = it ;
            BOOLEAN itemRemoved = FALSE, itemEndRemoved = FALSE ;

            // 0) traverse from it and change all ranges to the new group
            while ( (++tmpit) != _mapItems.end() )
            {
               clsCatalogItem *tmpItem = (*tmpit).second ;

               if ( endKey && *endKey <
                    tmpItem->getUpBoundKey( getOrdering() ) )
               {
                  if ( tmpItem->getGroupID() == item->getGroupID() )
                  {
                     itemEnd = tmpItem ;
                  }
                  break ;
               }

               if ( tmpItem->getGroupID() == item->getGroupID() )
               {
                  tmpItem->_groupID = groupID ;
                  tmpItem->_groupName = groupName ;
               }
            }

            rc = _splitItem( item, findKey, endKey, splitKey,
                             splitEndKey, groupID, groupName, itemRemoved ) ;
            if ( itemRemoved )
            {
               SAFE_OSS_DELETE( item ) ;
            }
            PD_RC_CHECK( rc, PDERROR, "Split begin item failed, rc: %d", rc ) ;
            rc = _splitItem( itemEnd, findKey, endKey, splitKey,
                             splitEndKey, groupID, groupName, itemEndRemoved ) ;
            if ( itemEndRemoved )
            {
               SAFE_OSS_DELETE( itemEnd ) ;
            }
            PD_RC_CHECK( rc, PDERROR, "Split end item failed, rc: %d", rc ) ;

            // 7) remove all duplicate records after the new group
            _deduplicate () ;

            // finally remake group ids
            rc = _remakeGroupIDs() ;
            if ( rc )
            {
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception happened during split: %s",
                       e.what() ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__CLSCTSET__SPLITINT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _clsCatalogSet::_remakeGroupIDs()
   {
      INT32 rc = SDB_OK ;
      _vecGroupID.clear() ;
      clsCatalogItem *item = NULL ;

      MAP_CAT_ITEM_IT it = _mapItems.begin() ;
      while ( it != _mapItems.end() )
      {
         item = it->second ;
         rc = _addGroupID( item->getGroupID() ) ;
         if ( rc )
         {
            break ;
         }
         ++it ;
      }

      return rc ;
   }

   INT32 _clsCatalogSet::_removeItem( clsCatalogItem * item )
   {
      MAP_CAT_ITEM_IT it = _mapItems.begin() ;
      while ( it != _mapItems.end() )
      {
         if ( it->second == item )
         {
            _mapItems.erase( it ) ;
            break ;
         }
         ++it ;
      }
      return SDB_OK ;
   }

   INT32 _clsCatalogSet::_addItem( clsCatalogItem * item )
   {
      INT32 rc = SDB_OK ;

      try
      {
         // add to map
         if ( !(_mapItems.insert(std::make_pair(
                                item->getUpBoundKey( getOrdering() ),
                                item))).second )
         {
            // if two ranges got same upper bound, that means something wrong
            rc = SDB_CAT_CORRUPTION ;
            PD_LOG ( PDERROR, "CataItem already exist: %s",
                     item->toBson().toString().c_str() ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
      }

      return rc ;
   }

   INT32 _clsCatalogSet::_splitItem( clsCatalogItem *item,
                                     clsCataItemKey *beginKey,
                                     clsCataItemKey *endKey,
                                     const BSONObj &beginKeyObj,
                                     const BSONObj &endKeyObj,
                                     UINT32 groupID,
                                     const CHAR *groupName,
                                     BOOLEAN &itemRemoved )
   {
      INT32 rc = SDB_OK ;
      clsCatalogItem *newItem = NULL ;

      itemRemoved = FALSE ;

      if ( NULL == item )
      {
         goto done ;
      }

      if ( *beginKey <= item->getLowBoundKey( getOrdering() ) )
      {
         if ( !endKey || *endKey >= item->getUpBoundKey( getOrdering() ) )
         {
            item->_groupID = groupID ;
            item->_groupName = groupName ;
         }
         // Recheck the endKey to make sure the bounds inside item bounds
         else if ( *endKey > item->getLowBoundKey( getOrdering() ) )
         {
            newItem = SDB_OSS_NEW clsCatalogItem( _saveName ) ;
            PD_CHECK( newItem, SDB_OOM, error, PDERROR, "Alloc failed" ) ;
            _removeItem( item ) ;
            itemRemoved = TRUE ;

            newItem->_lowBound = item->_lowBound.getOwned() ;
            newItem->_upBound  = endKeyObj.getOwned() ;
            newItem->_groupID  = groupID ;
            newItem->_groupName = groupName ;
            newItem->_isHash   = item->_isHash ;
            newItem->_ID       = ++_maxID ;

            item->_lowBound    = endKeyObj.getOwned() ;

            rc = _addItem( newItem ) ;
            if ( rc )
            {
               goto error ;
            }
            rc = _addItem( item ) ;
            if ( rc )
            {
               goto error ;
            }
            itemRemoved = FALSE ;
         }
      }
      else
      {
         newItem = SDB_OSS_NEW clsCatalogItem( _saveName ) ;
         PD_CHECK( newItem, SDB_OOM, error, PDERROR, "Alloc failed" ) ;
         _removeItem( item ) ;
         itemRemoved = TRUE ;

         newItem->_lowBound   = beginKeyObj.getOwned() ;
         newItem->_upBound    = item->_upBound.getOwned() ;
         newItem->_groupID    = groupID ;
         newItem->_groupName  = groupName ;
         newItem->_isHash     = item->_isHash ;
         newItem->_ID         = ++_maxID ;

         item->_upBound = beginKeyObj.getOwned() ;

         rc = _addItem( item ) ;
         if ( rc )
         {
            goto error ;
         }
         itemRemoved = FALSE ;

         rc = _addItem( newItem ) ;
         if ( rc )
         {
            goto error ;
         }

         if ( endKey && *endKey < newItem->getUpBoundKey( getOrdering() ) )
         {
            BOOLEAN newItemRemoved = FALSE ;
            rc = _splitItem( newItem, endKey, NULL, endKeyObj, BSONObj(),
                             item->_groupID, item->_groupName.c_str(),
                             newItemRemoved ) ;
            if ( !newItemRemoved )
            {
               // The new item had been added, should be freed by destructor
               // If the new item is removed again, will be error, free it in
               // error label
               newItem = NULL ;
            }
            PD_RC_CHECK( rc, PDERROR, "Sub split failed, rc: %d", rc ) ;
         }
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( newItem ) ;
      goto done ;
   }

   // this function removes adj ranges with same group id, by merging those
   // groups together
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET__DEDUP, "_clsCatalogSet::_deduplicate" )
   void _clsCatalogSet::_deduplicate ()
   {
      PD_TRACE_ENTRY ( SDB__CLSCTSET__DEDUP ) ;
      SDB_ASSERT ( _mapItems.size() >= 1, "_mapItems can't be NULL" ) ;
      MAP_CAT_ITEM_IT it     = _mapItems.begin() ;
      MAP_CAT_ITEM_IT itPrev = it ;
      ++it ;
      for ( ; it != _mapItems.end(); ++it )
      {
         // if previous group it is same as current group, let's assign the
         // low bound of prev group to the current group, and remove the prev
         // group from the list
         if ( it->second->getGroupID() ==
              itPrev->second->getGroupID() )
         {
            clsCatalogItem *itemPrev = ( itPrev->second ) ;

            // no need to copy the buffer because the bsonobj in catalogitem
            // ALWAYS own the buffer with smart pointer
            it->second->_lowBound = itemPrev->_lowBound ;

            // Remove item and free the memory
            _mapItems.erase ( itPrev ) ;
            SAFE_OSS_DELETE( itemPrev ) ;
         }
         itPrev = it ;
      }
      PD_TRACE_EXIT ( SDB__CLSCTSET__DEDUP ) ;
   }

   // build CataInfo field
   // non-sharded:
   // { "CataInfo":[{"GroupID":1000}]}
   // sharded
   // { "CataInfo":[{"GroupID":1000, "LowBound":{"":MinKey, "":MinKey},
   // "UpBound":{"":5,"":"abc"}}, { "GroupID":1001, "LowBound":{"":5, "":"abc"},
   // "UpBound":{"":MaxKey, "":MaxKey}}]}
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_TOCTINFOBSON, "_clsCatalogSet::toCataInfoBson" )
   BSONObj _clsCatalogSet::toCataInfoBson ()
   {
      PD_TRACE_ENTRY ( SDB__CLSCTSET_TOCTINFOBSON ) ;
      BSONObj obj ;
      if ( !isSharding() )
      {
         // non-sharded collection case, only need to provid groupid
         SDB_ASSERT ( _mapItems.size() == 1,
                      "map item size must be 1 for non-sharded collection" ) ;
         try
         {
            // since there's always only one element in non-sharded collection,
            // we just need to get the very first element in map and get it's
            // group id
            obj = BSON ( CAT_CATALOGINFO_NAME <<
                         BSON_ARRAY ( BSON ( CAT_CATALOGGROUPID_NAME <<
                                      _mapItems.begin()->second->getGroupID() )
                         )
                       ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR,
                     "Exception happened during creating "
                     "non-sharded catainfo: %s", e.what() ) ;
         }
      } // if ( !_isSharding )
      else
      {
         // sharded collection case, need to provide groupid, lowbound/upbound
         // for each range
         try
         {
            // builder for CataInfo
            BSONObjBuilder bb ;
            // builder for the array of catainfo
            BSONArrayBuilder ab ;
            // loop for each element in map
            MAP_CAT_ITEM_IT it     = _mapItems.begin() ;
            for ( ; it != _mapItems.end(); ++it )
            {
               // append each range into the array
               ab.append ( it->second->toBson () ) ;
            }
            // append array into CataInfo
            bb.append ( CAT_CATALOGINFO_NAME, ab.arr () ) ;
            // convert to bson object
            obj = bb.obj () ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR,
                     "Exception happened during creating "
                     "sharded catainfo: %s", e.what() ) ;
         }
      } // else
      PD_TRACE_EXIT ( SDB__CLSCTSET_TOCTINFOBSON ) ;
      return obj ;
   }

   // provide a collection record, and a groupID ( groupID = 0 means all groups
   // )
   // catSet [in]: collection record, which is in SYSCAT.SYSCOLLECTIONS
   // groupID [in]: the groupID we want to filter, 0 means all groups
   // note the catSet record should always be fetched from catalog collection
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCT_UDCATSET, "_clsCatalogSet::updateCatSet" )
   INT32 _clsCatalogSet::updateCatSet( const BSONObj & catSet,
                                       UINT32 groupID,
                                       BOOLEAN allowUpdateID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCT_UDCATSET ) ;

      PD_LOG ( PDDEBUG, "Update cataSet: %s", catSet.toString().c_str() ) ;
      // clean up previous existed info
      _clear() ;

      // version, required
      BSONElement ele = catSet.getField ( CAT_CATALOGVERSION_NAME ) ;
      PD_CHECK ( !ele.eoo() && ele.type() == NumberInt, SDB_CAT_CORRUPTION,
                 error, PDSEVERE,
                 "Catalog [%s] type error, type: %d",
                 CAT_CATALOGVERSION_NAME, ele.type() ) ;
      _version = (INT32)ele.Int() ;

      //update w, optional, 1 for default
      ele = catSet.getField( CAT_CATALOG_W_NAME ) ;
      // if the element does not exist, use default 1
      if ( ele.eoo () )
      {
         _w = 1 ;
      }
      else if ( ele.type() != NumberInt )
      {
         // if type is not Number, something wrong
         PD_RC_CHECK ( SDB_CAT_CORRUPTION, PDSEVERE,
                       "Catalog [%s] type error, type: %d",
                       CAT_CATALOG_W_NAME, ele.type() ) ;
      }
      else
      {
         _w = (INT32)ele.Int() ;
      }

      // update unique id
      if ( allowUpdateID )
      {
         ele = catSet.getField( CAT_CL_UNIQUEID ) ;
         if ( ele.eoo() )
         {
            _clUniqueID = 0;
         }
         else if ( ele.type() != NumberLong )
         {
            // if type is not Number, something wrong
            PD_RC_CHECK ( SDB_CAT_CORRUPTION, PDSEVERE,
                          "Catalog [%s] type error, type: %d",
                          CAT_CL_UNIQUEID, ele.type() ) ;
         }
         else
         {
            _clUniqueID = (UINT64)ele.Long();
         }
      }

      // isMainCl
      ele = catSet.getField( CAT_IS_MAINCL );
      if ( ele.booleanSafe() )
      {
         _isMainCL = TRUE;
         _isWholeRange = FALSE;
      }
      else
      {
         _isMainCL = FALSE;
      }

      // MainCLName
      ele = catSet.getField( CAT_MAINCL_NAME );
      if ( ele.type() == String )
      {
         _mainCLName = ele.str();
      }
      else
      {
         _mainCLName.clear() ;
      }

      // LobShardingKeyFormat
      _lobShardingKeyFormat = SDB_TIME_INVALID ;
      ele = catSet.getField( CAT_LOBSHARDINGKEYFORMAT_NAME );
      if ( ele.type() == String )
      {
         if ( !clsCheckAndParseLobKeyFormat( ele.valuestr(),
                                             &_lobShardingKeyFormat ) )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Init %s[%s] failed, rc: %d",
                    CAT_LOBSHARDINGKEYFORMAT_NAME, ele.valuestr(), rc ) ;
            goto error ;
         }
      }

      // auto increment parse
      ele = catSet.getField( CAT_AUTOINCREMENT ) ;
      if ( !ele.eoo() )
      {
         rc = _autoIncSet.init( ele ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init auto-increment[%s] failed, rc: %d",
                    catSet.toString().c_str(), rc ) ;
            goto error ;
         }
      }
      else
      {
         _autoIncSet.clear() ;
      }

      /// get attribute, it is optional.
      ele = catSet.getField (CAT_ATTRIBUTE_NAME ) ;
      if ( NumberInt == ele.type() )
      {
         _attribute = ele.numberInt() ;
      }

      // get the CompressionType. It's optional. Its value can be: -1, 0, 1.
      ele = catSet.getField( CAT_COMPRESSIONTYPE ) ;
      if ( ele.eoo() )
      {
         _compressType = UTIL_COMPRESSOR_INVALID ;
      }
      else if ( NumberInt == ele.type() )
      {
         INT32 compressType = ele.numberInt() ;
         if ( compressType < UTIL_COMPRESSOR_SNAPPY ||
              compressType > UTIL_COMPRESSOR_INVALID )
         {
            PD_LOG( PDERROR, "invalid value of compression type: %d",
                    compressType ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         _compressType = (UTIL_COMPRESSOR_TYPE)compressType ;
      }
      else
      {
         PD_LOG( PDERROR, "Catalog [%s] type error, type: %d" ,
                 CAT_COMPRESSIONTYPE, ele.type() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // Get the Size and Max options, if any.
      ele = catSet.getField( CAT_CL_MAX_SIZE ) ;
      if ( !ele.eoo() )
      {
         if ( !ele.isNumber() )
         {
            PD_LOG( PDERROR, "Type of Size option is not number: %d",
                    ele.type() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _maxSize = ele.numberLong() ;
      }

      ele = catSet.getField( CAT_CL_MAX_RECNUM ) ;
      if ( !ele.eoo() )
      {
         if ( !ele.isNumber() )
         {
            PD_LOG( PDERROR, "Type of Max option is not number: %d",
                    ele.type() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _maxRecNum = ele.numberLong() ;
      }

      ele = catSet.getField( CAT_CL_OVERWRITE ) ;
      if ( !ele.eoo() )
      {
         if ( !ele.isBoolean() )
         {
            PD_LOG( PDERROR, "Type of OverWrite option is not bool: %d",
                    ele.type() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _overwrite = ele.boolean() ;
      }

      //update sharding key, optional, default false
      ele = catSet.getField( CAT_SHARDINGKEY_NAME ) ;
      if ( ele.eoo() )
      {
         _shardingType = CLS_CA_SHARDINGTYPE_NONE ;
      }
      else if ( ele.type() != Object )
      {
         PD_RC_CHECK ( SDB_CAT_CORRUPTION, PDSEVERE,
                       "Catalog [%s] type error, type: %d",
                       CAT_SHARDINGKEY_NAME, ele.type() ) ;
      }
      else
      {
         _shardingKey = ele.embeddedObject().copy () ;

         ele = catSet.getField( CAT_SHARDING_TYPE ) ;

         /// range in default.
         if ( ele.eoo() )
         {
            _shardingType = CLS_CA_SHARDINGTYPE_RANGE ;
         }
         else if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "invalid type of shardingtype:%d",
                    ele.type() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else if ( 0 == ossStrcmp( CAT_SHARDING_TYPE_HASH,
                                   ele.valuestrsafe() ) )
         {
            _shardingType = CLS_CA_SHARDINGTYPE_HASH ;
         }
         else if ( 0 == ossStrcmp( CAT_SHARDING_TYPE_RANGE,
                                   ele.valuestrsafe() ) )
         {
            _shardingType = CLS_CA_SHARDINGTYPE_RANGE ;
         }
         else
         {
            PD_LOG( PDERROR, "invalid value of shardingtype:%s",
                    ele.valuestrsafe() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         /// ensureShardingIndex
         ele = catSet.getField( CAT_ENSURE_SHDINDEX ) ;
         if ( ele.eoo() )
         {
            _ensureShardingIndex = TRUE ;
         }
         else if ( Bool != ele.type() )
         {
            PD_LOG( PDERROR, "invalid type of ensureShardingIndex: %d",
                    ele.type() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else
         {
            _ensureShardingIndex = ele.boolean() ;
         }

         // AutoSplit
         ele = catSet.getField( CAT_DOMAIN_AUTO_SPLIT ) ;
         if ( ele.eoo() )
         {
            _autoSplit = -1 ;
         }
         else if ( Bool != ele.type() )
         {
            PD_LOG( PDERROR, "invalid type of ensureShardingIndex: %d",
                    ele.type() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else
         {
            _autoSplit = ele.boolean() ? 1 : 0 ;
         }

         /// register to sharding key site
         if ( _pSite )
         {
            _skSiteID = _pSite->registerKey( _shardingKey ) ;
         }
      }

      if ( isHashSharding() )
      {
         BSONElement ele = catSet.getField( CAT_SHARDING_PARTITION ) ;
         PD_CHECK( ele.eoo() || ele.isNumber(), SDB_SYS, error, PDERROR,
                   "Field[%s] type error, type: %d", CAT_SHARDING_PARTITION,
                   ele.type() ) ;
         _partition = ele.numberInt() ;

         PD_CHECK( ossIsPowerOf2( (UINT32)_partition, &_square ), SDB_SYS,
                   error, PDERROR, "Parition[%d] is not power of 2",
                   _partition ) ;

         /// InternalV
         /// when internal version is old, we broadcast the query request to every group.
         /// since of SEQUOIADBMAINSTREAM-610, we changed the partition function.
         ele = catSet.getField( CAT_INTERNAL_VERSION ) ;
         if ( NumberInt == ele.type() )
         {
            _internalV = ele.Int() ;
         }
      }

      // ordering, which is depends on _shardingKey, in non-sharded collection,
      // _shardingKey is empty and Order is not taking effective
      if ( isRangeSharding() )
      {
         _pOrder = SDB_OSS_NEW clsCataOrder ( Ordering::make ( _shardingKey ) ) ;
         PD_CHECK ( _pOrder, SDB_OOM, error, PDERROR,
                    "Failed to alloc memory for CataOrder" ) ;
      }

      _pKeyGen = SDB_OSS_NEW ixmIndexKeyGen ( _shardingKey ) ;
      PD_CHECK ( _pKeyGen, SDB_OOM, error, PDERROR,
                 "Failed to alloc memory for KeyGen" ) ;

      //need to update map and also the vector, usually CATALOGINFO field is
      //required, however for data node we didn't pass range from catalog in
      //order to reduce network traffic. Therefore if we do not find cataloginfo
      //field, it doesn't represent something goes wrong.
      ele = catSet.getField( CAT_CATALOGINFO_NAME ) ;
      if ( ele.eoo () )
      {
         goto done ;
      }
      PD_CHECK ( ele.type() == Array, SDB_CAT_CORRUPTION, error,
                 PDSEVERE, "Catalog [%s] type error, type: %d",
                 CAT_CATALOGINFO_NAME, ele.type() ) ;
      // parse ranges!!!
      {
         clsCatalogItem *cataItem = NULL ;
         BSONObj objCataInfo = ele.embeddedObject() ;
         // sanity check, if we are not sharded we should only see 1 field
         PD_CHECK ( isSharding() || objCataInfo.nFields() == 1,
                    SDB_CAT_CORRUPTION, error, PDSEVERE,
                    "The catalog info must be 1 item when not sharding" ) ;

         // loop through each element
         BSONObjIterator objItr ( objCataInfo ) ;
         while ( objItr.more () )
         {
            BSONElement eleCataItem = objItr.next () ;
            // each element has to be object type
            PD_CHECK ( !eleCataItem.eoo() && eleCataItem.type() == Object,
                       SDB_CAT_CORRUPTION, error, PDSEVERE,
                       "CataItem type error, type: %d", eleCataItem.type() ) ;

            // new Item
            cataItem = SDB_OSS_NEW _clsCatalogItem( _saveName, _isMainCL ) ;
            PD_CHECK ( cataItem, SDB_OOM, error, PDERROR,
                       "Failed to alloc memory for cataItem" ) ;

            // set the item
            rc = cataItem->updateItem( eleCataItem.embeddedObject(),
                                       isSharding(), isHashSharding() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG ( PDERROR, "Failed to update cataItem, rc = %d", rc ) ;
               SDB_OSS_DEL cataItem ;
               goto error ;
            }

            if ( _maxID < cataItem->getID() )
            {
               _maxID = cataItem->getID() ;
            }

            // make sure the created cata item is for the filter group
            if ( groupID != 0 && groupID != cataItem->getGroupID()
               && !_isMainCL )
            {
               // remove the record and loop to next
               // unset _isWholeRange
               SDB_OSS_DEL cataItem ;
               _isWholeRange = FALSE ;
               continue ;
            }

            // add to map
            rc = _addItem( cataItem ) ;
            if ( rc )
            {
               SDB_OSS_DEL cataItem ;
               goto error ;
            }

            if ( !_isMainCL )
            {
               // add group id
               rc = _addGroupID ( cataItem->getGroupID() ) ;
               if ( rc )
               {
                  goto error ;
               }
            }
            else
            {
               // add sub-collection name
               rc = _addSubClName( cataItem->getID(),
                                   cataItem->getSubClName() );
               if ( rc )
               {
                  goto error ;
               }
            }
         }
      }

      // check the last item, and see if it's including MaxKey. If all fields
      // are MaxKey, we know it must be the latest range, so we set _lastItem
      {
         // last item
         MAP_CAT_ITEM::reverse_iterator rit = _mapItems.rbegin() ;
         if ( rit != _mapItems.rend() &&
              _isObjAllMaxKey( rit->second->getUpBound() ) )
         {
            _lastItem = rit->second ;
            _lastItem->_isLast = TRUE ;
         }
         else
         {
            _lastItem = NULL ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSCT_UDCATSET, rc ) ;
      return rc ;
   error:
      PD_LOG ( PDERROR, "Update cataSet: %s", catSet.toString().c_str() ) ;
      goto done ;
   }

   _clsCatalogSet *_clsCatalogSet::next ()
   {
      return _next ;
   }

   INT32 _clsCatalogSet::next ( _clsCatalogSet * next )
   {
      _next = next ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET__ISOBJALLMK, "_clsCatalogSet::_isObjAllMaxKey" )
   BOOLEAN _clsCatalogSet::_isObjAllMaxKey ( const BSONObj &obj )
   {
      PD_TRACE_ENTRY ( SDB__CLSCTSET__ISOBJALLMK ) ;
      BOOLEAN ret = TRUE ;
      if ( isRangeSharding() )
      {
         INT32 index = 0 ;
         INT32 dir = 1 ;
         const Ordering *pOrder = getOrdering () ;
         BSONObjIterator itr ( obj ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( pOrder )
            {
               dir = pOrder->get ( index++ ) ;
            }

            if ( 1 == dir && ele.type() != MaxKey )
            {
               ret = FALSE ;
               goto done ;
            }
            else if ( -1 == dir && ele.type() != MinKey )
            {
               ret = FALSE ;
               goto done ;
            }
         }
      }
      else if ( isHashSharding() )
      {
         SDB_ASSERT( !obj.isEmpty(), "impossible" ) ;
         if ( !obj.isEmpty() )
         {
            BSONElement ele = obj.firstElement() ;
            SDB_ASSERT( ele.isNumber(), "impossible" ) ;
            if ( !ele.eoo() && ele.isNumber() )
            {
               if ( ele.Number() < _partition )
               {
                  ret = FALSE ;
                  goto done ;
               }
            }
         }
      }
      else
      {
         ret = TRUE ;
      }
   done :
      PD_TRACE_EXIT ( SDB__CLSCTSET__ISOBJALLMK ) ;
      return ret ;
   }

   BOOLEAN _clsCatalogSet::isMainCL() const
   {
      return _isMainCL;
   }

   INT32 _clsCatalogSet::getLobShardingKeyFormat()
   {
      return _lobShardingKeyFormat ;
   }

   INT32 _clsCatalogSet::getSubCLList( CLS_SUBCL_LIST &subCLLst,
                                       CLS_SUBCL_SORT_TYPE sortType )
   {
      try
      {
         subCLLst.reserve( _mapItems.size() ) ;

         /// sort by id desc, newest sub cl in first
         if ( SUBCL_SORT_BY_ID == sortType )
         {
            std::multimap<UINT32, std::string>::reverse_iterator rit =
               _subCLList.rbegin() ;
            while( rit != _subCLList.rend() )
            {
               subCLLst.push_back( rit->second ) ;
               ++rit ;
            }
         }
         else
         {
            MAP_CAT_ITEM_IT it = _mapItems.begin() ;
            while( it != _mapItems.end() )
            {
               subCLLst.push_back( it->second->getSubClName() ) ;
               ++it ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         return SDB_OOM ;
      }
      return SDB_OK;
   }

   BOOLEAN _clsCatalogSet::isContainSubCL( const string &subCLName ) const
   {
      std::multimap<UINT32, std::string>::const_iterator it = _subCLList.begin() ;
      while( it != _subCLList.end() )
      {
         if ( 0 == subCLName.compare( it->second ) )
         {
            return TRUE ;
         }
         ++it ;
      }
      return FALSE ;
   }

   INT32 _clsCatalogSet::getSubCLCount () const
   {
      return _subCLList.size() ;
   }

   const string& _clsCatalogSet::getMainCLName() const
   {
      return _mainCLName ;
   }

   BOOLEAN _clsCatalogSet::_checkLobBound( const BSONObj &boundObj,
                                           BSONObj &lobBoundObj )
   {
      if ( boundObj.nFields() != 1 )
      {
         return FALSE ;
      }

      BSONType type = boundObj.firstElement().type() ;
      if ( String == type )
      {
         if ( SDB_OK != clsGetLobBound( _lobShardingKeyFormat, boundObj,
                                        lobBoundObj ) )
         {
            return FALSE ;
         }

         return TRUE ;
      }

      if ( type == MinKey )
      {
         BSONObjBuilder builder ;
         builder.appendMinKey( FIELD_NAME_LOB_OID ) ;
         lobBoundObj = builder.obj() ;
         return TRUE ;
      }

      if ( type == MaxKey )
      {
         BSONObjBuilder builder ;
         builder.appendMaxKey( FIELD_NAME_LOB_OID ) ;
         lobBoundObj = builder.obj() ;
         return TRUE ;
      }

      return FALSE ;
   }

   INT32 _clsCatalogSet::addSubCL ( const CHAR *subCLName,
                                    const BSONObj &lowBound,
                                    const BSONObj &upBound,
                                    clsCatalogItem **ppItem )
   {
      INT32 rc = SDB_OK;
      BSONObj lowBoundObj;
      BSONObj upBoundObj;
      clsCatalogItem *pNewItem = NULL;
      MAP_CAT_ITEM::iterator iter;
      BOOLEAN canMergerRight = FALSE;
      BOOLEAN canMergerLeft = FALSE;
      MAP_CAT_ITEM::iterator iterRight;
      MAP_CAT_ITEM::iterator iterLeft;

      // generate boundary and ignore the unrelated fields
      rc = genKeyObj( lowBound, lowBoundObj );
      PD_RC_CHECK( rc, PDERROR, "failed to get low-bound(rc=%d)", rc );
      rc = genKeyObj( upBound, upBoundObj );
      PD_RC_CHECK( rc, PDERROR, "failed to get up-bound(rc=%d)", rc );

      // build new item
      pNewItem = SDB_OSS_NEW clsCatalogItem( FALSE, TRUE );
      PD_CHECK( pNewItem, SDB_OOM, error, PDERROR, "malloc failed!" );
      pNewItem->_lowBound = lowBoundObj;
      pNewItem->_upBound = upBoundObj;
      pNewItem->_subCLName = subCLName;
      pNewItem->_ID = ++_maxID ;

      if ( _lobShardingKeyFormat != SDB_TIME_INVALID )
      {
         BSONObj lobLowBound ;
         BSONObj lobUpBound ;

         if ( !_checkLobBound(lowBoundObj, lobLowBound)
              || !_checkLobBound(upBoundObj, lobUpBound) )
         {
            rc = SDB_BOUND_INVALID ;
            PD_LOG( PDERROR, "LowBound or upBound in invalid in lob MainCL:"
                    "lowBound=%s,upBound=%s", lowBoundObj.toString().c_str(),
                    upBoundObj.toString().c_str() ) ;
            goto error ;
         }
      }

      // check if the new boundary is conflict with existing boundary
      {
      clsCataItemKey newItemLowKey = pNewItem->getLowBoundKey( _pOrder->getOrdering() );
      clsCataItemKey newItemUpKey = pNewItem->getUpBoundKey( _pOrder->getOrdering() );
      PD_CHECK( newItemLowKey < newItemUpKey, SDB_BOUND_INVALID, error, PDERROR,
               "invalid boundary(low:%s, up:%s)",
               lowBoundObj.toString().c_str(), upBoundObj.toString().c_str() );

      // empty: directly insert the new item
      if ( _mapItems.empty() )
      {
         rc = _addItem( pNewItem );
         PD_RC_CHECK( rc, PDERROR,
                     "Failed to add the new item(rc=%d)",
                     rc );
         goto done;
      }

      iter = _mapItems.upper_bound( newItemUpKey );

      // check the right adjacent boundary
      if ( iter != _mapItems.end() )
      {
         // now: iter->_upBound > pNewItem->_upBound.
         // so there must be intersection if iter->_lowBound < pNewItem->_upBound
         // note: here is use "<" not "<=" because the boundary is left close
         // and right open( ex: [ a, b ) )
         clsCataItemKey rightLowKey
                     = iter->second->getLowBoundKey( _pOrder->getOrdering() );
         if ( rightLowKey < newItemUpKey )
         {
            rc = SDB_BOUND_CONFLICT;
            PD_LOG( PDERROR,
                  "the boundary of new sub-collection(%s) is conflict "
                  "with the sub-collection(%s)",
                  subCLName, iter->second->_subCLName.c_str() );
            goto error;
         }
         else if ( rightLowKey == newItemUpKey
                  && 0 == iter->second->getSubClName().compare(
                           pNewItem->_subCLName ))
         {
            iterRight = iter;
            canMergerRight = TRUE;
         }
      }
      if ( iter != _mapItems.begin() )
      {
         --iter;
         // now,iter is the left adjacent boundary element,
         // and iter->_upBound < pNewItem->_upBound.
         // so there must be intersection if iter->_upBound > pNewItem->_lowBound
         // note: here is use ">" not ">=" because the boundary is left close
         // and right open( ex: [ a, b ) )
         clsCataItemKey leftUpKey
                  = iter->second->getUpBoundKey( _pOrder->getOrdering() );
         if ( leftUpKey > newItemLowKey )
         {
            rc = SDB_BOUND_CONFLICT;
            PD_LOG( PDERROR,
                  "the boundary of new sub-collection(%s) is conflict "
                  "with the sub-collection(%s)",
                  subCLName, iter->second->_subCLName.c_str() );
            goto error;
         }
         else if ( leftUpKey == newItemLowKey
                  && 0 == iter->second->getSubClName().compare(
                           pNewItem->_subCLName ))
         {
            iterLeft = iter;
            canMergerLeft = TRUE;
         }
      }
      }

      if ( canMergerLeft )
      {
         clsCatalogItem *item = iterLeft->second ;
         pNewItem->_lowBound = item->getLowBound();
         _mapItems.erase( iterLeft );
         SAFE_OSS_DELETE( item ) ;
      }
      if ( canMergerRight )
      {
         clsCatalogItem *item = iterRight->second ;
         pNewItem->_upBound = item->getUpBound();
         _mapItems.erase( iterRight );
         SAFE_OSS_DELETE( item ) ;
      }
      rc = _addItem( pNewItem );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to add the new item(rc=%d)",
                   rc );

      if ( ppItem )
      {
         (*ppItem) = pNewItem ;
      }

   done:
      return rc;
   error:
      if ( pNewItem != NULL )
      {
         SDB_OSS_DEL pNewItem;
      }
      goto done;
   }

   INT32 _clsCatalogSet::delSubCL ( const CHAR *subCLName )
   {
      MAP_CAT_ITEM_IT it = _mapItems.begin() ;
      while ( it != _mapItems.end() )
      {
         string strSubClName = it->second->getSubClName();
         if ( 0 == strSubClName.compare( subCLName ) )
         {
            clsCatalogItem *item = it->second ;
            _mapItems.erase( it++ ) ;
            SAFE_OSS_DELETE( item ) ;
         }
         else
         {
            ++it ;
         }
      }
      return SDB_OK ;
   }

   INT32 _clsCatalogSet::renameSubCL ( const CHAR *subCLName,
                                       const CHAR* newSubCLName )
   {
      MAP_CAT_ITEM_IT it = _mapItems.begin() ;
      while ( it != _mapItems.end() )
      {
         const string& strSubClName = it->second->getSubClName();
         if ( 0 == strSubClName.compare( subCLName ) )
         {
            clsCatalogItem *item = it->second ;
            item->renameSubClName( newSubCLName ) ;
         }
         else
         {
            ++it ;
         }
      }
      return SDB_OK ;
   }

   INT32 _clsCatalogSet::getSubCLBounds ( const std::string &subCLName,
                                          BSONObj &lowBound,
                                          BSONObj &upBound ) const
   {
      MAP_CAT_ITEM::const_iterator it = _mapItems.begin() ;
      while ( it != _mapItems.end() )
      {
         clsCatalogItem *item = it->second ;
         string strSubClName = item->getSubClName() ;
         if ( 0 == strSubClName.compare( subCLName ) )
         {
            lowBound = item->getLowBound() ;
            upBound = item->getUpBound() ;
            return SDB_OK ;
         }
         else
         {
            ++it ;
         }
      }
      return SDB_CAT_NO_MATCH_CATALOG ;
   }

   const clsAutoIncSet* _clsCatalogSet::getAutoIncSet() const
   {
      return &_autoIncSet ;
   }

   clsAutoIncSet* _clsCatalogSet::getAutoIncSet()
   {
      return &_autoIncSet ;
   }

   INT32 _clsCatalogSet::findLobSubCLName( const _utilLobID &lobID,
                                           string &subCLName )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      CHAR szTimestmpStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;

      if ( SDB_TIME_INVALID == _lobShardingKeyFormat )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Operation only support in a Lob MainCL:cl=%s,"
                 "cata=%s", name(), toCataInfoBson().toString().c_str() ) ;
         goto error ;
      }

      rc = clsGetLobTimeStr( lobID.getSeconds(), _lobShardingKeyFormat,
                             szTimestmpStr, OSS_TIMESTAMP_STRING_LEN ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get lobTimeStr:lobShardingKeyFormat=%d,"
                 "cl=%s,cata=%s", _lobShardingKeyFormat, name(),
                 toCataInfoBson().toString().c_str() ) ;
         goto error ;
      }

      builder.append( _shardingKey.firstElement().fieldName(),
                      szTimestmpStr ) ;
      obj = builder.obj() ;

      rc = findSubCLName ( obj, subCLName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Operation only support in a Lob MainCL:cl=%s,"
                 "cata=%s", name(), toCataInfoBson().toString().c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsCatalogSet::findSubCLName ( const BSONObj &obj,
                                         string &subCLName )
   {
      INT32 rc = SDB_OK;

      _clsCatalogItem *item = NULL;
      rc = findItem( obj, item );
      if ( rc != SDB_OK )
      {
         goto error;
      }
      subCLName = item->getSubClName();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _clsCatalogSet::_rewriteOidField( const BSONElement &oidEle,
                                           BSONArrayBuilder &arrayBuilder )
   {
      INT32 rc = SDB_OK ;
      INT64 seconds ;
      BYTE oidArray[ UTIL_LOBID_ARRAY_LEN ] ;
      CHAR timeStr[ OSS_TIMESTAMP_STRING_LEN ] ;
      bson::OID oid = oidEle.OID() ;

      oid.toByteArray( oidArray, UTIL_LOBID_ARRAY_LEN ) ;

      rc = _utilLobID::parseSeconds( oidArray, UTIL_LOBID_ARRAY_LEN, seconds ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to parse seconds from oid[%s]",
                   oid.toString().c_str() ) ;

      rc = clsGetLobTimeStr( seconds, _lobShardingKeyFormat, timeStr,
                             OSS_TIMESTAMP_STRING_LEN ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get lob timeStr:seconds=%lld,"
                   "_lobShardingKeyFormat=%d", seconds, _lobShardingKeyFormat ) ;

      arrayBuilder.append( timeStr ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsCatalogSet::_rewriteOidField( const BSONElement &oidEle,
                                           const CHAR *fieldName,
                                           BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      INT64 seconds ;
      BYTE oidArray[ UTIL_LOBID_ARRAY_LEN ] ;
      CHAR timeStr[ OSS_TIMESTAMP_STRING_LEN ] ;
      bson::OID oid = oidEle.OID() ;

      oid.toByteArray( oidArray, UTIL_LOBID_ARRAY_LEN ) ;

      rc = _utilLobID::parseSeconds( oidArray, UTIL_LOBID_ARRAY_LEN, seconds ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to parse seconds from oid[%s]",
                   oid.toString().c_str() ) ;

      rc = clsGetLobTimeStr( seconds, getLobShardingKeyFormat(), timeStr,
                             OSS_TIMESTAMP_STRING_LEN ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get lob timeStr:seconds=%lld,"
                   "_lobShardingKeyFormat=%d", seconds, _lobShardingKeyFormat ) ;

      builder.append( fieldName, timeStr ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsCatalogSet::_rewriteMatcherForLob( const BSONObj &matcher,
                                                BSONArrayBuilder &arrayBuilder )
   {
      INT32 rc = SDB_OK ;

      BSONObjIterator iter( matcher ) ;
      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         if ( Object == ele.type() )
         {
            BSONObjBuilder builder ;
            rc = _rewriteMatcherForLob( ele.embeddedObject(), builder ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to rewrite lob matcher(%s),"
                         "rc=%d", ele.toString().c_str(), rc ) ;
            arrayBuilder.append( builder.obj() ) ;
         }
         else if ( Array == ele.type() )
         {
            BSONArrayBuilder subArrayBuilder ;
            rc = _rewriteMatcherForLob( ele.embeddedObject(), subArrayBuilder ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to rewrite lob matcher(%s),"
                         "rc=%d", ele.toString().c_str(), rc ) ;
            arrayBuilder.append( subArrayBuilder.arr() ) ;
         }
         else
         {
            arrayBuilder.append( ele ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsCatalogSet::_rewriteMatcherForLob( const BSONObj &matcher,
                                                BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObjIterator iter( matcher ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            if ( 0 != ossStrcmp( ele.fieldName(), FIELD_NAME_LOB_OID ) )
            {
               if ( Array == ele.type() )
               {
                  BSONArrayBuilder subArrayBuilder(
                                    builder.subarrayStart( ele.fieldName() ) ) ;
                  rc = _rewriteMatcherForLob( ele.embeddedObject(),
                                              subArrayBuilder ) ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }

                  subArrayBuilder.done() ;
               }
               else if ( Object == ele.type() )
               {
                  BSONObjBuilder subObjBuidler(
                                    builder.subobjStart( ele.fieldName() ) ) ;
                  rc = _rewriteMatcherForLob( ele.embeddedObject(),
                                              subObjBuidler ) ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }

                  subObjBuidler.done() ;
               }
               else
               {
                  builder.append( ele ) ;
               }

               continue ;
            }

            // ele.fieldName() == FIELD_NAME_LOB_OID
            if ( jstOID == ele.type() )
            {
               rc = _rewriteOidField( ele,
                                      _shardingKey.firstElement().fieldName(),
                                      builder ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
            }
            else if ( Object == ele.type() )
            {
               // { Oid: { $lt :{ $id:"xxx" } } } => { ShardingKeyName: {$lt :"201907"}}
               BSONObjBuilder subObjBuidler( builder.subobjStart(
                                   _shardingKey.firstElement().fieldName() ) ) ;
               BSONObj subObj = ele.embeddedObject() ;
               BSONObjIterator subIter( subObj ) ;
               while ( subIter.more() )
               {
                  BSONElement subEle = subIter.next() ;
                  const CHAR* subFieldName = subEle.fieldName() ;
                  // should be $lt or other $xx
                  if ( MTH_OPERATOR_EYECATCHER != subFieldName[0] )
                  {
                     rc = SDB_INVALIDARG ;
                     goto error ;
                  }

                  if ( jstOID == subEle.type() )
                  {
                     // { Oid: { $lt :{ $id:"xxx" } } }
                     rc = _rewriteOidField( subEle, subFieldName,
                                            subObjBuidler ) ;
                     if ( SDB_OK != rc )
                     {
                        goto error ;
                     }
                  }
                  else if ( Array == subEle.type() )
                  {
                     // { Oid: { $in :[ { $id:"xxx" }... ] } }
                     BSONArrayBuilder opArrayBuidler(
                           subObjBuidler.subarrayStart( subEle.fieldName() ) ) ;
                     BSONObjIterator opArrayIter( subEle.embeddedObject() ) ;
                     while ( opArrayIter.more() )
                     {
                        BSONElement arrayItemEle = opArrayIter.next() ;
                        if ( jstOID == arrayItemEle.type() )
                        {
                           // { $id:"xxx" }
                           rc = _rewriteOidField( arrayItemEle, opArrayBuidler ) ;
                           if ( SDB_OK != rc )
                           {
                              goto error ;
                           }
                        }
                        else
                        {
                           // other format is unreconigzed
                           rc = SDB_INVALIDARG ;
                           goto error ;
                        }
                     }

                     opArrayBuidler.done() ;
                  }
               }

               subObjBuidler.done() ;
            }
            else
            {
               //unreconigzed matcher: { Oid:[ xx, xx ] }
               rc = SDB_INVALIDARG ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to rewrite matcher: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsCatalogSet::findLobSubCLNamesByMatcher( const BSONObj *matcher,
                                                     CLS_SUBCL_LIST &subCLList,
                                                     CLS_SUBCL_SORT_TYPE sortType )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      BSONObj newMatcher ;

      if ( SDB_TIME_INVALID == getLobShardingKeyFormat() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "The collection[%s] is not lob maincl",
                 name() ) ;
         goto error ;
      }

      if ( _mapItems.empty() )
      {
         PD_LOG( PDWARNING, "The collection[%s]'s cataItem is empty",
                 name() ) ;
         rc = SDB_CAT_NO_MATCH_CATALOG ;
         goto error ;
      }

      if ( NULL == matcher || matcher->isEmpty() )
      {
         goto getall ;
      }

      try
      {
         rc = _rewriteMatcherForLob( *matcher, builder ) ;
         if ( SDB_OK != rc )
         {
            goto getall ;
         }

         newMatcher = builder.obj() ;
         if ( newMatcher.isEmpty() )
         {
            goto getall ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = findSubCLNames( newMatcher, subCLList, sortType ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Match catalog item failed, rc: %d", rc ) ;
         goto error ;
      }
      else
      {
         goto done ;
      }

   getall:
      rc = getSubCLList( subCLList, sortType ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get subcl list, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsCatalogSet::findSubCLNames( const BSONObj &matcher,
                                         CLS_SUBCL_LIST &subCLList,
                                         CLS_SUBCL_SORT_TYPE sortType )
   {
      INT32 rc = SDB_OK ;
      ossPoolMultiMap<UINT32, clsCatalogItem* > mapSubItem ;

      if ( !isMainCL() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "The collection[%s] is not collection-partitioned",
                 name() ) ;
         goto error ;
      }
      else if ( _mapItems.empty() )
      {
         PD_LOG( PDWARNING, "The collection[%s]'s cataItem is empty", name() ) ;
         rc = SDB_CAT_NO_MATCH_CATALOG ;
         goto error ;
      }

      subCLList.clear() ;

      try
      {
         if ( isHashSharding() )
         {
            subCLList.reserve( _mapItems.size() ) ;
            MAP_CAT_ITEM::iterator iter = _mapItems.begin() ;
            while( iter != _mapItems.end() )
            {
               if ( SUBCL_SORT_BY_ID == sortType )
               {
                  mapSubItem.insert( std::make_pair( iter->second->getID(),
                                                     iter->second ) ) ;
               }
               else
               {
                  subCLList.push_back( iter->second->getSubClName() ) ;
               }
               ++iter ;
            }
         }
         else
         {
            MAP_CAT_ITEM mapBoundItem ;
            CLS_SET_CATAITEM setItem ;
            CLS_SET_CATAITEM::iterator itSet ;
            clsCatalogMatcher clsMatcher( _shardingKey, isHashSharding() ) ;
            rc = clsMatcher.loadPattern( matcher ) ;
            PD_RC_CHECK( rc, PDERROR, "Load matcher failed, rc: %d", rc ) ;

            if ( clsMatcher.isUniverse() )
            {
               rc = getSubCLList( subCLList, sortType ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to get subcl list, rc: %d", rc ) ;
                  goto error ;
               }
               goto done ;
            }

            rc = clsMatcher.calc( this, setItem ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Calc matcher failed, rc: %d", rc ) ;
               goto error ;
            }

            if ( setItem.empty() )
            {
               rc = SDB_CAT_NO_MATCH_CATALOG ;
               PD_LOG( PDWARNING, "Couldn't find the match catalog, rc: %d",
                       rc ) ;
               goto error ;
            }

            itSet = setItem.begin() ;
            while ( itSet != setItem.end() )
            {
               if ( SUBCL_SORT_BY_ID == sortType )
               {
                  mapSubItem.insert( std::make_pair( (*itSet)->getID(),
                                                     (clsCatalogItem*)(*itSet) ) ) ;
               }
               else
               {
                  mapBoundItem.insert( std::make_pair( (*itSet)->getUpBoundKey( getOrdering() ),
                                                       (clsCatalogItem*)(*itSet) ) ) ;
               }
               ++itSet ;
            }

            if ( SUBCL_SORT_BY_ID == sortType )
            {
               subCLList.reserve( mapSubItem.size() ) ;

               ossPoolMultiMap<UINT32, clsCatalogItem* >::reverse_iterator rit ;
               rit = mapSubItem.rbegin() ;
               while ( rit != mapSubItem.rend() )
               {
                  subCLList.push_back( rit->second->getSubClName() ) ;
                  ++rit ;
               }
            }
            else
            {
               subCLList.reserve( mapBoundItem.size() ) ;

               MAP_CAT_ITEM::iterator it ;
               it = mapBoundItem.begin() ;
               while ( it != mapBoundItem.end() )
               {
                  subCLList.push_back( it->second->getSubClName() ) ;
                  ++it ;
               }
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
   note: _clsCatalogAgent implement
   */
   _clsCatalogAgent::_clsCatalogAgent ()
   {
      _catVersion = -1 ;
   }

   _clsCatalogAgent::~_clsCatalogAgent ()
   {
      clearAll () ;
   }

   INT32 _clsCatalogAgent::catVersion ()
   {
      return _catVersion ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTAGENT_GETALLNAMES, "_clsCatalogAgent::getAllNames" )
   INT32 _clsCatalogAgent::getAllNames( std::vector<string> &names )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCTAGENT_GETALLNAMES ) ;
      CAT_MAP_IT itr = _mapCatalog.begin() ;

      try
      {
         names.reserve( _mapCatalog.size() ) ;
         for ( ; itr != _mapCatalog.end(); itr++ )
         {
            _clsCatalogSet *set = itr->second ;
            do
            {
               names.push_back( set->nameStr() ) ;
               set = set->next() ;
            } while ( NULL != set ) ;
         }
      }
      catch( std::exception &e )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      PD_TRACE_EXITRC ( SDB__CLSCTAGENT_GETALLNAMES, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTAGENT_CLVS, "_clsCatalogAgent::collectionVersion" )
   INT32 _clsCatalogAgent::collectionVersion ( const CHAR* name )
   {
      PD_TRACE_ENTRY ( SDB__CLSCTAGENT_CLVS ) ;
      _clsCatalogSet *set = collectionSet ( name ) ;
      if ( set )
      {
         return set->getVersion () ;
      }

      PD_TRACE_EXIT ( SDB__CLSCTAGENT_CLVS ) ;
      return SDB_CLS_NO_CATALOG_INFO ;
   }

   INT32 _clsCatalogAgent::collectionW ( const CHAR * name )
   {
      _clsCatalogSet *set = collectionSet ( name ) ;
      if ( set )
      {
         return (INT32)set->getW () ;
      }

      return SDB_CLS_NO_CATALOG_INFO ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTAGENT_CLINFO, "_clsCatalogAgent::collectionInfo" )
   INT32 _clsCatalogAgent::collectionInfo ( const CHAR * name , INT32 &version,
                                            UINT32 &w )
   {
      PD_TRACE_ENTRY ( SDB__CLSCTAGENT_CLINFO ) ;
      _clsCatalogSet *set = collectionSet ( name ) ;
      if ( set )
      {
         version = set->getVersion () ;
         w = set->getW () ;
         return SDB_OK ;
      }

      PD_TRACE_EXIT ( SDB__CLSCTAGENT_CLINFO ) ;
      return SDB_CLS_NO_CATALOG_INFO ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTAGENT_CLSET, "_clsCatalogAgent::collectionSet" )
   clsCatalogSet *_clsCatalogAgent::collectionSet ( const CHAR* name,
                                                    utilCLUniqueID clUniqueID )
   {
      PD_TRACE_ENTRY ( SDB__CLSCTAGENT_CLSET ) ;
      _clsCatalogSet *set = NULL ;

      // first find by id, if findout nothing, then find by name
      if ( UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
      {
         ID_CAT_MAP_IT it = _mapIDCatalog.find ( clUniqueID ) ;
         if ( it != _mapIDCatalog.end() )
         {
            set = it->second ;
            goto done ;
         }
      }

      if ( NULL == set && name )
      {
         UINT32 hashCode = ossHash ( name ) ;
         CAT_MAP_IT it = _mapCatalog.find ( hashCode ) ;
         if ( it != _mapCatalog.end() )
         {
            set = it->second ;
            while ( set )
            {
               if ( ossStrcmp ( set->name(), name ) == 0 )
               {
                  goto done ;
               }
               set = set->next () ;
            }
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSCTAGENT_CLSET ) ;
      return set ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTAGENT__ADDCLSET, "_clsCatalogAgent::_addCollectionSet" )
   _clsCatalogSet *_clsCatalogAgent::_addCollectionSet ( const CHAR * name,
                                                         utilCLUniqueID clUniqueID )
   {
      PD_TRACE_ENTRY ( SDB__CLSCTAGENT__ADDCLSET ) ;
      BOOLEAN addName = FALSE ;

      _clsCatalogSet *catSet = SDB_OSS_NEW _clsCatalogSet ( name,
                                                            FALSE,
                                                            clUniqueID) ;
      if ( !catSet )
      {
         return NULL ;
      }

      UINT32 hashCode = ossHash ( name ) ;

      try
      {
         CAT_MAP_IT it = _mapCatalog.find ( hashCode ) ;
         if ( it == _mapCatalog.end() )
         {
            _mapCatalog[hashCode] = catSet ;
         }
         else
         {
            //add to the last
            _clsCatalogSet * rootSet = it->second ;
            while ( rootSet->next() )
            {
               rootSet = rootSet->next () ;
            }
            rootSet->next ( catSet ) ;
         }
         addName = TRUE ;

         if ( UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
         {
            _mapIDCatalog[ clUniqueID ] = catSet ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         if ( addName )
         {
            clear( name ) ;
         }
         else
         {
            SDB_OSS_DEL catSet ;
            catSet = NULL ;
         }
      }

      PD_TRACE_EXIT ( SDB__CLSCTAGENT__ADDCLSET ) ;
      return catSet ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTAGENT_UPCT, "_clsCatalogAgent::updateCatalog" )
   INT32 _clsCatalogAgent::updateCatalog ( INT32 version, UINT32 groupID,
                                           const CHAR* objdata, UINT32 length,
                                           _clsCatalogSet **ppSet )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCTAGENT_UPCT ) ;
      _catVersion = version ;
      _clsCatalogSet *catSet = NULL ;

      if ( !objdata || length == 0 )
      {
         return SDB_INVALIDARG ;
      }

      try
      {
         BSONObj catalog ( objdata ) ;
         string clName ;
         UINT64 clUniqueID = UTIL_UNIQUEID_NULL ;

         /// get cl name and cl id
         BSONElement ele = catalog.getField ( CAT_COLLECTION_NAME ) ;
         if ( ele.type() != String )
         {
            rc = SDB_SYS ;
            PD_LOG ( PDERROR, "collection name type[%d] error", ele.type() ) ;
            goto error ;
         }
         clName = ele.str() ;

         ele = catalog.getField ( CAT_CL_UNIQUEID ) ;
         if ( ele.eoo() )
         {
            // it is ok, catalog hasn't been upgraded to new version.
         }
         else if ( NumberLong == ele.type() )
         {
            clUniqueID = ( utilCLUniqueID ) ele.numberLong() ;
         }
         else
         {
            rc = SDB_SYS ;
            PD_LOG ( PDERROR, "collection id type[%d] error", ele.type() ) ;
            goto error ;
         }

         /// add cata info to cache map
         catSet = collectionSet ( clName.c_str(), clUniqueID ) ;
         if ( catSet )
         {
            if ( ossStrcmp( clName.c_str(), catSet->name() ) != 0 )
            {
               /// name is not the same
               clear( catSet->name() ) ;
               catSet = NULL ;
            }
            else if ( clUniqueID != catSet->clUniqueID() )
            {
               /// id is not the same
               clear( clName.c_str() ) ;
               catSet = NULL ;
            }
         }

         if ( !catSet )
         {
            catSet = _addCollectionSet ( clName.c_str(), clUniqueID ) ;
         }

         if ( !catSet )
         {
            rc = SDB_OOM ;
            PD_LOG ( PDERROR, "create collection set failed" ) ;
            goto error ;
         }

         rc = catSet->updateCatSet ( catalog, groupID, FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Update catalogSet[%s] failed[rc:%d]",
                     clName.c_str(), rc ) ;
            clear( clName.c_str() ) ;
            // catSet is gone by clear()
            // go away without setting return pointer
            goto error ;
         }

         if ( ppSet )
         {
            *ppSet = catSet ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Exception to create catalog BSON: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSCTAGENT_UPCT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTAGENT_CLEAR, "_clsCatalogAgent::clear" )
   INT32 _clsCatalogAgent::clear ( const CHAR* name, CHAR* mainCLBuf,
                                   UINT32 bufSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCTAGENT_CLEAR ) ;
      _clsCatalogSet *preSet = NULL ;
      _clsCatalogSet *curSet = NULL ;
      UINT32 hashCode = ossHash ( name ) ;
      utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;
      CAT_MAP_IT it ;

      if ( mainCLBuf && bufSize < DMS_COLLECTION_FULL_NAME_SZ )
      {
         SDB_ASSERT( FALSE, "Buff size is invalid" ) ;
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buff size(%u) should grater than %u",
                 bufSize, DMS_COLLECTION_FULL_NAME_SZ ) ;
         goto error ;
      }

      it = _mapCatalog.find ( hashCode ) ;
      if ( it != _mapCatalog.end() )
      {
         curSet = it->second ;
         while ( curSet )
         {
            if ( ossStrcmp ( curSet->name(), name ) == 0 )
            {
               if ( !curSet->getMainCLName().empty() &&
                    mainCLBuf != NULL )
               {
                  ossStrncpy( mainCLBuf, curSet->getMainCLName().c_str(),
                              DMS_COLLECTION_FULL_NAME_SZ ) ;
                  mainCLBuf[ DMS_COLLECTION_FULL_NAME_SZ ] = '\0' ;
               }
               clUniqueID = curSet->clUniqueID() ;
               break ;
            }
            preSet = curSet ;
            curSet = curSet->next () ;
         }

         if ( curSet )
         {
            if ( preSet )
            {
               preSet->next( curSet->next() ) ;
            }
            else if ( !preSet && curSet->next() )
            {
               it->second = curSet->next () ;
            }
            else
            {
               _mapCatalog.erase ( it ) ;
            }

            curSet->next ( NULL ) ;
            SDB_OSS_DEL curSet ;
         }
      }

      if ( UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
      {
         ID_CAT_MAP_IT it = _mapIDCatalog.find ( clUniqueID ) ;
         if ( it != _mapIDCatalog.end() )
         {
            _mapIDCatalog.erase ( it ) ;
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSCTAGENT_CLEAR ) ;
      return rc ;
   error:
      goto done ;
   }

   /* Description:
         All collections belonging to the cs, clear their catalog info.
         All maincl of the collection of the cs, clear their catalog info.
      Return value:
         pSubCLs: subcl of the collection of the cs, but excluding collections
                  belonging to this cs
         pMainCLs: maincl of the collection of the cs, but excluding collections
                   belonging to this cs
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTAGENT_CRBYSPACENAME, "_clsCatalogAgent::clearBySpaceName" )
   INT32 _clsCatalogAgent::clearBySpaceName ( const CHAR * csName,
                                              CLS_SUBCL_LIST *pSubCLs,
                                              ossPoolSet< string > * pMainCLs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCTAGENT_CRBYSPACENAME ) ;
      _clsCatalogSet *preSet = NULL ;
      _clsCatalogSet *curSet = NULL ;
      _clsCatalogSet *tmpSet = NULL ;
      UINT32 nameLen = ossStrlen(csName) ;
      BOOLEAN itAdd  = TRUE ;
      ossPoolSet< string > mainCLList ;
      ossPoolSet< string >::iterator iterMain ;
      CAT_MAP_IT it = _mapCatalog.begin() ;
      utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;

      if ( NULL == pMainCLs )
      {
         pMainCLs = &mainCLList ;
      }

      if ( 0 == nameLen )
      {
         goto done ;
      }

      while ( it != _mapCatalog.end() )
      {
         itAdd = TRUE ;
         preSet = NULL ;
         curSet = it->second ;

         while ( curSet )
         {
            /// add sub collections
            if ( pSubCLs &&
                 0 == ossStrncmp( curSet->_mainCLName.c_str(),
                                  csName, nameLen ) &&
                 '.' == curSet->_mainCLName.at( nameLen ) )
            {
               if ( 0 == ossStrncmp( curSet->_name.c_str(), csName, nameLen ) &&
                    '.' == curSet->_name.at( nameLen ) )
               {
                  // this cl belongs to the cs
               }
               else
               {
                  try
                  {
                     pSubCLs->push_back( curSet->_name ) ;
                  }
                  catch( std::exception &e )
                  {
                     PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
                     rc = SDB_OOM ;
                  }
               }
            }

            if ( ossStrncmp ( curSet->name(), csName, nameLen ) == 0
               && (curSet->name())[nameLen] == '.' )
            {
               string strMainCL = curSet->getMainCLName() ;
               if ( !strMainCL.empty() )
               {
                  if ( 0 == ossStrncmp( strMainCL.c_str(), csName, nameLen )
                       && '.' == strMainCL.at( nameLen ) )
                  {
                     // this cl belongs to the cs
                  }
                  else
                  {
                     try
                     {
                        pMainCLs->insert( strMainCL ) ;
                     }
                     catch( std::exception &e )
                     {
                        PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
                        rc = SDB_OOM ;
                     }
                  }
               }
               // save CS unique ID for deleting
               // there might be expired unique ID for the same cs name
               clUniqueID = curSet->clUniqueID() ;

               tmpSet = curSet ;
               curSet = curSet->next () ;

               if ( preSet )
               {
                  preSet->next( curSet ) ;
               }
               else if ( !preSet && curSet )
               {
                  it->second = curSet ;
               }
               else
               {
                  _mapCatalog.erase ( it++ ) ;
                  itAdd = FALSE ;
               }

               tmpSet->next ( NULL ) ;
               SDB_OSS_DEL tmpSet ;

               if ( UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
               {
                  ID_CAT_MAP_IT it = _mapIDCatalog.find ( clUniqueID ) ;
                  if ( it != _mapIDCatalog.end() )
                  {
                     _mapIDCatalog.erase ( it ) ;
                  }
               }

               continue ;
            }

            preSet = curSet ;
            curSet = curSet->next () ;
         }

         if ( itAdd )
         {
            ++it ;
         }
      }

      // clear catalog caches for related main-collections
      iterMain = pMainCLs->begin() ;
      while ( iterMain != pMainCLs->end() )
      {
         clear( (*iterMain).c_str() ) ;
         ++iterMain ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSCTAGENT_CRBYSPACENAME, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTAGENT_CLALL, "_clsCatalogAgent::clearAll" )
   INT32 _clsCatalogAgent::clearAll ()
   {
      PD_TRACE_ENTRY ( SDB__CLSCTAGENT_CLALL ) ;
      CAT_MAP_IT it = _mapCatalog.begin () ;
      while ( it != _mapCatalog.end() )
      {
         SDB_OSS_DEL it->second ;
         ++it ;
      }
      _mapCatalog.clear ();
      _mapIDCatalog.clear ();

      PD_TRACE_EXIT ( SDB__CLSCTAGENT_CLALL ) ;
      return SDB_OK ;
   }

   INT32 _clsCatalogAgent::lock_r ( INT32 millisec )
   {
      return _rwMutex.lock_r ( millisec ) ;
   }

   INT32 _clsCatalogAgent::lock_w ( INT32 millisec )
   {
      return _rwMutex.lock_w ( millisec ) ;
   }

   INT32 _clsCatalogAgent::release_r ()
   {
      return _rwMutex.release_r () ;
   }

   INT32 _clsCatalogAgent::release_w ()
   {
      return _rwMutex.release_w () ;
   }

   /*
      _clsShardingKeySite implement
   */
   _clsShardingKeySite::_clsShardingKeySite()
   {
      _id = 1 ;
   }

   _clsShardingKeySite::~_clsShardingKeySite()
   {
   }

   UINT32 _clsShardingKeySite::registerKey( const BSONObj &shardingKey )
   {
      UINT32 retID = 0 ;
      map< BSONObj, UINT64 >::iterator it ;

      /// lock
      _mutex.get() ;
      it = _mapKey2ID.find( shardingKey ) ;
      if ( it == _mapKey2ID.end() )
      {
         retID = _id++ ;
         try
         {
            _mapKey2ID[ shardingKey ] = ossPack32To64( retID, 1 ) ;
         }
         catch( std::exception & )
         {
            /// register failed
            retID = 0 ;
         }
      }
      else
      {
         UINT32 lo = 0 ;
         ossUnpack32From64( it->second, retID, lo ) ;
         it->second = ossPack32To64( retID, lo + 1 ) ;
      }
      /// unlock
      _mutex.release() ;

      return retID ;
   }

   void _clsShardingKeySite::unregisterKey( const BSONObj &shardingKey,
                                            UINT32 skSiteID )
   {
      if ( skSiteID == 0 )
      {
         return ;
      }

      map< BSONObj, UINT64 >::iterator it ;
      /// lock
      _mutex.get() ;
      it = _mapKey2ID.find( shardingKey ) ;
      if ( it != _mapKey2ID.end() )
      {
         UINT32 hi = 0 ;
         UINT32 lo = 0 ;
         ossUnpack32From64( it->second, hi, lo ) ;
         /// when equal
         if ( skSiteID == hi )
         {
            if ( lo > 1 )
            {
               it->second = ossPack32To64( hi, lo - 1 ) ;
            }
            else
            {
               _mapKey2ID.erase( it ) ;
            }
         }
      }
      /// unlock
      _mutex.release() ;
   }

   /*
      _clsGroupItem implement
    */
   _clsGroupItem::_clsGroupItem ( UINT32 groupID )
   {
      _groupID = groupID ;
      _groupVersion = 0 ;
      _primaryPos = CLS_RG_NODE_POS_INVALID;
      _primaryNode.value = MSG_INVALID_ROUTEID ;
      _upIdentify = 0 ;
   }

   _clsGroupItem::~_clsGroupItem ()
   {
      _clear() ;
   }

   INT32 _clsGroupItem::updateGroupItem( const BSONObj & obj )
   {
      INT32 rc = SDB_OK ;
      UINT32 primary = 0 ;
      UINT32 groupID = 0 ;
      map<UINT64, _netRouteNode> groups ;

      PD_LOG( PDDEBUG, "Update groupItem[%s]", obj.toString().c_str() ) ;

      rc = msgParseCatGroupObj( obj.objdata(), _groupVersion, groupID,
                                _groupName, groups, &primary ) ;
      PD_RC_CHECK( rc, PDERROR, "parse catagroup obj failed, rc: %d", rc ) ;

      // check groupid is right
      PD_CHECK( _groupID == groupID, SDB_SYS, error, PDERROR,
                "group item[groupid:%d] update group id[%d] invalid[%s]",
                _groupID, groupID, obj.toString().c_str() ) ;

      // set primary node
      if ( primary != 0 )
      {
         _primaryNode.columns.groupID = _groupID ;
         _primaryNode.columns.nodeID = (UINT16)primary ;
         _primaryNode.columns.serviceID = 0 ;
      }

      // update groups
      rc = updateNodes( groups ) ;
      PD_RC_CHECK( rc, PDERROR, "update nodes failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   UINT32 _clsGroupItem::nodeCount ()
   {
      return _vecNodes.size() ;
   }

   const VEC_NODE_INFO* _clsGroupItem::getNodes () const
   {
      return &_vecNodes ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGPIM_GETNDID2, "_clsGroupItem::getNodeID" )
   INT32 _clsGroupItem::getNodeID ( UINT32 pos, MsgRouteID& id,
                                    MSG_ROUTE_SERVICE_TYPE type )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSGPIM_GETNDID2 ) ;
      id.value = MSG_INVALID_ROUTEID ;
      if ( pos >= _vecNodes.size() )
      {
         rc = SDB_INVALIDARG ;
         goto done;
      }
      {
      clsNodeItem& item = _vecNodes[pos] ;
      id = item._id ;
      id.columns.serviceID = (UINT16)type ;
      }
   done:
      PD_TRACE_EXIT ( SDB__CLSGPIM_GETNDID2 ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGPIM_GETNDID, "_clsGroupItem::getNodeID" )
   INT32 _clsGroupItem::getNodeID ( const std::string& hostName,
                                    const std::string& serviceName,
                                    MsgRouteID& id,
                                    MSG_ROUTE_SERVICE_TYPE type )
   {
      INT32 rc = SDB_CLS_NODE_NOT_EXIST ;
      PD_TRACE_ENTRY ( SDB__CLSGPIM_GETNDID ) ;
      id.value = MSG_INVALID_ROUTEID ;

      VEC_NODE_INFO_IT it = _vecNodes.begin() ;
      while ( it != _vecNodes.end() )
      {
         clsNodeItem& node = *it ;
         if ( ossStrcmp ( node._host, hostName.c_str() ) == 0 &&
              node._service[type] == serviceName )
         {
            id = node._id ;
            id.columns.serviceID = type ;
            rc = SDB_OK ;
            goto done;
         }
         ++it ;
      }
   done:
      PD_TRACE_EXIT ( SDB__CLSGPIM_GETNDID ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGPIM_GETNDINFO1, "_clsGroupItem::getNodeInfo" )
   INT32 _clsGroupItem::getNodeInfo ( UINT32 pos, MsgRouteID & id,
                                      string & hostName, string & serviceName,
                                      MSG_ROUTE_SERVICE_TYPE type )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSGPIM_GETNDINFO1 ) ;
      id.value = MSG_INVALID_ROUTEID ;
      if ( pos >= _vecNodes.size() )
      {
         rc = SDB_INVALIDARG ;
         goto done;
      }
      {
      clsNodeItem& item = _vecNodes[pos] ;
      id = item._id ;
      id.columns.serviceID = (UINT16)type ;
      hostName = item._host ;
      serviceName = item._service[(UINT16)type] ;
      }
   done:
      PD_TRACE_EXIT ( SDB__CLSGPIM_GETNDINFO1 ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGPIM_GETNDINFO2, "_clsGroupItem::getNodeInfo" )
   INT32 _clsGroupItem::getNodeInfo ( const MsgRouteID& id, string& hostName,
                                      string& serviceName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSGPIM_GETNDINFO2 ) ;
      INT32 pos = nodePos ( (UINT32)id.columns.nodeID ) ;
      if ( pos < 0 )
      {
         rc = pos ;
         goto done ;
      }
      {
      clsNodeItem& item = _vecNodes[pos] ;
      hostName = item._host ;
      serviceName = item._service[id.columns.serviceID] ;
      }
   done:
      PD_TRACE_EXIT ( SDB__CLSGPIM_GETNDINFO2 ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGPIM_GETNDINFO3, "_clsGroupItem::getNodeInfo" )
   INT32 _clsGroupItem::getNodeInfo ( UINT32 pos, SINT32 &status,
                                      INT32 faultTimeout )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB__CLSGPIM_GETNDINFO3 ) ;
      if ( pos >= _vecNodes.size() )
      {
         rc = SDB_INVALIDARG ;
         goto done;
      }

      {
         clsNodeItem& item = _vecNodes[pos] ;
         _rwMutex.lock_r() ;
         status = item.getStatus( (UINT64)time(NULL), faultTimeout ) ;
         _rwMutex.release_r() ;
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSGPIM_GETNDINFO3 ) ;
      return rc;
   }

   BOOLEAN _clsGroupItem::isNodeInStatus( UINT32 pos, INT32 status,
                                          INT32 faultTimeout )
   {
      /// node not exist
      if ( pos >= _vecNodes.size() )
      {
         return FALSE ;
      }

      clsNodeItem& item = _vecNodes[pos] ;
      ossScopedRWLock lock( &_rwMutex, SHARED ) ;
      return item.isInStatus( (UINT64)time(NULL), status,
                              faultTimeout ) ;
   }

   INT32 _clsGroupItem::nodePos ( UINT32 nodeID )
   {
      UINT32 index = 0 ;
      while ( index < _vecNodes.size() )
      {
         if ( (UINT32)_vecNodes[index]._id.columns.nodeID == nodeID )
         {
            return (INT32)index ;
         }
         ++index ;
      }
      return SDB_CLS_NODE_NOT_EXIST ;
   }

   clsNodeItem* _clsGroupItem::nodeItem ( UINT32 nodeID )
   {
      INT32 pos = nodePos ( nodeID ) ;
      return pos < 0 ? NULL : &_vecNodes[pos] ;
   }

   clsNodeItem* _clsGroupItem::nodeItemByPos( UINT32 pos )
   {
      if ( pos >= _vecNodes.size() )
      {
         return NULL ;
      }
      return &_vecNodes[ pos ] ;
   }

   MsgRouteID _clsGroupItem::primary ( MSG_ROUTE_SERVICE_TYPE type )
   {
      ossScopedRWLock lock( &_rwMutex, SHARED ) ;

      if ( MSG_INVALID_ROUTEID == _primaryNode.value )
      {
         return _primaryNode ;
      }
      MsgRouteID tmp ;
      tmp.value = _primaryNode.value ;
      tmp.columns.serviceID = (UINT16)type ;
      return tmp ;
   }

   void _clsGroupItem::setGroupInfo ( const std::string & name,
                                      UINT32 version, UINT32 primary )
   {
      _groupName = name ;
      _groupVersion = version ;
      if ( primary != 0 )
      {
         _primaryNode.columns.groupID = _groupID ;
         _primaryNode.columns.nodeID = (UINT16)primary ;
         _primaryNode.columns.serviceID = 0 ;
      }
   }

   INT32 _clsGroupItem::updateNodes ( std::map <UINT64, _netRouteNode> & nodes )
   {
      INT32 rc = SDB_OK ;

      _clear() ;

      std::map <UINT64, _netRouteNode>::iterator it = nodes.begin () ;
      UINT8 pos = 0 ;

      try
      {
         _vecNodes.reserve( nodes.size() ) ;
         // Add nodes
         while ( it != nodes.end() )
         {
            _netRouteNode & node = it->second ;

            // Set primary position
            if ( _primaryNode.columns.nodeID == node._id.columns.nodeID )
            {
               _primaryPos = pos ;
            }

            // By default, use the position in group array as node instance
            if ( !utilCheckInstanceID( (UINT32)node._instanceID, FALSE ) )
            {
               node._instanceID = pos + 1 ;
            }

            _vecNodes.push_back ( node ) ;

            ++ it ;
            ++ pos ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
      }

      return rc ;
   }

   void _clsGroupItem::_clear ()
   {
      _vecNodes.clear () ;
      _primaryPos = CLS_RG_NODE_POS_INVALID;
   }

   UINT32 _clsGroupItem::getPrimaryPos()
   {
      ossScopedRWLock lock( &_rwMutex, SHARED ) ;

      return _primaryPos ;
   }

   void _clsGroupItem::cancelPrimary ()
   {
      ossScopedRWLock lock( &_rwMutex, SHARED ) ;

      _primaryNode.value = MSG_INVALID_ROUTEID ;
      _primaryPos = CLS_RG_NODE_POS_INVALID ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGPIM_UPPRRIMARY, "_clsGroupItem::updatePrimary" )
   INT32 _clsGroupItem::updatePrimary ( const MsgRouteID & nodeID,
                                        BOOLEAN primary,
                                        INT32 *pPreStat )
   {
      INT32 rc = SDB_OK ;
      UINT32 index = 0 ;
      PD_TRACE_ENTRY ( SDB__CLSGPIM_UPPRRIMARY ) ;
      SDB_ASSERT ( nodeID.columns.groupID == _groupID, "group id not same" ) ;

      ossScopedRWLock lock( &_rwMutex, EXCLUSIVE ) ;

      // if cancel primary and the pimary is invalid or not the same with
      // nodeID, do nothing
      if ( ( MSG_INVALID_ROUTEID == _primaryNode.value ||
             _primaryNode.columns.nodeID != nodeID.columns.nodeID )
            && FALSE == primary )
      {
         goto done ;
      }

      // cancel the node primary
      if ( !primary )
      {
         _primaryNode.value = MSG_INVALID_ROUTEID ;
         _primaryPos = CLS_RG_NODE_POS_INVALID ;
         goto done ;
      }

      // set the node to primary
      // be sure the nodeid is exist
      while ( index < _vecNodes.size() )
      {
         if ( nodeID.columns.nodeID == _vecNodes[index]._id.columns.nodeID )
         {
            _primaryNode.columns.groupID = _groupID ;
            _primaryNode.columns.nodeID = nodeID.columns.nodeID ;
            _primaryNode.columns.serviceID = 0 ;
            _primaryPos = index ;

            if ( pPreStat )
            {
               clsNodeItem &tmpNode = _vecNodes[index] ;
               *pPreStat = ( INT32 )tmpNode.getStatus( (UINT64)time( NULL ),
                                                       NET_NODE_FAULTUP_MIN_TIME ) ;
            }
            goto done ;
         }
         ++index ;
      }
      rc = SDB_CLS_NODE_NOT_EXIST ;

      PD_LOG ( PDWARNING, "Update group primary node[%u,%u,%u] to %s failed, "
               "the node does't exist in group info.",
               nodeID.columns.groupID, nodeID.columns.nodeID,
               nodeID.columns.serviceID,
               primary ? "primary" : "slave" ) ;

   done :
      PD_TRACE_EXIT ( SDB__CLSGPIM_UPPRRIMARY ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGPIM_UPNODESTAT, "_clsGroupItem::updateNodeStat" )
   void _clsGroupItem::updateNodeStat( UINT16 nodeID,
                                       NET_NODE_STATUS status,
                                       UINT64 *pTime )
   {
      PD_TRACE_ENTRY ( SDB__CLSGPIM_UPNODESTAT ) ;
      UINT64 curTime = 0 ;
      VEC_NODE_INFO_IT it ;

      if ( pTime )
      {
         curTime = *pTime ;
      }
      else
      {
         curTime = ( UINT64 )time( NULL ) ;
      }

      it = _vecNodes.begin() ;
      while ( it != _vecNodes.end() )
      {
         if ( it->_id.columns.nodeID == nodeID )
         {
            _rwMutex.lock_w() ;
            (*it).updateStatus( status, curTime ) ;
            _rwMutex.release_w() ;

            if ( NET_NODE_STAT_NORMAL != status )
            {
               updatePrimary( it->_id, FALSE ) ;
            }
            break;
         }
         ++it ;
      }
      PD_TRACE_EXIT ( SDB__CLSGPIM_UPNODESTAT ) ;
      return ;
   }

   void _clsGroupItem::clearNodesStat()
   {
      ossScopedRWLock lock( &_rwMutex, EXCLUSIVE ) ;

      VEC_NODE_INFO_IT it = _vecNodes.begin() ;
      while ( it != _vecNodes.end() )
      {
         clsNodeItem &item = *it ;
         item.updateStatus( NET_NODE_STAT_NORMAL, 0 ) ;
         ++it ;
      }
   }

   void _clsGroupItem::inheritStat( const _clsGroupItem *pItem,
                                    UINT32 falutTimeout )
   {
      if ( pItem )
      {
         UINT64 curTime = time( NULL ) ;
         INT32 nodeStatus = 0 ;
         UINT64 nodeFalutTime = 0 ;

         const VEC_NODE_INFO* pNodes = pItem->getNodes() ;
         for ( UINT32 i = 0 ; i < pNodes->size() ; ++i )
         {
            const clsNodeItem &node = (*pNodes)[ i ] ;
            node.getStatus( nodeStatus, nodeFalutTime ) ;

            if ( NET_NODE_STAT_NORMAL != nodeStatus &&
                 curTime - nodeFalutTime <= falutTimeout )
            {
               updateNodeStat( node._id.columns.nodeID,
                               (NET_NODE_STATUS)nodeStatus,
                               &nodeFalutTime ) ;
            }
         }
      }
   }

   /*
   note: _clsNodeMgrAgent implement
   */
   _clsNodeMgrAgent::_clsNodeMgrAgent ()
   {
   }

   _clsNodeMgrAgent::~_clsNodeMgrAgent ()
   {
      clearAll () ;
   }

   INT32 _clsNodeMgrAgent::groupCount ()
   {
      return _groupMap.size() ;
   }

   INT32 _clsNodeMgrAgent::getGroupsID( VEC_UINT32 &groups )
   {
      groups.clear() ;
      GROUP_MAP_IT it = _groupMap.begin() ;

      try
      {
         groups.reserve( _groupMap.size() ) ;
         while ( it != _groupMap.end() )
         {
            groups.push_back( it->first ) ;
            ++it ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         return SDB_OOM ;
      }
      return (INT32)groups.size() ;
   }

   INT32 _clsNodeMgrAgent::getGroupsName( vector< string > &groups )
   {
      groups.clear() ;
      GROUP_NAME_MAP_IT it = _groupNameMap.begin() ;

      try
      {
         groups.reserve( _groupNameMap.size() ) ;
         while ( it != _groupNameMap.end() )
         {
            groups.push_back( it->first ) ;
            ++it ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         return SDB_OOM ;
      }
      return (INT32)_groupNameMap.size() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSNDMGRAG_GPVS, "_clsNodeMgrAgent::groupVersion" )
   INT32 _clsNodeMgrAgent::groupVersion ( UINT32 id )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSNDMGRAG_GPVS ) ;
      clsGroupItem* item = groupItem ( id ) ;
      if ( !item )
      {
         rc = SDB_CLS_NO_GROUP_INFO ;
         goto done ;
      }
      rc = item->groupVersion() ;
   done :
      PD_TRACE_EXITRC ( SDB__CLSNDMGRAG_GPVS, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLDNDMGRAGENT_GPID2NAME, "_clsNodeMgrAgent::groupID2Name" )
   INT32 _clsNodeMgrAgent::groupID2Name ( UINT32 id, std::string & name )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLDNDMGRAGENT_GPID2NAME ) ;
      clsGroupItem* item = groupItem ( id ) ;
      if ( !item )
      {
         rc = SDB_CLS_NO_GROUP_INFO ;
         goto done ;
      }
      name = item->groupName() ;
   done :
      PD_TRACE_EXITRC ( SDB__CLDNDMGRAGENT_GPID2NAME, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSNDMGRAGENT_GPNM2ID, "_clsNodeMgrAgent::groupName2ID" )
   INT32 _clsNodeMgrAgent::groupName2ID ( const CHAR * name, UINT32 & id )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSNDMGRAGENT_GPNM2ID ) ;
      clsGroupItem* item = groupItem ( name ) ;
      if ( !item )
      {
         rc = SDB_CLS_NO_GROUP_INFO ;
         goto done ;
      }
      id = item->groupID() ;
   done :
      PD_TRACE_EXITRC ( SDB__CLSNDMGRAGENT_GPNM2ID, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSNDMGRAGENT_GPNDCOUNT, "_clsNodeMgrAgent::groupNodeCount" )
   INT32 _clsNodeMgrAgent::groupNodeCount ( UINT32 id )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSNDMGRAGENT_GPNDCOUNT ) ;
      clsGroupItem* item = groupItem ( id ) ;
      if ( !item )
      {
         rc = SDB_CLS_NO_GROUP_INFO ;
         goto done ;
      }
      rc = (INT32)(item->nodeCount()) ;
   done :
      PD_TRACE_EXITRC ( SDB__CLSNDMGRAGENT_GPNDCOUNT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSNDMGRAGENT_GPPRYND, "_clsNodeMgrAgent::groupPrimaryNode" )
   INT32 _clsNodeMgrAgent::groupPrimaryNode ( UINT32 id, MsgRouteID & primary,
                                              MSG_ROUTE_SERVICE_TYPE type )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSNDMGRAGENT_GPPRYND ) ;
      clsGroupItem* item = groupItem ( id ) ;
      if ( !item )
      {
         rc = SDB_CLS_NO_GROUP_INFO ;
         goto done ;
      }
      primary = item->primary ( type ) ;
   done :
      PD_TRACE_EXITRC ( SDB__CLSNDMGRAGENT_GPPRYND, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSNDMGRAGENT_CELPRY, "_clsNodeMgrAgent::cancelPrimary" )
   INT32 _clsNodeMgrAgent::cancelPrimary( UINT32 id )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSNDMGRAGENT_CELPRY ) ;
      clsGroupItem* item = groupItem ( id ) ;
      if ( !item )
      {
         rc = SDB_CLS_NO_GROUP_INFO ;
         goto done ;
      }
      item->cancelPrimary() ;
   done :
      PD_TRACE_EXITRC ( SDB__CLSNDMGRAGENT_CELPRY, rc ) ;
      return rc ;
   }
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSNDMGRAGENT_GPIM1, "_clsNodeMgrAgent::groupItem" )
   clsGroupItem* _clsNodeMgrAgent::groupItem ( const CHAR * name )
   {
      PD_TRACE_ENTRY ( SDB__CLSNDMGRAGENT_GPIM1 ) ;
      if ( !name )
      {
         PD_TRACE_EXIT ( SDB__CLSNDMGRAGENT_GPIM1 ) ;
         return NULL ;
      }

      GROUP_NAME_MAP_IT it = _groupNameMap.find ( name ) ;
      PD_TRACE_EXIT ( SDB__CLSNDMGRAGENT_GPIM1 ) ;
      return it == _groupNameMap.end() ? NULL : groupItem ( it->second ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSNDMGRAGENT_GPIM2, "_clsNodeMgrAgent::groupItem" )
   clsGroupItem* _clsNodeMgrAgent::groupItem ( UINT32 id )
   {
      PD_TRACE_ENTRY ( SDB__CLSNDMGRAGENT_GPIM2) ;
      GROUP_MAP_IT it = _groupMap.find ( id ) ;
      PD_TRACE_EXIT ( SDB__CLSNDMGRAGENT_GPIM2 ) ;
      return it == _groupMap.end() ? NULL : it->second ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSNDMGRAGENT_CLALL, "_clsNodeMgrAgent::clearAll" )
   INT32 _clsNodeMgrAgent::clearAll ()
   {
      PD_TRACE_ENTRY ( SDB__CLSNDMGRAGENT_CLALL ) ;
      GROUP_MAP_IT it = _groupMap.begin() ;
      while ( it != _groupMap.end() )
      {
         SDB_OSS_DEL it->second ;
         ++it ;
      }
      _groupMap.clear() ;
      _groupNameMap.clear() ;
      PD_TRACE_EXIT ( SDB__CLSNDMGRAGENT_CLALL ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSNDMGRAGENT_CLGP, "_clsNodeMgrAgent::clearGroup" )
   INT32 _clsNodeMgrAgent::clearGroup ( UINT32 id )
   {
      PD_TRACE_ENTRY ( SDB__CLSNDMGRAGENT_CLGP ) ;
      BOOLEAN find = FALSE ;
      GROUP_MAP_IT it = _groupMap.find ( id ) ;
      if ( it != _groupMap.end() )
      {
         find = TRUE ;
         SDB_OSS_DEL it->second ;
         _groupMap.erase ( it ) ;
      }

      if ( find )
      {
         _clearGroupName( id ) ;
      }
      PD_TRACE_EXIT ( SDB__CLSNDMGRAGENT_CLGP ) ;
      return SDB_OK ;
   }

   INT32 _clsNodeMgrAgent::lock_r ( INT32 millisec )
   {
      return _rwMutex.lock_r ( millisec ) ;
   }

   INT32 _clsNodeMgrAgent::lock_w ( INT32 millisec )
   {
      return _rwMutex.lock_w ( millisec ) ;
   }

   INT32 _clsNodeMgrAgent::release_r ()
   {
      return _rwMutex.release_r () ;
   }

   INT32 _clsNodeMgrAgent::release_w ()
   {
      return _rwMutex.release_w () ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSNDMGRAGENT_UPGPINFO, "_clsNodeMgrAgent::updateGroupInfo" )
   INT32 _clsNodeMgrAgent::updateGroupInfo ( const CHAR * objdata,
                                             UINT32 length, UINT32 *pGroupID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSNDMGRAGENT_UPGPINFO ) ;
      clsGroupItem* item = NULL ;
      UINT32 groupVersion = 0 ;
      UINT32 groupID = 0 ;
      string groupName ;
      map<UINT64, _netRouteNode> group ;
      UINT32 primary = 0 ;

      if ( !objdata || 0 == length )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = msgParseCatGroupObj( objdata, groupVersion, groupID,
                                groupName, group, &primary ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      item = groupItem ( groupID ) ;
      if ( !item )
      {
         item = _addGroupItem ( groupID ) ;
      }

      if ( !item )
      {
         PD_LOG ( PDERROR, "Create group item failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      // clear group name map
      _clearGroupName( groupID ) ;

      item->setGroupInfo ( groupName, groupVersion, primary ) ;
      rc = item->updateNodes ( group ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _addGroupName ( groupName, groupID ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( pGroupID )
      {
         *pGroupID = groupID ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSNDMGRAGENT_UPGPINFO, rc ) ;
      return rc ;
   error:
      if ( item )
      {
         clearGroup ( groupID ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSNDMGRAGENT__ADDGPIM, "_clsNodeMgrAgent::_addGroupItem" )
   clsGroupItem* _clsNodeMgrAgent::_addGroupItem ( UINT32 id )
   {
      PD_TRACE_ENTRY ( SDB__CLSNDMGRAGENT__ADDGPIM ) ;
      clsGroupItem* item = SDB_OSS_NEW clsGroupItem ( id ) ;
      if ( item )
      {
         try
         {
            _groupMap[id] = item ;
         }
         catch( std::exception & )
         {
            SDB_OSS_DEL item ;
            item = NULL ;
         }
      }
      PD_TRACE_EXIT ( SDB__CLSNDMGRAGENT__ADDGPIM ) ;
      return item ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSNDMGRAGENT__ADDGPNAME, "_clsNodeMgrAgent::_addGroupName" )
   INT32 _clsNodeMgrAgent::_addGroupName ( const std::string& name, UINT32 id )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSNDMGRAGENT__ADDGPNAME ) ;
      GROUP_NAME_MAP_IT it = _groupNameMap.find ( name ) ;
      if ( it != _groupNameMap.end() )
      {
         // the same
         if ( it->second == id )
         {
            rc = SDB_OK ;
            goto done ;
         }
      }

      try
      {
         _groupNameMap[name] = id ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__CLSNDMGRAGENT__ADDGPNAME, rc ) ;
      return rc ;
   }

   INT32 _clsNodeMgrAgent::_clearGroupName( UINT32 id )
   {
      GROUP_NAME_MAP_IT it = _groupNameMap.begin() ;
      while ( it != _groupNameMap.end() )
      {
         if ( it->second == id )
         {
            _groupNameMap.erase( it ) ;
            break ;
         }
         ++it ;
      }
      return SDB_OK ;
   }

   /// cls catalog agent tool functions :

   INT32 clsPartition( const BSONObj & keyObj,
                       UINT32 partitionBit,
                       UINT32 internalVersion )
   {
      if ( CAT_INTERNAL_VERSION_3 <= internalVersion )
      {
         return BSON_HASHER::hashObj( keyObj, partitionBit ) ;
      }
      else if ( CAT_INTERNAL_VERSION_2 == internalVersion )
      {
         return BSON_HASHER_OBSOLETE::hash( keyObj, partitionBit ) ;
      }
      /// if it is a old version collection, use old hash algorithm.
      else
      {
         md5::md5digest digest ;
         md5::md5( keyObj.objdata(), keyObj.objsize(), digest ) ;
         UINT32 hashValue = 0 ;
         UINT32 i = 0 ;
         while ( i++ < 4 )
         {
            hashValue |= ( (UINT32)digest[i] << ( 32 - 8 * i ) ) ;
         }
         return (INT32)( hashValue >> ( 32 - partitionBit ) ) ;
      }
   }

   INT32 clsPartition( const bson::OID &oid, UINT32 sequence, UINT32 partitionBit )
   {
      md5_state_t st ;
      md5::md5digest digest ;
      md5_init(&st);
      md5_append(&st, (const md5_byte_t *)(oid.getData()), sizeof( oid ) );
      md5_append(&st, (const md5_byte_t *)( &sequence ), sizeof( sequence ) ) ;
      md5_finish( &st, digest ) ;
      UINT32 hashValue = 0 ;
      UINT32 i = 0 ;
      while ( i++ < 4 )
      {
         hashValue |= ( (UINT32)digest[i] << ( 32 - 8 * i ) ) ;
      }
      return (INT32)( hashValue >> ( 32 - partitionBit ) ) ;
   }

   clsShardingKeySite* clsGetShardingKeySite()
   {
      static clsShardingKeySite s_skSite ;
      return &s_skSite ;
   }

   INT32 clsGetLobBound( INT32 lobKeyFormat, const BSONObj &originalBound,
                         BSONObj &lobBound )
   {
      INT32 rc = SDB_OK ;
      time_t seconds ;
      _utilLobID lobID ;
      bson::OID oid ;
      BYTE idArray[ UTIL_LOBID_ARRAY_LEN + 1] = {0} ;
      BSONElement ele = originalBound.firstElement() ;
      if ( ele.type() != String )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Bound must be String format:bound=%s,rc=%d",
                 originalBound.toString().c_str(), rc ) ;
         goto error ;
      }

      rc = clsGetLobSecondsByTimeStr( ele.valuestr(), lobKeyFormat, seconds ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get lob second:bound=%s,format=%d,rc=%d",
                 originalBound.toString().c_str(), lobKeyFormat, rc ) ;
         goto error ;
      }

      lobID.initOnlySeconds( seconds ) ;
      rc = lobID.toByteArray( idArray, UTIL_LOBID_ARRAY_LEN ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get byteArray from LobID[%s]:rc=%d",
                 lobID.toString().c_str(), rc ) ;
         goto error ;
      }

      oid.init( idArray, UTIL_LOBID_ARRAY_LEN ) ;
      lobBound = BSON( FIELD_NAME_LOB_OID << oid ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN clsCheckAndParseLobKeyFormat( const CHAR *keyFormat, INT32 *result )
   {
      BOOLEAN isValid = FALSE ;
      INT32 tmpResult = SDB_TIME_INVALID ;

      if ( NULL == keyFormat )
      {
         //do nothing here
      }
      else if ( ossStrcasecmp( keyFormat, "YYYYMMDD" ) == 0 )
      {
         tmpResult = SDB_TIME_DAY ;
         isValid = TRUE ;
      }
      else if ( ossStrcasecmp( keyFormat, "YYYYMM" ) == 0 )
      {
         tmpResult = SDB_TIME_MONTH ;
         isValid = TRUE ;
      }
      else if ( ossStrcasecmp( keyFormat, "YYYY" ) == 0 )
      {
         tmpResult = SDB_TIME_YEAR ;
         isValid = TRUE ;
      }

      if ( isValid && NULL != result )
      {
         *result = tmpResult ;
      }

      return isValid ;
   }

   INT32 clsGetLobSecondsByTimeStr( const CHAR *timeStr,
                                    INT32 lobKeyFormat,
                                    time_t &seconds )
   {
      INT32 rc = SDB_OK ;
      INT32 parseNum = 0 ;
      struct tm tmTime = { 0 } ;
      struct tm tmpTM ;
      time_t localSeconds ;
      if ( SDB_TIME_DAY == lobKeyFormat )
      {
         PD_CHECK( ossStrlen( timeStr ) == 8, SDB_INVALIDARG, error, PDERROR,
                   "TimeStr[%s]'s length must be 8", timeStr ) ;

         CHAR format[] = "%04d%02d%02d" ;
         parseNum = ossSscanf( timeStr, format, &tmTime.tm_year, &tmTime.tm_mon,
                               &tmTime.tm_mday ) ;
         PD_CHECK( 3 == parseNum, SDB_INVALIDARG, error, PDERROR, "ParseNum[%d]"
                   " must be 3", parseNum ) ;
         tmTime.tm_mon -= 1 ;
      }
      else if ( SDB_TIME_MONTH == lobKeyFormat )
      {
         PD_CHECK( ossStrlen( timeStr ) == 6, SDB_INVALIDARG, error, PDERROR,
                   "TimeStr[%s]'s length must be 6", timeStr ) ;

         CHAR format[] = "%04d%02d" ;
         parseNum = ossSscanf( timeStr, format, &tmTime.tm_year, &tmTime.tm_mon ) ;
         PD_CHECK( 2 == parseNum, SDB_INVALIDARG, error, PDERROR, "ParseNum[%d]"
                   " must be 2", parseNum ) ;
         tmTime.tm_mon -= 1 ;
         tmTime.tm_mday = 1 ;
      }
      else if ( SDB_TIME_YEAR == lobKeyFormat )
      {
         PD_CHECK( ossStrlen( timeStr ) == 4, SDB_INVALIDARG, error, PDERROR,
                   "TimeStr[%s]'s length must be 4", timeStr ) ;

         CHAR format[] = "%04d" ;
         parseNum = ossSscanf( timeStr, format, &tmTime.tm_year ) ;
         PD_CHECK( 1 == parseNum, SDB_INVALIDARG, error, PDERROR, "ParseNum[%d]"
                   " must be 1", parseNum ) ;
         tmTime.tm_mon = 0 ;
         tmTime.tm_mday = 1 ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid lobKeyFormat[%d]", lobKeyFormat ) ;
         goto error ;
      }

      PD_CHECK( (tmTime.tm_mday <=31 && tmTime.tm_mday >= 1), SDB_INVALIDARG,
                error, PDERROR, "Invalid time format[%s]", timeStr ) ;
      PD_CHECK( (tmTime.tm_mon <=11 && tmTime.tm_mon >= 0), SDB_INVALIDARG,
                error, PDERROR, "Invalid time format[%s]", timeStr ) ;

      tmTime.tm_year -= 1900 ;

      clsNormalizeTM( tmTime ) ;
      tmpTM = tmTime ;

      localSeconds = mktime( &tmTime ) ;
      if ( tmpTM.tm_mday != tmTime.tm_mday || tmpTM.tm_mon != tmTime.tm_mon )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid time format[%s]", timeStr ) ;
         goto error ;
      }

      ossTimeLocalToUTCInSameDate( localSeconds, seconds ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   // adjust tm to a valid time.
   // 20190231 => 20190228
   // 20200231 => 20200229
   void clsNormalizeTM( struct tm &tmTime )
   {
      if ( tmTime.tm_mday > 28 )
      {
         INT32 maxDay = ossTimeGetMaxDay( tmTime.tm_year + 1900,
                                          tmTime.tm_mon +1 ) ;
         if ( tmTime.tm_mday > maxDay )
         {
            tmTime.tm_mday = maxDay ;
         }
      }
   }

   INT32 clsGetLobTimeStr( INT64 seconds, INT32 lobShardingKeyFormat,
                           CHAR *timeStr, INT32 timeStrLen )
   {
      INT32 rc = SDB_OK ;
      struct tm tmpTm ;
      time_t t ;

      PD_CHECK( NULL != timeStr, SDB_INVALIDARG, error, PDERROR,
                "TimeStr is NULL") ;

      t = ( time_t )seconds ;
      ossGmtime( t, tmpTm ) ;
      if ( SDB_TIME_DAY == lobShardingKeyFormat )
      {
         PD_CHECK( timeStrLen >= 9, SDB_INVALIDARG, error, PDERROR,
                   "TimeStrLen[%d] is too small", timeStrLen ) ;

         ossSnprintf( timeStr, timeStrLen, "%04d%02d%02d",
                      tmpTm.tm_year + 1900, tmpTm.tm_mon + 1, tmpTm.tm_mday ) ;
      }
      else if ( SDB_TIME_MONTH == lobShardingKeyFormat )
      {
         PD_CHECK( timeStrLen >= 7, SDB_INVALIDARG, error, PDERROR,
                   "TimeStrLen[%d] is too small", timeStrLen ) ;

         ossSnprintf( timeStr, timeStrLen, "%04d%02d",
                      tmpTm.tm_year + 1900, tmpTm.tm_mon + 1 ) ;
      }
      else if ( SDB_TIME_YEAR == lobShardingKeyFormat )
      {
         PD_CHECK( timeStrLen >= 5, SDB_INVALIDARG, error, PDERROR,
                   "TimeStrLen[%d] is too small", timeStrLen ) ;

         ossSnprintf( timeStr, timeStrLen, "%04d", tmpTm.tm_year + 1900 ) ;
      }
      else
      {
         PD_CHECK( timeStrLen >= 15, SDB_INVALIDARG, error, PDERROR,
                   "TimeStrLen[%d] is too small", timeStrLen ) ;

         ossSnprintf( timeStr, timeStrLen, "%04d%02d%02d%02d%02d%02d",
                      tmpTm.tm_year + 1900, tmpTm.tm_mon + 1, tmpTm.tm_mday,
                      tmpTm.tm_hour, tmpTm.tm_min, tmpTm.tm_sec ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
