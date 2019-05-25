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
#include "clsCataHashMatcher.hpp"
#include "utilBsonHash.hpp"
#include "utilCommon.hpp"
#include <set>

#include "../bson/lib/md5.hpp"
#include "../bson/lib/md5.h"


using namespace bson ;

namespace engine
{
#define CLS_CA_SHARDINGTYPE_NONE 0
#define CLS_CA_SHARDINGTYPE_RANGE 1
#define CLS_CA_SHARDINGTYPE_HASH 2


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

   BSONObj& _clsCatalogItem::getLowBound ()
   {
      return _lowBound ;
   }

   BSONObj& _clsCatalogItem::getUpBound ()
   {
      return _upBound ;
   }

   clsCataItemKey _clsCatalogItem::getLowBoundKey ( const Ordering* ordering )
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

   clsCataItemKey _clsCatalogItem::getUpBoundKey ( const Ordering * ordering )
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
            BSONElement eleGroupID = obj.getField ( CAT_CATALOGGROUPID_NAME ) ;
            if ( !eleGroupID.isNumber() )
            {
               PD_LOG ( PDERROR, "Parse catalog item failed, field[%s] error",
                        CAT_CATALOGGROUPID_NAME ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            _groupID = eleGroupID.numberInt () ;

            if ( _saveName )
            {
               BSONElement eleGroupName = obj.getField( CAT_GROUPNAME_NAME ) ;
               PD_CHECK( String == eleGroupName.type(), SDB_SYS, error, PDERROR,
                         "Parse catalog item failed, field[%s] type error",
                         CAT_GROUPNAME_NAME ) ;
               _groupName = eleGroupName.String() ;
            }
         }

         if ( isSharding )
         {
            BSONElement eleLow = obj.getField( CAT_LOWBOUND_NAME ) ;
            if ( Object != eleLow.type() )
            {
               PD_LOG ( PDERROR, "Parse catalog item failed, field[%s] error",
                        CAT_LOWBOUND_NAME ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            _lowBound = eleLow.embeddedObject().copy () ;

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
      return BSONObj () ;
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

   Ordering* _clsCataOrder::getOrdering ()
   {
      return &_ordering ;
   }

   /*
   note: _clsCatalogSet implement
   */
   _clsCatalogSet::_clsCatalogSet ( const CHAR * name, BOOLEAN saveName )
   {
      _saveName = saveName ;
      _name = name ;
      _version = -1 ;
      _w = 1 ;
      _next = NULL ;
      _lastItem = NULL ;
      _pOrder = NULL ;
      _pKeyGen = NULL ;
      _isWholeRange = TRUE ;
      _groupCount = 0 ;
      _shardingType = CLS_CA_SHARDINGTYPE_NONE ;
      _ensureShardingIndex = true ;
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

   VEC_GROUP_ID *_clsCatalogSet::getAllGroupID ()
   {
      return &_vecGroupID ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_GETALLGPID, "_clsCatalogSet::getAllGroupID" )
   UINT32 _clsCatalogSet::getAllGroupID ( VEC_GROUP_ID &vecGroup ) const
   {
      PD_TRACE_ENTRY ( SDB__CLSCTSET_GETALLGPID ) ;
      vecGroup.clear() ;
      UINT32 size = (UINT32)_vecGroupID.size() ;
      for ( UINT32 index = 0 ; index < size ; index++ )
      {
         vecGroup.push_back ( _vecGroupID[index] ) ;
      }
      PD_TRACE_EXIT ( SDB__CLSCTSET_GETALLGPID ) ;
      return size ;
   }

   UINT32 _clsCatalogSet::groupCount() const
   {
      return _groupCount ;
   }

   Ordering *_clsCatalogSet::getOrdering ()
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
   void _clsCatalogSet::_addGroupID ( UINT32 groupID )
   {
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
      _vecGroupID.push_back ( groupID ) ;
      _groupCount = _vecGroupID.size() ;

   done:
      PD_TRACE_EXIT ( SDB__CLSCTSET_ADDGPID ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_ADDSUBCLNAME, "_clsCatalogSet::_addSubClName" )
   void _clsCatalogSet::_addSubClName( UINT32 id,
                                       const std::string &strClName )
   {
      PD_TRACE_ENTRY ( SDB__CLSCTSET_ADDSUBCLNAME ) ;
      _subCLList.insert( std::make_pair( id, strClName ) ) ;
      PD_TRACE_EXIT ( SDB__CLSCTSET_ADDSUBCLNAME ) ;
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

      _shardingType = CLS_CA_SHARDINGTYPE_NONE;
      _shardingKey = BSONObj() ;
      _maxSize = 0 ;
      _maxRecNum = 0 ;
      _overwrite = FALSE ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_FINDIM, "_clsCatalogSet::findItem" )
   INT32 _clsCatalogSet::findItem ( const BSONObj &obj,
                                    clsCatalogItem *& item )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCTSET_FINDIM ) ;
      item = NULL ;
      if ( !isSharding() || ( _isWholeRange && 1 == _groupCount ) )
      {
         PD_CHECK ( 1 == _mapItems.size(), SDB_SYS, error, PDERROR,
                    "When not sharding, the collection[%s] cataItem "
                    "number[%d] error", name(), _mapItems.size() ) ;
         item = _mapItems.begin()->second ;
         goto done ;
      }
      {
         BSONObjSet objSet ;
         PD_CHECK( _pKeyGen, SDB_SYS, error, PDSEVERE, "KeyGen is null" ) ;
         rc = _pKeyGen->getKeys( obj , objSet ) ;
         PD_RC_CHECK ( rc, PDERROR, "Generate key failed, rc = %d", rc ) ;
         PD_CHECK ( 1 == objSet.size(), SDB_MULTI_SHARDING_KEY, error, PDINFO,
                    "More than one sharding key is detected" ) ;

         {
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET__FINDIM, "_clsCatalogSet::_findItem" )
   INT32 _clsCatalogSet::_findItem( const clsCataItemKey &findKey,
                                    clsCatalogItem *& item )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCTSET__FINDIM ) ;
      item = NULL ;
      if ( !isSharding() || ( _isWholeRange && _groupCount == 1 ) )
      {
         PD_CHECK ( 1 == _mapItems.size(), SDB_SYS, error, PDERROR,
                    "When not sharding, the collection[%s] cataItem "
                    "number[%d] error", name(), _mapItems.size() ) ;
         item = _mapItems.begin()->second ;
         goto done ;
      }
      {
         MAP_CAT_ITEM_IT it = _mapItems.upper_bound ( findKey ) ;
         if ( it == _mapItems.end () )
         {
            item = _lastItem ;
            rc = item ? SDB_OK : SDB_CAT_NO_MATCH_CATALOG ;
            goto done ;
         }

         if ( !_isWholeRange )
         {
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_FINDGPID, "_clsCatalogSet::findGroupID" )
   INT32 _clsCatalogSet::findGroupID ( const BSONObj & obj, UINT32 &groupID )
   {
      INT32 rc              = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCTSET_FINDGPID ) ;
      _clsCatalogItem *item = NULL ;
      rc = findItem ( obj, item ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_FINDGPID2, "_clsCatalogSet::findGroupID2" )
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
      BOOLEAN result = FALSE ;
      MAP_CAT_ITEM::iterator iter ;

      PD_CHECK ( !_mapItems.empty(), SDB_SYS, error, PDERROR,
                 "The collection[%s]'s cataItem is empty", name() ) ;

      if ( !isSharding() || 1 == _groupCount )
      {
         iter = _mapItems.begin();
         while( iter != _mapItems.end() )
         {
            vecGroup.push_back( iter->second->getGroupID() );
            ++iter;
         }
         goto done ;
      }

      if ( isHashSharding() )
      {
         if ( CAT_INTERNAL_VERSION_2 <= getInternalV() )
         {
            clsCataHashMatcher hashMatcher( _shardingKey ) ;
            rc = hashMatcher.loadPattern( matcher, _square, getInternalV() ) ;
            PD_RC_CHECK( rc, PDERROR, "Load matcher failed, rc: %d", rc ) ;

            iter = _mapItems.begin();
            while( iter != _mapItems.end() )
            {
               rc = hashMatcher.matches( iter->second, result );
               PD_RC_CHECK( rc, PDERROR, "Match catalog item failed, rc: %d",
                            rc ) ;
               if ( result )
               {
                  vecGroup.push_back( iter->second->getGroupID() );
               }
               ++iter ;
            }
         }
         else
         {
            iter = _mapItems.begin();
            while( iter != _mapItems.end() )
            {
               vecGroup.push_back( iter->second->getGroupID() );
               ++iter ;
            }
         }
         goto done ;
      }
      else
      {
         clsCatalogMatcher clsMatcher( _shardingKey ) ;
         rc = clsMatcher.loadPattern( matcher );
         PD_RC_CHECK( rc, PDERROR, "Load matcher failed, rc: %d", rc ) ;

         iter = _mapItems.begin();
         while( iter != _mapItems.end() )
         {
            rc = clsMatcher.matches( iter->second, result );
            PD_RC_CHECK( rc, PDERROR, "Match catalog item failed, rc: %d",
                         rc ) ;
            if ( result )
            {
               vecGroup.push_back( iter->second->getGroupID() );
            }
            ++iter ;
         }
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_ISKEYONBD, "_clsCatalogSet::isKeyOnBoundary" )
   BOOLEAN _clsCatalogSet::isKeyOnBoundary ( const BSONObj &obj,
                                             UINT32* pGroupID )
   {
      PD_TRACE_ENTRY ( SDB__CLSCTSET_ISKEYONBD ) ;
      BOOLEAN result        = FALSE ;

      if ( !isSharding() )
      {
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
         MAP_CAT_ITEM_IT it ;
         if ( isHashSharding() )
         {
            clsCataItemKey findKey ( obj.firstElement().numberInt() ) ;
            it = _mapItems.lower_bound ( findKey ) ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_SPLIT, "_clsCatalogSet::split" )
   INT32 _clsCatalogSet::split ( const BSONObj &splitKey,
                                 const BSONObj &splitEndKey,
                                 UINT32 groupID, const CHAR *groupName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__CLSCTSET_SPLIT ) ;

      SDB_ASSERT ( _isWholeRange, "split must be done on whole range" ) ;

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
         if ( isHashSharding() )
         {
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
         MAP_CAT_ITEM_IT it = _mapItems.upper_bound ( *findKey ) ;

         if ( it == _mapItems.end () )
         {
            item = _lastItem ;
            --it ;
         }
         else
         {
            item = it->second ;
         }
         SDB_ASSERT ( item, "last item should never be NULL in split" ) ;
         if ( item->getGroupID() == groupID )
         {
            PD_LOG ( PDERROR, "split got duplicate source and dest on group %d",
                     groupID ) ;
            goto done ;
         }

         {
            MAP_CAT_ITEM_IT tmpit = it ;
            BOOLEAN itemRemoved = FALSE, itemEndRemoved = FALSE ;

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

            _deduplicate () ;

            _remakeGroupIDs() ;
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

   void _clsCatalogSet::_remakeGroupIDs()
   {
      _vecGroupID.clear() ;
      clsCatalogItem *item = NULL ;

      MAP_CAT_ITEM_IT it = _mapItems.begin() ;
      while ( it != _mapItems.end() )
      {
         item = it->second ;
         _addGroupID( item->getGroupID() ) ;
         ++it ;
      }
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
      if ( !(_mapItems.insert(std::make_pair(
                             item->getUpBoundKey( getOrdering() ),
                             item))).second )
      {
         rc = SDB_CAT_CORRUPTION ;
         PD_LOG ( PDERROR, "CataItem already exist: %s",
                  item->toBson().toString().c_str() ) ;
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
         if ( it->second->getGroupID() ==
              itPrev->second->getGroupID() )
         {
            clsCatalogItem *itemPrev = ( itPrev->second ) ;

            it->second->_lowBound = itemPrev->_lowBound ;

            _mapItems.erase ( itPrev ) ;
            SAFE_OSS_DELETE( itemPrev ) ;
         }
         itPrev = it ;
      }
      PD_TRACE_EXIT ( SDB__CLSCTSET__DEDUP ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTSET_TOCTINFOBSON, "_clsCatalogSet::toCataInfoBson" )
   BSONObj _clsCatalogSet::toCataInfoBson ()
   {
      PD_TRACE_ENTRY ( SDB__CLSCTSET_TOCTINFOBSON ) ;
      BSONObj obj ;
      if ( !isSharding() )
      {
         SDB_ASSERT ( _mapItems.size() == 1,
                      "map item size must be 1 for non-sharded collection" ) ;
         try
         {
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
         try
         {
            BSONObjBuilder bb ;
            BSONArrayBuilder ab ;
            MAP_CAT_ITEM_IT it     = _mapItems.begin() ;
            for ( ; it != _mapItems.end(); ++it )
            {
               ab.append ( it->second->toBson () ) ;
            }
            bb.append ( CAT_CATALOGINFO_NAME, ab.arr () ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCT_UDCATSET, "_clsCatalogSet::updateCatSet" )
   INT32 _clsCatalogSet::updateCatSet( const BSONObj & catSet, UINT32 groupID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCT_UDCATSET ) ;

      PD_LOG ( PDDEBUG, "Update cataSet: %s", catSet.toString().c_str() ) ;
      _clear() ;

      BSONElement ele = catSet.getField ( CAT_CATALOGVERSION_NAME ) ;
      PD_CHECK ( !ele.eoo() && ele.type() == NumberInt, SDB_CAT_CORRUPTION,
                 error, PDSEVERE,
                 "Catalog [%s] type error", CAT_CATALOGVERSION_NAME ) ;
      _version = (INT32)ele.Int() ;

      ele = catSet.getField( CAT_CATALOG_W_NAME ) ;
      if ( ele.eoo () )
      {
         _w = 1 ;
      }
      else if ( ele.type() != NumberInt )
      {
         PD_RC_CHECK ( SDB_CAT_CORRUPTION, PDSEVERE,
                       "Catalog [%s] type error", CAT_CATALOG_W_NAME ) ;
      }
      else
      {
         _w = (INT32)ele.Int() ;
      }

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

      ele = catSet.getField( CAT_MAINCL_NAME );
      if ( ele.type() == String )
      {
         _mainCLName = ele.str();
      }
      else
      {
         _mainCLName.clear() ;
      }

      ele = catSet.getField (CAT_ATTRIBUTE_NAME ) ;
      if ( NumberInt == ele.type() )
      {
         _attribute = ele.numberInt() ;
      }

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
         PD_LOG( PDERROR, "Catalog [%s] type error: %d" ,
                 CAT_COMPRESSIONTYPE, ele.type() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

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

      ele = catSet.getField( CAT_SHARDINGKEY_NAME ) ;
      if ( ele.eoo() )
      {
         _shardingType = CLS_CA_SHARDINGTYPE_NONE ;
      }
      else if ( ele.type() != Object )
      {
         PD_RC_CHECK ( SDB_CAT_CORRUPTION, PDSEVERE,
                       "Catalog [%s] type error",
                       CAT_SHARDINGKEY_NAME ) ;
      }
      else
      {
         _shardingKey = ele.embeddedObject().copy () ;

         ele = catSet.getField( CAT_SHARDING_TYPE ) ;

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

         ele = catSet.getField( CAT_ENSURE_SHDINDEX ) ;
         if ( ele.eoo() )
         {
            _ensureShardingIndex = true ;
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

         if ( _pSite && !_mainCLName.empty() )
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

         ele = catSet.getField( CAT_INTERNAL_VERSION ) ;
         if ( NumberInt == ele.type() )
         {
            _internalV = ele.Int() ;
         }
      }

      if ( isRangeSharding() )
      {
         _pOrder = SDB_OSS_NEW clsCataOrder ( Ordering::make ( _shardingKey ) ) ;
         PD_CHECK ( _pOrder, SDB_OOM, error, PDERROR,
                    "Failed to alloc memory for CataOrder" ) ;
      }

      _pKeyGen = SDB_OSS_NEW ixmIndexKeyGen ( _shardingKey ) ;
      PD_CHECK ( _pKeyGen, SDB_OOM, error, PDERROR,
                 "Failed to alloc memory for KeyGen" ) ;

      ele = catSet.getField( CAT_CATALOGINFO_NAME ) ;
      if ( ele.eoo () )
      {
         goto done ;
      }
      PD_CHECK ( ele.type() == Array, SDB_CAT_CORRUPTION, error,
                 PDSEVERE, "Catalog [%s] type error",
                 CAT_CATALOGINFO_NAME ) ;
      {
         clsCatalogItem *cataItem = NULL ;
         BSONObj objCataInfo = ele.embeddedObject() ;
         PD_CHECK ( isSharding() || objCataInfo.nFields() == 1,
                    SDB_CAT_CORRUPTION, error, PDSEVERE,
                    "The catalog info must be 1 item when not sharding" ) ;

         BSONObjIterator objItr ( objCataInfo ) ;
         while ( objItr.more () )
         {
            BSONElement eleCataItem = objItr.next () ;
            PD_CHECK ( !eleCataItem.eoo() && eleCataItem.type () == Object,
                       SDB_CAT_CORRUPTION, error, PDSEVERE,
                       "CataItem type error" ) ;

            cataItem = SDB_OSS_NEW _clsCatalogItem( _saveName, _isMainCL ) ;
            PD_CHECK ( cataItem, SDB_OOM, error, PDERROR,
                       "Failed to alloc memory for cataItem" ) ;

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

            if ( groupID != 0 && groupID != cataItem->getGroupID()
               && !_isMainCL )
            {
               SDB_OSS_DEL cataItem ;
               _isWholeRange = FALSE ;
               continue ;
            }

            rc = _addItem( cataItem ) ;
            if ( rc )
            {
               SDB_OSS_DEL cataItem ;
               goto error ;
            }

            if ( !_isMainCL )
            {
               _addGroupID ( cataItem->getGroupID() ) ;
            }
            else
            {
               _addSubClName( cataItem->getID(), cataItem->getSubClName() );
            }
         }
      }

      {
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
   BOOLEAN _clsCatalogSet::_isObjAllMaxKey ( BSONObj &obj )
   {
      PD_TRACE_ENTRY ( SDB__CLSCTSET__ISOBJALLMK ) ;
      BOOLEAN ret = TRUE ;
      if ( isRangeSharding() )
      {
         INT32 index = 0 ;
         INT32 dir = 1 ;
         Ordering *pOrder = getOrdering () ;
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
   INT32 _clsCatalogSet::getSubCLList( vector< string > &subCLLst,
                                       CLS_SUBCL_SORT_TYPE sortType )
   {
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

      rc = genKeyObj( lowBound, lowBoundObj );
      PD_RC_CHECK( rc, PDERROR, "failed to get low-bound(rc=%d)", rc );
      rc = genKeyObj( upBound, upBoundObj );
      PD_RC_CHECK( rc, PDERROR, "failed to get up-bound(rc=%d)", rc );

      pNewItem = SDB_OSS_NEW clsCatalogItem( FALSE, TRUE );
      PD_CHECK( pNewItem, SDB_OOM, error, PDERROR,
               "malloc failed!" );
      pNewItem->_lowBound = lowBoundObj;
      pNewItem->_upBound = upBoundObj;
      pNewItem->_subCLName = subCLName;
      pNewItem->_ID = ++_maxID ;

      {
      clsCataItemKey newItemLowKey = pNewItem->getLowBoundKey( _pOrder->getOrdering() );
      clsCataItemKey newItemUpKey = pNewItem->getUpBoundKey( _pOrder->getOrdering() );
      PD_CHECK( newItemLowKey < newItemUpKey, SDB_BOUND_INVALID, error, PDERROR,
               "invalid boundary(low:%s, up:%s)",
               lowBoundObj.toString().c_str(), lowBoundObj.toString().c_str() );

      if ( _mapItems.empty() )
      {
         rc = _addItem( pNewItem );
         PD_RC_CHECK( rc, PDERROR,
                     "failed to add the new item(rc=%d)",
                     rc );
         goto done;
      }

      iter = _mapItems.upper_bound( newItemUpKey );

      if ( iter != _mapItems.end() )
      {
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

   INT32 _clsCatalogSet::findSubCLNames( const BSONObj &matcher,
                                         vector< string > &subCLList,
                                         CLS_SUBCL_SORT_TYPE sortType )
   {
      INT32 rc = SDB_OK;
      BOOLEAN result = FALSE;
      std::multimap<UINT32, clsCatalogItem* > mapSubItem ;
      MAP_CAT_ITEM::iterator iter;

      if ( !isMainCL() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "The collection[%s] is not collection-partitioned",
                 name() ) ;
         goto error ;
      }
      else if ( _mapItems.empty() )
      {
         PD_LOG( PDERROR, "The collection[%s]'s cataItem is empty", name() ) ;
         rc = SDB_CAT_NO_MATCH_CATALOG ;
         goto error ;
      }

      subCLList.clear() ;

      if ( isHashSharding() )
      {
         iter = _mapItems.begin();
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
         goto done ;
      }
      else
      {
         clsCatalogMatcher clsMatcher( _shardingKey ) ;
         rc = clsMatcher.loadPattern( matcher );
         PD_RC_CHECK( rc, PDERROR, "Load matcher failed, rc: %d", rc ) ;

         iter = _mapItems.begin() ;
         while( iter != _mapItems.end() )
         {
            rc = clsMatcher.matches( iter->second, result );
            PD_RC_CHECK( rc, PDERROR, "Match catalog item failed, rc: %d",
                         rc ) ;
            if ( result )
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
            }
            ++iter;
         }

         if ( SUBCL_SORT_BY_ID == sortType )
         {
            PD_CHECK( mapSubItem.size() != 0, SDB_CAT_NO_MATCH_CATALOG,
                      error, PDERROR, "couldn't find the match catalog" ) ;
         }
         else
         {
            PD_CHECK( subCLList.size() != 0, SDB_CAT_NO_MATCH_CATALOG,
                      error, PDERROR, "couldn't find the match catalog" ) ;
         }
      }

   done:
      if ( SDB_OK == rc && SUBCL_SORT_BY_ID == sortType )
      {
         std::multimap<UINT32, clsCatalogItem* >::reverse_iterator rit =
               mapSubItem.rbegin() ;
         while( rit != mapSubItem.rend() )
         {
            subCLList.push_back( rit->second->getSubClName() ) ;
            ++rit ;
         }
      }
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
   void _clsCatalogAgent::getAllNames( std::vector<string> &names )
   {
      PD_TRACE_ENTRY ( SDB__CLSCTAGENT_GETALLNAMES ) ;
      CAT_MAP_IT itr = _mapCatalog.begin() ;
      for ( ; itr != _mapCatalog.end(); itr++ )
      {
         _clsCatalogSet *set = itr->second ;
         do
         {
            names.push_back( string( set->name() ) ) ;
            set = set->next() ;
         } while ( NULL != set ) ;
      }
      PD_TRACE_EXIT ( SDB__CLSCTAGENT_GETALLNAMES ) ;
      return ;
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
   _clsCatalogSet *_clsCatalogAgent::collectionSet ( const CHAR * name )
   {
      PD_TRACE_ENTRY ( SDB__CLSCTAGENT_CLSET ) ;
      _clsCatalogSet *set = NULL ;
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
   done:
      PD_TRACE_EXIT ( SDB__CLSCTAGENT_CLSET ) ;
      return set ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTAGENT__ADDCLSET, "_clsCatalogAgent::_addCollectionSet" )
   _clsCatalogSet *_clsCatalogAgent::_addCollectionSet ( const CHAR * name )
   {
      PD_TRACE_ENTRY ( SDB__CLSCTAGENT__ADDCLSET ) ;
      _clsCatalogSet *catSet = SDB_OSS_NEW _clsCatalogSet ( name, FALSE ) ;
      if ( !catSet )
      {
         return NULL ;
      }

      UINT32 hashCode = ossHash ( name ) ;

      CAT_MAP_IT it = _mapCatalog.find ( hashCode ) ;
      if ( it == _mapCatalog.end() )
      {
         _mapCatalog[hashCode] = catSet ;
      }
      else
      {
         _clsCatalogSet * rootSet = it->second ;
         while ( rootSet->next() )
         {
            rootSet = rootSet->next () ;
         }
         rootSet->next ( catSet ) ;
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
         BSONElement ele = catalog.getField ( CAT_COLLECTION_NAME ) ;
         if ( ele.type() != String )
         {
            rc = SDB_SYS ;
            PD_LOG ( PDERROR, "collection name type error" ) ;
            goto error ;
         }

         catSet = collectionSet ( ele.str().c_str() ) ;
         if ( !catSet )
         {
            catSet = _addCollectionSet ( ele.str().c_str() ) ;
         }

         if ( !catSet )
         {
            rc = SDB_OOM ;
            PD_LOG ( PDERROR, "create collection set failed" ) ;
            goto error ;
         }

         rc = catSet->updateCatSet ( catalog, groupID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Update catalogSet[%s] failed[rc:%d]",
                     ele.str().c_str(), rc ) ;
            clear( ele.str().c_str() ) ;
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
   INT32 _clsCatalogAgent::clear ( const CHAR* name,
                                   CHAR * mainCL )
   {
      PD_TRACE_ENTRY ( SDB__CLSCTAGENT_CLEAR ) ;
      _clsCatalogSet *preSet = NULL ;
      _clsCatalogSet *curSet = NULL ;
      UINT32 hashCode = ossHash ( name ) ;

      CAT_MAP_IT it = _mapCatalog.find ( hashCode ) ;
      if ( it != _mapCatalog.end() )
      {
         curSet = it->second ;
         while ( curSet )
         {
            if ( ossStrcmp ( curSet->name(), name ) == 0 )
            {
               if ( !curSet->getMainCLName().empty() &&
                    mainCL != NULL )
               {
                  ossStrncpy( mainCL, curSet->getMainCLName().c_str(),
                              DMS_COLLECTION_FULL_NAME_SZ ) ;
                  mainCL[ DMS_COLLECTION_FULL_NAME_SZ ] = '\0' ;
               }
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

      PD_TRACE_EXIT ( SDB__CLSCTAGENT_CLEAR ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCTAGENT_CRBYSPACENAME, "_clsCatalogAgent::clearBySpaceName" )
   INT32 _clsCatalogAgent::clearBySpaceName ( const CHAR * name,
                                              vector< string > *pRelatedCLs,
                                              _utilSet< string > * pMainCLs )
   {
      PD_TRACE_ENTRY ( SDB__CLSCTAGENT_CRBYSPACENAME ) ;
      _clsCatalogSet *preSet = NULL ;
      _clsCatalogSet *curSet = NULL ;
      _clsCatalogSet *tmpSet = NULL ;
      UINT32 nameLen = ossStrlen(name) ;
      BOOLEAN itAdd  = TRUE ;
      _utilSet< string > mainCLList ;
      _utilSet< string >::iterator iterMain ;
      CAT_MAP_IT it = _mapCatalog.begin() ;

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
            if ( pRelatedCLs &&
                 0 == ossStrncmp( curSet->_mainCLName.c_str(),
                                  name, nameLen ) &&
                 '.' == curSet->_mainCLName.at( nameLen ) )
            {
               pRelatedCLs->push_back( curSet->_name ) ;
            }

            if ( ossStrncmp ( curSet->name(), name, nameLen ) == 0
               && (curSet->name())[nameLen] == '.' )
            {
               string strMainCL = curSet->getMainCLName() ;
               if ( !strMainCL.empty() )
               {
                  pMainCLs->insert( strMainCL ) ;
               }
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

      iterMain = pMainCLs->begin() ;
      while ( iterMain != pMainCLs->end() )
      {
         clear( (*iterMain).c_str() ) ;
         ++iterMain ;
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSCTAGENT_CRBYSPACENAME ) ;
      return SDB_OK ;
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

      _mutex.get() ;
      it = _mapKey2ID.find( shardingKey ) ;
      if ( it == _mapKey2ID.end() )
      {
         retID = _id++ ;
         _mapKey2ID[ shardingKey ] = ossPack32To64( retID, 1 ) ;
      }
      else
      {
         UINT32 lo = 0 ;
         ossUnpack32From64( it->second, retID, lo ) ;
         it->second = ossPack32To64( retID, lo + 1 ) ;
      }
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
      _mutex.get() ;
      it = _mapKey2ID.find( shardingKey ) ;
      if ( it != _mapKey2ID.end() )
      {
         UINT32 hi = 0 ;
         UINT32 lo = 0 ;
         ossUnpack32From64( it->second, hi, lo ) ;
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

      PD_CHECK( _groupID == groupID, SDB_SYS, error, PDERROR,
                "group item[groupid:%d] update group id[%d] invalid[%s]",
                _groupID, groupID, obj.toString().c_str() ) ;

      if ( primary != 0 )
      {
         _primaryNode.columns.groupID = _groupID ;
         _primaryNode.columns.nodeID = (UINT16)primary ;
         _primaryNode.columns.serviceID = 0 ;
      }

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

   const VEC_NODE_INFO* _clsGroupItem::getNodes ()
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

      _clear() ;

      std::map <UINT64, _netRouteNode>::iterator it = nodes.begin () ;
      UINT8 pos = 0 ;

      while ( it != nodes.end() )
      {
         _netRouteNode & node = it->second ;

         if ( _primaryNode.columns.nodeID == node._id.columns.nodeID )
         {
            _primaryPos = pos ;
         }

         if ( !utilCheckInstanceID( (UINT32)node._instanceID, FALSE ) )
         {
            node._instanceID = pos + 1 ;
         }

         _vecNodes.push_back ( node ) ;

         ++ it ;
         ++ pos ;
      }

      return SDB_OK ;
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

      if ( ( MSG_INVALID_ROUTEID == _primaryNode.value ||
             _primaryNode.columns.nodeID != nodeID.columns.nodeID )
            && FALSE == primary )
      {
         goto done ;
      }

      if ( !primary )
      {
         _primaryNode.value = MSG_INVALID_ROUTEID ;
         _primaryPos = CLS_RG_NODE_POS_INVALID ;
         goto done ;
      }

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

      PD_LOG ( PDERROR, "Update group primary node[%u,%u,%u] to %s error",
               nodeID.columns.groupID, nodeID.columns.nodeID,
               nodeID.columns.serviceID,
               primary ? "primary" : "slave" ) ;

   done :
      PD_TRACE_EXIT ( SDB__CLSGPIM_UPPRRIMARY ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSGPIM_UPNODESTAT, "_clsGroupItem::updateNodeStat" )
   void _clsGroupItem::updateNodeStat( UINT16 nodeID,
                                       NET_NODE_STATUS status )
   {
      PD_TRACE_ENTRY ( SDB__CLSGPIM_UPNODESTAT ) ;
      VEC_NODE_INFO_IT it = _vecNodes.begin() ;
      while ( it != _vecNodes.end() )
      {
         if ( it->_id.columns.nodeID == nodeID )
         {
            _rwMutex.lock_w() ;
            (*it).updateStatus( status, (UINT64)time(NULL) ) ;
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

   INT32 _clsNodeMgrAgent::getGroupsID( vector< UINT32 > &groups )
   {
      groups.clear() ;
      GROUP_MAP_IT it = _groupMap.begin() ;
      while ( it != _groupMap.end() )
      {
         groups.push_back( it->first ) ;
         ++it ;
      }
      return (INT32)groups.size() ;
   }

   INT32 _clsNodeMgrAgent::getGroupsName( vector< string > &groups )
   {
      groups.clear() ;
      GROUP_NAME_MAP_IT it = _groupNameMap.begin() ;
      while ( it != _groupNameMap.end() )
      {
         groups.push_back( it->first ) ;
         ++it ;
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
         _groupMap[id] = item ;
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
         if ( it->second == id )
         {
            rc = SDB_OK ;
            goto done ;
         }
      }
      _groupNameMap[name] = id ;
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

}

