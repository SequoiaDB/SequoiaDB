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

   Source File Name = catLevelLock.hpp

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

#ifndef CAT_LEVEL_LOCK_HPP__
#define CAT_LEVEL_LOCK_HPP__

#include "core.hpp"
#include "pd.hpp"
#include "oss.hpp"
#include "ossErr.h"
#include "ossLatch.hpp"
#include <string>
#include <map>
#include <vector>

using namespace std ;

namespace engine
{

   class _catLevelLockMgr ;
   class _catLockTreeNode ;

   /*
      CAT_LOCK_TYPE define
   */
   enum CAT_LOCK_TYPE
   {
      CAT_LOCK_DATA        = 1,  // for collectionspace,collection
      CAT_LOCK_NODE        = 2,  // for group,node
      CAT_LOCK_DOMAIN      = 3,  // for domain
      CAT_LOCK_SHARDING    = 4,  // for sharding

      CAT_LOCK_MAX
   } ;

   /*
      _catLockTreeNode define
   */
   class _catLockTreeNode : public SDBObject
   {
      friend class _catLevelLockMgr ;

      public:
         _catLockTreeNode() ;
         ~_catLockTreeNode() ;

         BOOLEAN              isEmpty() ;
         _catLockTreeNode*    getSubTreeNode( const string &name ) ;
         void                 releaseSubTreeNode( _catLockTreeNode *subTree ) ;

         BOOLEAN              tryLock( OSS_LATCH_MODE mode ) ;
         void                 unLock() ;

      protected:
         void                 _setLatch( ossSpinSLatch *latch ) ;
         ossSpinSLatch*       _getLatch() { return _latch ; }

         void                 _setMgr( _catLevelLockMgr *mgr ) ;
         _catLevelLockMgr*    _getMgr() { return _mgr ; }

         void                 _setType( INT32 type ) ;
         INT32                _getType() const { return _type ; }

         void                 _setName( const string &name ) ;
         string               _getName() const { return _name ; }

         _catLockTreeNode*    _getParent() { return _parent ; }
         void                 _setParent( _catLockTreeNode *parent ) ;

         BOOLEAN              _isZeroLevel() const ;

      private:
         map< string, _catLockTreeNode >     _mapSubs ;

         ossSpinSLatch*                      _latch ;
         _catLevelLockMgr*                   _mgr ;
         _catLockTreeNode*                   _parent ;
         INT32                               _type ;
         INT32                               _lockType ;
         string                              _name ;
         UINT64                              _lockCount ;
   } ;
   typedef _catLockTreeNode catLockTreeNode ;

   /*
      _catLevelLockMgr define
   */
   class _catLevelLockMgr : public SDBObject
   {
      public:
         _catLevelLockMgr() ;
         ~_catLevelLockMgr() ;

         ossSpinSLatch*    getLatch() ;
         void              releaseLatch( ossSpinSLatch *latch ) ;

         catLockTreeNode*  getLockTreeNode( INT32 type ) ;
         void              releaseLockTreeNode( catLockTreeNode *lockTreeNode ) ;

      private:
         vector< ossSpinSLatch* >            _vecLatch ;
         map< INT32, catLockTreeNode >       _mapType2Lock ;

   } ;
   typedef _catLevelLockMgr catLevelLockMgr ;

   /*
      _catZeroLevelLock define
   */
   class _catZeroLevelLock : public SDBObject
   {
      public:
         _catZeroLevelLock( CAT_LOCK_TYPE type ) ;
         virtual ~_catZeroLevelLock() ;

         virtual BOOLEAN   tryLock( OSS_LATCH_MODE mode ) ;
         virtual void      unLock() ;

      protected:
         catLevelLockMgr         *_mgr ;
         catLockTreeNode         *_zeroLevelNode ;
         CAT_LOCK_TYPE           _type ;

      private:
         // forbidden copy construct and equal assignment
         _catZeroLevelLock( const _catZeroLevelLock &right ) ;
         _catZeroLevelLock& operator=( const _catZeroLevelLock &right ) ;

   } ;
   typedef _catZeroLevelLock catZeroLevelLock ;

   /*
      _catOneLevelLock define
   */
   class _catOneLevelLock : public _catZeroLevelLock
   {
      public:
         _catOneLevelLock( CAT_LOCK_TYPE type,
                           const string &level1Name ) ;
         virtual ~_catOneLevelLock() ;

         virtual BOOLEAN   tryLock( OSS_LATCH_MODE mode ) ;
         virtual void      unLock() ;

         void              setLevel1Name( const string &name ) ;

      protected:
         string            _level1Name ;
         catLockTreeNode   *_oneLevelNode ;

   } ;
   typedef _catOneLevelLock catOneLevelLock ;

