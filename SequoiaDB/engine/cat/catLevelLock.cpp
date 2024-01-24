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

   Source File Name = catLevelLock.cpp

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
          2015/01/17  XJH Init

*******************************************************************************/

#include "catLevelLock.hpp"
#include "catalogueCB.hpp"
#include "catCommon.hpp"
#include "rtn.hpp"

using namespace std ;

namespace engine
{

   #define CAT_MAX_LATCH_SIZE             ( 16 )

   /*
      _catLockTreeNode implement
   */
   _catLockTreeNode::_catLockTreeNode()
   {
      _latch     = NULL ;
      _mgr       = NULL ;
      _parent    = NULL ;
      _type      = CAT_LOCK_MAX ;
      _lockType  = -1 ;
      _lockCount = 0L ;
   }

   _catLockTreeNode::~_catLockTreeNode()
   {
      SDB_ASSERT( -1 == _lockType && 0L == _lockCount,
                  "lock should be freed" ) ;
      _latch   = NULL ;
      _mgr     = NULL ;
      _parent  = NULL ;
   }

   BOOLEAN _catLockTreeNode::isEmpty()
   {
      if ( _mapSubs.size() > 0 )
      {
         return FALSE ;
      }
      if ( _lockType != -1 )
      {
         return FALSE ;
      }
      if ( _lockCount != 0L )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   _catLockTreeNode* _catLockTreeNode::getSubTreeNode( const string & name )
   {
      _catLockTreeNode &subTree = _mapSubs[ name ] ;
      if ( NULL == subTree._getLatch() )
      {
         subTree._setLatch( _mgr->getLatch() ) ;
      }
      if ( NULL == subTree._getParent() )
      {
         subTree._setParent( this ) ;
         subTree._setName( name ) ;
         subTree._setMgr( _mgr ) ;
      }
      return &subTree ;
   }

   void _catLockTreeNode::releaseSubTreeNode( _catLockTreeNode *subTreeNode )
   {
      if ( subTreeNode->isEmpty() )
      {
         if ( subTreeNode->_getLatch() )
         {
            _mgr->releaseLatch( subTreeNode->_getLatch() ) ;
            subTreeNode->_setLatch( NULL ) ;
         }
         _mapSubs.erase( subTreeNode->_getName() ) ;
      }
   }

   BOOLEAN _catLockTreeNode::tryLock( OSS_LATCH_MODE mode )
   {
      BOOLEAN lock = FALSE ;

      if ( _isZeroLevel() ) // zero level
      {
         lock = TRUE ;
      }
      else if ( _latch )
      {
         if ( EXCLUSIVE == mode )
         {
            lock = _latch->try_get() ;
         }
         else
         {
            lock = _latch->try_get_shared() ;
         }
      }

      if ( lock )
      {
         _lockType = (INT32)mode ;
         _lockCount++ ;
      }
      return lock ;
   }

   void _catLockTreeNode::unLock()
   {
      if ( -1 == _lockType || 0L == _lockCount )
      {
         SDB_ASSERT( -1 == _lockType && 0L == _lockCount,
                     "lock should be freed" ) ;
         return ;
      }

      if ( !_isZeroLevel() )  // not zero level
      {
         if ( EXCLUSIVE == _lockType )
         {
            _latch->release() ;
         }
         else
         {
            _latch->release_shared() ;
         }
      }
      _lockCount-- ;
      if ( 0L == _lockCount )
      {
         _lockType = -1 ;
      }
   }

   void _catLockTreeNode::_setLatch( ossSpinSLatch *latch )
   {
      _latch = latch ;
   }

   void _catLockTreeNode::_setMgr( _catLevelLockMgr *mgr )
   {
      _mgr = mgr ;
   }

   void _catLockTreeNode::_setType( INT32 type )
   {
      _type = type ;
   }

   void _catLockTreeNode::_setName( const string &name )
   {
      _name = name ;
   }

   void _catLockTreeNode::_setParent( _catLockTreeNode *parent )
   {
      _parent = parent ;
   }

   BOOLEAN _catLockTreeNode::_isZeroLevel() const
   {
      return ( CAT_LOCK_MAX != _type ) ? TRUE : FALSE ;
   }

   /*
      _catLevelLockMgr implement
   */
   _catLevelLockMgr::_catLevelLockMgr()
   {
   }

   _catLevelLockMgr::~_catLevelLockMgr()
   {
      for ( UINT32 i = 0 ; i < _vecLatch.size() ; ++i )
      {
         SDB_OSS_DEL _vecLatch[ i ] ;
      }
      _vecLatch.clear() ;
      _mapType2Lock.clear() ;
   }

   catLockTreeNode* _catLevelLockMgr::getLockTreeNode( INT32 type )
   {
      SDB_ASSERT( CAT_LOCK_MAX != type, "Invalid type" ) ;

      catLockTreeNode &lockTree = _mapType2Lock[ type ] ;
      if ( NULL == lockTree._getMgr() )
      {
         // zero level don't need latch
         lockTree._setType( type ) ;
         lockTree._setMgr( this ) ;
      }
      return &lockTree ;
   }

   void _catLevelLockMgr::releaseLockTreeNode( catLockTreeNode *lockTreeNode )
   {
      if ( lockTreeNode->isEmpty() )
      {
         if ( lockTreeNode->_getLatch() )
         {
            releaseLatch( lockTreeNode->_getLatch() ) ;
            lockTreeNode->_setLatch( NULL ) ;
         }
         _mapType2Lock.erase( lockTreeNode->_getType() ) ;
      }
   }

   ossSpinSLatch* _catLevelLockMgr::getLatch()
   {
      ossSpinSLatch *latch = NULL ;
      vector< ossSpinSLatch* >::iterator it = _vecLatch.begin() ;
      if ( it != _vecLatch.end() )
      {
         latch = *it ;
         _vecLatch.erase( it ) ;
      }
      else
      {
         latch = SDB_OSS_NEW ossSpinSLatch() ;
      }
      return latch ;
   }

   void _catLevelLockMgr::releaseLatch( ossSpinSLatch * latch )
   {
      if ( NULL == latch )
      {
         return ;
      }
      if ( _vecLatch.size() < CAT_MAX_LATCH_SIZE )
      {
         _vecLatch.push_back( latch ) ;
      }
      else
      {
         SDB_OSS_DEL latch ;
      }
   }

   /*
      _catZeroLevelLock implement
   */
   _catZeroLevelLock::_catZeroLevelLock( CAT_LOCK_TYPE type )
   {
      _mgr           = sdbGetCatalogueCB()->getLevelLockMgr() ;
      _type          = type ;
      _zeroLevelNode = NULL ;

      SDB_ASSERT( CAT_LOCK_MAX != _type, "Invalid type" ) ;
   }

   _catZeroLevelLock::~_catZeroLevelLock()
   {
      unLock() ;
   }

   BOOLEAN _catZeroLevelLock::tryLock( OSS_LATCH_MODE mode )
   {
      BOOLEAN locked = FALSE ;

      if ( NULL == _zeroLevelNode )
      {
         _zeroLevelNode = _mgr->getLockTreeNode( _type ) ;
      }

      if ( _zeroLevelNode )
      {
         locked = _zeroLevelNode->tryLock( mode ) ;
         if ( !locked )
         {
            _zeroLevelNode = NULL ;
            goto error ;
         }
      }

   done:
      return locked ;
   error:
      unLock() ;
      goto done ;
   }

   void _catZeroLevelLock::unLock()
   {
      if ( _zeroLevelNode )
      {
         _zeroLevelNode->unLock() ;
         _mgr->releaseLockTreeNode( _zeroLevelNode ) ;
         _zeroLevelNode = NULL ;
      }
   }

   /*
      _catOneLevelLock implement
   */
   _catOneLevelLock::_catOneLevelLock( CAT_LOCK_TYPE type,
                                       const string &level1Name )
   :_catZeroLevelLock( type )
   {
      _level1Name       = level1Name ;
      _oneLevelNode     = NULL ;
   }

   _catOneLevelLock::~_catOneLevelLock()
   {
      unLock() ;
   }

   void _catOneLevelLock::setLevel1Name( const string &name )
   {
      _level1Name = name ;
   }

   BOOLEAN _catOneLevelLock::tryLock( OSS_LATCH_MODE mode )
   {
      BOOLEAN locked = FALSE ;

      // first lock zero
      // then lock one
      if ( _catZeroLevelLock::tryLock( SHARED ) )
      {
         if ( NULL == _oneLevelNode )
         {
            _oneLevelNode = _zeroLevelNode->getSubTreeNode( _level1Name ) ;
         }

         if ( _oneLevelNode )
         {
            locked = _oneLevelNode->tryLock( mode ) ;
            if ( !locked )
            {
               _oneLevelNode = NULL ;
               goto error ;
            }
         }
      }

   done:
      return locked ;
   error:
      unLock() ;
      goto done ;
   }

   void _catOneLevelLock::unLock()
   {
      if ( _oneLevelNode )
      {
         _oneLevelNode->unLock() ;
         _zeroLevelNode->releaseSubTreeNode( _oneLevelNode ) ;
         _oneLevelNode = NULL ;
      }
      _catZeroLevelLock::unLock() ;
   }

   /*
      _catTwoLevelLock implement
   */
   _catTwoLevelLock::_catTwoLevelLock( CAT_LOCK_TYPE type,
                                       const string &level1Name,
                                       const string &level2Name )
   :_catOneLevelLock( type, level1Name )
   {
      _level2Name          = level2Name ;
      _twoLevelNode        = NULL ;
   }

   _catTwoLevelLock::~_catTwoLevelLock()
   {
      unLock() ;
   }

   void _catTwoLevelLock::setLevel2Name( const string &name )
   {
      _level2Name = name ;
   }

   BOOLEAN _catTwoLevelLock::tryLock( OSS_LATCH_MODE mode )
   {
      BOOLEAN locked = FALSE ;

      // first lock one
      // then lock two
      if ( _catOneLevelLock::tryLock( SHARED ) )
      {
         if ( NULL == _twoLevelNode )
         {
            _twoLevelNode = _oneLevelNode->getSubTreeNode( _level2Name ) ;
         }

         if ( _twoLevelNode )
         {
            locked = _twoLevelNode->tryLock( mode ) ;
            if ( !locked )
            {
               _twoLevelNode = NULL ;
               goto error ;
            }
         }
      }

   done:
      return locked ;
   error:
      unLock() ;
      goto done ;
   }

   void _catTwoLevelLock::unLock()
   {
      if ( _twoLevelNode )
      {
         _twoLevelNode->unLock() ;
         _oneLevelNode->releaseSubTreeNode( _twoLevelNode ) ;
         _twoLevelNode = NULL ;
      }
      _catOneLevelLock::unLock() ;
   }

   /*
      _catCSLock implement
   */
   _catCSLock::_catCSLock( const string &csName )
   :_catOneLevelLock( CAT_LOCK_DATA, csName )
   {
   }
   _catCSLock::~_catCSLock()
   {
   }

   /*
      _catCLLock implement
   */
   _catCLLock::_catCLLock( const string &csName, const string &clName )
   :_catTwoLevelLock( CAT_LOCK_DATA, csName, clName )
   {
   }
   _catCLLock::~_catCLLock()
   {
   }

   /*
      _catCSShardingLock implement
    */
   _catCSShardingLock::_catCSShardingLock ( const string & csName )
   : _catOneLevelLock( CAT_LOCK_SHARDING, csName )
   {
   }

   _catCSShardingLock::~_catCSShardingLock ()
   {
   }

   /*
      _catCLShardingLock implement
    */
   _catCLShardingLock::_catCLShardingLock ( const string & csName, const string & clName )
   : _catTwoLevelLock( CAT_LOCK_SHARDING, csName, clName )
   {
   }

   _catCLShardingLock::~_catCLShardingLock ()
   {
   }

   /*
      _catGroupLock implement
   */
   _catGroupLock::_catGroupLock( const string &groupName )
   :_catOneLevelLock( CAT_LOCK_NODE, groupName )
   {
   }
   _catGroupLock::~_catGroupLock()
   {
   }

   /*
      _catNodeLock implement
   */
   _catNodeLock::_catNodeLock( const string &groupName,
                               const string &nodeName )
   :_catTwoLevelLock( CAT_LOCK_NODE, groupName, nodeName )
   {
   }
   _catNodeLock::~_catNodeLock()
   {
   }

   /*
      _catDomainLock implement
   */
   _catDomainLock::_catDomainLock( const string &domainName )
   :_catOneLevelLock( CAT_LOCK_DOMAIN, domainName )
   {
   }
   _catDomainLock::~_catDomainLock()
   {
   }

   /*
      _catRecycleBinLock implement
    */
   _catRecycleBinLock::_catRecycleBinLock()
   : _catZeroLevelLock( CAT_LOCK_RECYCLEBIN )
   {
   }

   _catRecycleBinLock::~_catRecycleBinLock()
   {
   }

   /*
      _catRecycleCSLock implement
    */
   _catCSRecycleLock::_catCSRecycleLock( const string &csName )
   : _catOneLevelLock( CAT_LOCK_RECYCLEBIN, csName )
   {
   }

   _catCSRecycleLock::~_catCSRecycleLock()
   {
   }

   /*
      _catRecycleCLLock implement
    */
   _catCLRecycleLock::_catCLRecycleLock( const std::string &csName,
                                         const std::string &clFullName )
   : _catTwoLevelLock( CAT_LOCK_RECYCLEBIN, csName, clFullName )
   {
   }

   _catCLRecycleLock::~_catCLRecycleLock()
   {
   }

   /*
      _catCtxLockMgr implement
    */
   _catCtxLockMgr::_catCtxLockMgr ()
   : _ignoreLock( FALSE )
   {
   }

   _catCtxLockMgr::~_catCtxLockMgr ()
   {
      // Make sure objects are unlocked
      unlockObjects() ;
   }

   BOOLEAN _catCtxLockMgr::tryLockCollectionSpace (
         const std::string &csName,
         OSS_LATCH_MODE mode )
   {
      return _tryLockObject ( CAT_LOCK_DATA, csName, mode ) ;
   }

   BOOLEAN _catCtxLockMgr::tryLockCollection (
         const std::string &csName,
         const std::string &clFullName,
         OSS_LATCH_MODE mode )
   {
      return _tryLockObject ( CAT_LOCK_DATA, csName, clFullName, mode ) ;
   }

   BOOLEAN _catCtxLockMgr::tryLockCollection (
         const std::string &clFullName,
         OSS_LATCH_MODE mode )
   {
      INT32 rc = SDB_OK ;
      CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;

      // Resolve collection space from collection full name
      rc = rtnResolveCollectionSpaceName( clFullName.c_str(),
                                          clFullName.size(),
                                          csName,
                                          DMS_COLLECTION_SPACE_NAME_SZ ) ;
      if ( SDB_OK != rc )
      {
         return FALSE ;
      }
      return _tryLockObject ( CAT_LOCK_DATA, csName, clFullName, mode ) ;
   }

   BOOLEAN _catCtxLockMgr::tryLockCollectionSpaceSharding ( const string & csName,
                                                            OSS_LATCH_MODE mode )
   {
      return _tryLockObject( CAT_LOCK_SHARDING, csName, mode ) ;
   }

   BOOLEAN _catCtxLockMgr::tryLockCollectionSharding ( const string & csName,
                                                       const string & clFullName,
                                                       OSS_LATCH_MODE mode )
   {
      return _tryLockObject( CAT_LOCK_SHARDING, csName, clFullName, mode ) ;
   }

   BOOLEAN _catCtxLockMgr::tryLockCollectionSharding ( const string & clFullName,
                                                       OSS_LATCH_MODE mode )
   {
      INT32 rc = SDB_OK ;
      CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;

      // Resolve collection space from collection full name
      rc = rtnResolveCollectionSpaceName( clFullName.c_str(),
                                          clFullName.size(),
                                          csName,
                                          DMS_COLLECTION_SPACE_NAME_SZ ) ;
      if ( SDB_OK != rc )
      {
         return FALSE ;
      }

      return tryLockCollectionSharding( csName, clFullName, mode ) ;
   }

   BOOLEAN _catCtxLockMgr::tryLockDomain (
         const std::string &domainName,
         OSS_LATCH_MODE mode )
   {
      return _tryLockObject ( CAT_LOCK_DOMAIN, domainName, mode ) ;
   }

   BOOLEAN _catCtxLockMgr::tryLockGroup(
         const std::string &groupName,
         OSS_LATCH_MODE mode )
   {
      return _tryLockObject ( CAT_LOCK_NODE, groupName, mode ) ;
   }

   BOOLEAN _catCtxLockMgr::tryLockNode ( const std::string &groupName,
                                         const std::string &nodeName,
                                         OSS_LATCH_MODE mode )
   {
      return _tryLockObject ( CAT_LOCK_NODE, groupName, nodeName, mode ) ;
   }

   BOOLEAN _catCtxLockMgr::tryLockRecycleItem( const utilRecycleItem &recycleItem,
                                               OSS_LATCH_MODE mode )
   {
      if ( UTIL_RECYCLE_CS == recycleItem.getType() )
      {
         string levelOneName ;

         try
         {
            StringBuilder levelOneBuilder ;
            levelOneBuilder << recycleItem.getOriginID() ;
            levelOneName = levelOneBuilder.str() ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to build lock name for "
                    "recycle item, occur exception %s", e.what() ) ;
            return FALSE ;
         }

         return _tryLockObject( CAT_LOCK_RECYCLEBIN,
                                levelOneName,
                                mode ) ;
      }
      else
      {
         SDB_ASSERT( UTIL_RECYCLE_CL == recycleItem.getType(),
                     "Should be collection recycle item" ) ;

         string levelOneName, levelTwoName ;

         try
         {
            StringBuilder levelOneBuilder, levelTwoBuilder ;
            utilCSUniqueID csUniqueID =
                  utilGetCSUniqueID( recycleItem.getOriginID() ) ;

            levelOneBuilder << csUniqueID ;
            levelTwoBuilder << csUniqueID << "." << recycleItem.getRecycleID() ;

            levelOneName = levelOneBuilder.str() ;
            levelTwoName = levelTwoBuilder.str() ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to build lock name for "
                    "recycle item, occur exception %s", e.what() ) ;
            return FALSE ;
         }

         return _tryLockObject( CAT_LOCK_RECYCLEBIN,
                                levelOneName,
                                levelTwoName,
                                mode ) ;
      }
   }

   BOOLEAN _catCtxLockMgr::_tryLockObject ( CAT_LOCK_TYPE type,
                                            const std::string &name,
                                            OSS_LATCH_MODE mode )
   {
      catOneLevelLock *pLock = NULL ;

      if ( _ignoreLock )
      {
         return TRUE ;
      }

      // Create lock by lock type
      switch ( type )
      {
      case CAT_LOCK_DATA :
         pLock = SDB_OSS_NEW catCSLock ( name ) ;
         break ;
      case CAT_LOCK_NODE :
         pLock = SDB_OSS_NEW catGroupLock ( name ) ;
         break ;
      case CAT_LOCK_DOMAIN :
         pLock = SDB_OSS_NEW catDomainLock ( name ) ;
         break ;
      case CAT_LOCK_SHARDING :
         pLock = SDB_OSS_NEW catCSShardingLock( name ) ;
         break ;
      case CAT_LOCK_RECYCLEBIN :
         pLock = SDB_OSS_NEW catCSRecycleLock( name ) ;
         break ;
      default :
         break ;
      }

      return _tryLockObject ( pLock, mode ) ;
   }

   BOOLEAN _catCtxLockMgr::_tryLockObject ( CAT_LOCK_TYPE type,
                                            const std::string &parentName,
                                            const std::string &name,
                                            OSS_LATCH_MODE mode )
   {
      catOneLevelLock *pLock = NULL ;

      SDB_ASSERT( parentName != name, "Names of 2 levels are the same" ) ;

      if ( _ignoreLock )
      {
         return TRUE ;
      }

      // Create lock by lock type
      switch ( type )
      {
      case CAT_LOCK_DATA :
         pLock = SDB_OSS_NEW catCLLock ( parentName, name ) ;
         break ;
      case CAT_LOCK_NODE :
         pLock = SDB_OSS_NEW catNodeLock ( parentName, name ) ;
         break ;
      case CAT_LOCK_SHARDING :
         pLock = SDB_OSS_NEW catCLShardingLock( parentName, name ) ;
         break ;
      case CAT_LOCK_RECYCLEBIN :
         pLock = SDB_OSS_NEW catCLRecycleLock( parentName, name ) ;
         break ;
      default :
         break ;
      }

      return _tryLockObject ( pLock, mode ) ;
   }

   BOOLEAN _catCtxLockMgr::_tryLockObject ( catOneLevelLock *pLock,
                                            OSS_LATCH_MODE mode )
   {
      if ( !pLock )
      {
         return FALSE ;
      }

      if ( _ignoreLock )
      {
         SAFE_OSS_DELETE( pLock ) ;
         return TRUE ;
      }

      if ( pLock->tryLock( mode ) )
      {
         _lockList.push_back( pLock ) ;
         return TRUE ;
      }

      SDB_OSS_DEL pLock ;
      return FALSE ;
   }

   void _catCtxLockMgr::unlockObjects ()
   {
      LOCK_LIST::iterator iterLock = _lockList.begin () ;
      while ( iterLock != _lockList.end() )
      {
         catOneLevelLock *pLock = ( *iterLock ) ;
         iterLock = _lockList.erase( iterLock ) ;
         // Make sure the object is unlocked
         pLock->unLock();
         SDB_OSS_DEL pLock;
      }
   }
}