   /*
      _catTwoLevelLock define
   */
   class _catTwoLevelLock : public _catOneLevelLock
   {
      public:
         _catTwoLevelLock( CAT_LOCK_TYPE type,
                           const string &level1Name,
                           const string &level2Name ) ;
         virtual ~_catTwoLevelLock() ;

         virtual BOOLEAN   tryLock( OSS_LATCH_MODE mode ) ;
         virtual void      unLock() ;

         void              setLevel2Name( const string &name ) ;

      protected:
         string            _level2Name ;
         catLockTreeNode   *_twoLevelNode ;

   } ;
   typedef _catTwoLevelLock catTwoLevelLock ;

   /*
      _catCSLock define
   */
   class _catCSLock : public _catOneLevelLock
   {
      public:
         _catCSLock( const string &csName ) ;
         virtual ~_catCSLock() ;
   } ;
   typedef _catCSLock catCSLock ;

   /*
      _catCLLock define
   */
   class _catCLLock : public _catTwoLevelLock
   {
      public:
         _catCLLock( const string &csName, const string &clName ) ;
         virtual ~_catCLLock() ;
   } ;
   typedef _catCLLock catCLLock ;

   /*
      _catCSShardingLock define
    */
   class _catCSShardingLock : public _catOneLevelLock
   {
      public :
         _catCSShardingLock ( const string & csName ) ;
         virtual ~_catCSShardingLock () ;
   } ;

   typedef class _catCSShardingLock catCSShardingLock ;

   /*
      _catCLShardingLock define
    */
   class _catCLShardingLock : public _catTwoLevelLock
   {
      public :
         _catCLShardingLock ( const string & csName, const string & clName ) ;
         virtual ~_catCLShardingLock () ;
   } ;

   typedef class _catCLShardingLock catCLShardingLock ;

   /*
      _catGroupLock define
   */
   class _catGroupLock : public _catOneLevelLock
   {
      public:
         _catGroupLock( const string &groupName ) ;
         virtual ~_catGroupLock() ;
   } ;
   typedef _catGroupLock catGroupLock ;

   /*
      _catNodeLock define
   */
   class _catNodeLock : public _catTwoLevelLock
   {
      public:
         _catNodeLock( const string &groupName, const string &nodeName ) ;
         virtual ~_catNodeLock() ;
   } ;
   typedef _catNodeLock catNodeLock ;

   /*
      _catDomainLock define
   */
   class _catDomainLock : public _catOneLevelLock
   {
      public:
         _catDomainLock( const string &domainName ) ;
         virtual ~_catDomainLock() ;
   } ;
   typedef _catDomainLock catDomainLock ;

   /*
       _catCtxLockMgr define
    */
   class _catCtxLockMgr : public SDBObject
   {
   protected:
      typedef std::vector< catOneLevelLock *> LOCK_LIST ;

   public:
      _catCtxLockMgr () ;
      ~_catCtxLockMgr () ;

   public:
      // _catCtxLockMgr functions
      BOOLEAN tryLockCollectionSpace ( const std::string &csName,
                                       OSS_LATCH_MODE mode ) ;

      BOOLEAN tryLockCollection ( const std::string &csName,
                                  const std::string &clFullName,
                                  OSS_LATCH_MODE mode ) ;

      BOOLEAN tryLockCollection ( const std::string &clFullName,
                                  OSS_LATCH_MODE mode ) ;

      BOOLEAN tryLockCollectionSpaceSharding ( const std::string & csName,
                                               OSS_LATCH_MODE mode ) ;

      BOOLEAN tryLockCollectionSharding ( const std::string & clFullName,
                                          OSS_LATCH_MODE mode ) ;

      BOOLEAN tryLockCollectionSharding ( const std::string & csName,
                                          const std::string & clFullName,
                                          OSS_LATCH_MODE mode ) ;

      BOOLEAN tryLockDomain ( const std::string &domainName,
                              OSS_LATCH_MODE mode ) ;

      BOOLEAN tryLockGroup ( const std::string &groupName,
                             OSS_LATCH_MODE mode ) ;

      BOOLEAN tryLockNode ( const std::string &groupName,
                            const std::string &nodeName,
                            OSS_LATCH_MODE mode ) ;

      void unlockObjects () ;

   protected:
      BOOLEAN _tryLockObject ( CAT_LOCK_TYPE type,
                               const std::string &name,
                               OSS_LATCH_MODE mode ) ;

      BOOLEAN _tryLockObject ( CAT_LOCK_TYPE type,
                               const std::string &parentName,
                               const std::string &name,
                               OSS_LATCH_MODE mode ) ;

      BOOLEAN _tryLockObject ( catOneLevelLock *pLock, OSS_LATCH_MODE mode ) ;

   protected:
      LOCK_LIST _lockList ;
   } ;

   typedef class _catCtxLockMgr catCtxLockMgr ;
}

#endif // CAT_LEVEL_LOCK_HPP__

