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

   Source File Name = clsFreezingWindow.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsFreezingWindow.hpp"
#include "pmdEnv.hpp"
#include "pmdEDUMgr.hpp"
#include "rtnCB.hpp"
#include "clsMgr.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{

   /*
      _clsFreezingWindow implement
   */
   _clsFreezingWindow::_clsFreezingWindow()
   {
      _clCount = 0 ;
   }

   _clsFreezingWindow::~_clsFreezingWindow()
   {
   }

   INT32 _clsFreezingWindow::registerCL ( const CHAR *pName, UINT64 &opID )
   {
      INT32 rc = SDB_OK ;
      UINT64 oldOpID = opID ;

      ossScopedLock lock( &_latch ) ;

      /// first increase _clCount. Because waitForOpr will use _clCount without
      /// latch. If don't increase first, the case will occur:
      /// 1. split thread has run to opID = pmdAcquireGlobalID() ;
      /// 2. write thread run to pmdAcquireGlobalID(), then run to waitForOpr::
      ///    if ( isWrite && _clCount > 0 )
      /// 3. the split thread will not wait write thread, so some data will lost
      ++_clCount ;

      // operator ID is not given, acquire one for it
      // Must be acquired inside latch, which could avoid other operators to
      // acquire operator ID and pass the checking between acquiring and
      // registering
      if ( 0 == opID )
      {
         opID = pmdAcquireGlobalID() ;
      }

      if ( !pName || !*pName )
      {
         rc = SDB_INVALIDARG ;
         SDB_ASSERT( FALSE, "Name is invalid" ) ;
      }
      else
      {
         try
         {
            ossPoolString name( pName ) ;
            rc = _registerCLInternal( name, opID ) ;
         }
         catch( std::exception &e )
         {
            /// ossPoolString maybe throw exception when out of memory
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      /// last decrease _clCount
      --_clCount ;

      if ( rc && opID != oldOpID )
      {
         /// restore opID when failed
         opID = oldOpID ;
      }

      return rc ;
   }

   INT32 _clsFreezingWindow::registerCS( const CHAR *pName,
                                         UINT64 &opID )
   {
      INT32 rc = SDB_OK ;
      UINT64 oldOpID = opID ;

      ossScopedLock lock( &_latch ) ;

      /// first increase _clCount. Because waitForOpr will use _clCount without
      /// latch. If don't increase first, the case will occur:
      /// 1. split thread has run to opID = pmdAcquireGlobalID() ;
      /// 2. write thread run to pmdAcquireGlobalID(), then run to waitForOpr::
      ///    if ( isWrite && _clCount > 0 )
      /// 3. the split thread will not wait write thread, so some data will lost
      ++_clCount ;

      // operator ID is not given, acquire one for it
      // Must be acquired inside latch, which could avoid other operators to
      // acquire operator ID and pass the checking between acquiring and
      // registering
      if ( 0 == opID )
      {
         opID = pmdAcquireGlobalID() ;
      }

      if ( !pName || !*pName || ossStrchr( pName, '.' ) )
      {
         rc = SDB_INVALIDARG ;
         SDB_ASSERT( FALSE, "Name is invalid" ) ;
      }
      else
      {
         try
         {
            ossPoolString name( pName ) ;
            rc = _registerCSInternal( name, opID ) ;
         }
         catch( std::exception &e )
         {
            /// ossPoolString maybe throw exception when out of memory
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      /// last decrease _clCount
      --_clCount ;

      if ( rc && opID != oldOpID )
      {
         /// restore opID when failed
         opID = oldOpID ;
      }

      return rc ;
   }

   INT32 _clsFreezingWindow::registerWhole( UINT64 &opID  )
   {
      INT32 rc = SDB_OK ;
      UINT64 oldOpID = opID ;

      ossScopedLock lock( &_latch ) ;

      /// first increase _clCount. Because waitForOpr will use _clCount without
      /// latch. If don't increase first, the case will occur:
      /// 1. split thread has run to opID = pmdAcquireGlobalID() ;
      /// 2. write thread run to pmdAcquireGlobalID(), then run to waitForOpr::
      ///    if ( isWrite && _clCount > 0 )
      /// 3. the split thread will not wait write thread, so some data will lost
      ++_clCount ;

      // operator ID is not given, acquire one for it
      // Must be acquired inside latch, which could avoid other operators to
      // acquire operator ID and pass the checking between acquiring and
      // registering
      if ( 0 == opID )
      {
         opID = pmdAcquireGlobalID() ;
      }

      rc = _regWholeInternal( opID ) ;

      /// last decrease _clCount
      --_clCount ;

      if ( rc && opID != oldOpID )
      {
         /// restore opID when failed
         opID = oldOpID ;
      }

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFREEZWND_UPDCLTRANSWHITELST, "_clsFreezingWindow::updateCLTransWhiteList" )
   INT32 _clsFreezingWindow::updateCLTransWhiteList( const CHAR *pName,
                                                     UINT64 opID,
                                                     const DPS_TRANS_ID_SET &whiteList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSFREEZWND_UPDCLTRANSWHITELST ) ;

      SDB_ASSERT( NULL != pName, "name is invalid" ) ;
      SDB_ASSERT( 0 != opID, "block ID is invalid" ) ;

      if ( whiteList.empty() )
      {
         goto done ;
      }

      try
      {
         ossScopedLock lock( &_latch ) ;

         MAP_WINDOW::iterator iterCL ;
         OP_SET::iterator iterItem ;

         ossPoolString name( pName ) ;
         iterCL = _mapWindow.find( name ) ;
         PD_CHECK( iterCL != _mapWindow.end(), SDB_SYS, error, PDERROR,
                   "Failed to find freezing item for collection [%s]",
                   pName ) ;

         iterItem = iterCL->second.find( opID ) ;
         PD_CHECK( iterItem != iterCL->second.end(), SDB_SYS, error, PDERROR,
                   "Failed to find freezing item for collection [%s], "
                   "op [%llu]", pName, opID ) ;

         iterItem->updateTransWhiteList( whiteList ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to update white list for collection, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSFREEZWND_UPDCLTRANSWHITELST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFREEZWND_UPDCSTRANSWHITELST, "_clsFreezingWindow::updateCSTransWhiteList" )
   INT32 _clsFreezingWindow::updateCSTransWhiteList( const CHAR *pName,
                                                     UINT64 opID,
                                                     const DPS_TRANS_ID_SET &whiteList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSFREEZWND_UPDCSTRANSWHITELST ) ;

      SDB_ASSERT( NULL != pName, "name is invalid" ) ;
      SDB_ASSERT( 0 != opID, "block ID is invalid" ) ;

      if ( whiteList.empty() )
      {
         goto done ;
      }

      try
      {
         ossScopedLock lock( &_latch ) ;

         MAP_CS_WINDOW::iterator iterCS ;
         OP_SET::iterator iterItem ;

         ossPoolString name( pName ) ;
         iterCS = _mapCSWindow.find( name ) ;
         PD_CHECK( iterCS != _mapCSWindow.end(), SDB_SYS, error, PDERROR,
                   "Failed to find freezing item for collection space [%s]",
                   pName ) ;

         iterItem = iterCS->second.find( opID ) ;
         PD_CHECK( iterItem != iterCS->second.end(), SDB_SYS, error, PDERROR,
                   "Failed to find freezing item for collection space [%s], "
                   "op [%llu]", pName, opID ) ;

         iterItem->updateTransWhiteList( whiteList ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to update white list for collection space, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSFREEZWND_UPDCSTRANSWHITELST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFREEZWND_UPDWHOLETRANSWHITELST, "_clsFreezingWindow::updateWholeTransWhiteList" )
   INT32 _clsFreezingWindow::updateWholeTransWhiteList( UINT64 opID,
                                                        const DPS_TRANS_ID_SET &whiteList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSFREEZWND_UPDWHOLETRANSWHITELST ) ;

      SDB_ASSERT( 0 != opID, "block ID is invalid" ) ;

      try
      {
         ossScopedLock lock( &_latch ) ;

         OP_SET::iterator iterItem ;

         iterItem = _setWholeID.find( opID ) ;
         PD_CHECK( iterItem != _setWholeID.end(), SDB_SYS, error, PDERROR,
                   "Failed to find freezing item for whole DB, op [%llu]",
                   opID ) ;

         iterItem->updateTransWhiteList( whiteList ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to update white list for whole DB, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSFREEZWND_UPDWHOLETRANSWHITELST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFREEZWND_UPDCLCTXWHITELST, "_clsFreezingWindow::updateCLCtxWhiteList" )
   INT32 _clsFreezingWindow::updateCLCtxWhiteList( const CHAR *pName,
                                                   UINT64 opID,
                                                   const RTN_CTX_ID_SET &whiteList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSFREEZWND_UPDCLCTXWHITELST ) ;

      SDB_ASSERT( NULL != pName, "name is invalid" ) ;
      SDB_ASSERT( 0 != opID, "block ID is invalid" ) ;

      if ( whiteList.empty() )
      {
         goto done ;
      }

      try
      {
         ossScopedLock lock( &_latch ) ;

         MAP_WINDOW::iterator iterCL ;
         OP_SET::iterator iterItem ;

         ossPoolString name( pName ) ;
         iterCL = _mapWindow.find( name ) ;
         PD_CHECK( iterCL != _mapWindow.end(), SDB_SYS, error, PDERROR,
                   "Failed to find freezing item for collection [%s]",
                   pName ) ;

         iterItem = iterCL->second.find( opID ) ;
         PD_CHECK( iterItem != iterCL->second.end(), SDB_SYS, error, PDERROR,
                   "Failed to find freezing item for collection [%s], "
                   "op [%llu]", pName, opID ) ;

         iterItem->updateCtxWhiteList( whiteList ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to update white list for collection, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSFREEZWND_UPDCLCTXWHITELST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFREEZWND_UPDCSCTXWHITELST, "_clsFreezingWindow::updateCSCtxWhiteList" )
   INT32 _clsFreezingWindow::updateCSCtxWhiteList( const CHAR *pName,
                                                   UINT64 opID,
                                                   const RTN_CTX_ID_SET &whiteList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSFREEZWND_UPDCSCTXWHITELST ) ;

      SDB_ASSERT( NULL != pName, "name is invalid" ) ;
      SDB_ASSERT( 0 != opID, "block ID is invalid" ) ;

      if ( whiteList.empty() )
      {
         goto done ;
      }

      try
      {
         ossScopedLock lock( &_latch ) ;

         MAP_CS_WINDOW::iterator iterCS ;
         OP_SET::iterator iterItem ;

         ossPoolString name( pName ) ;
         iterCS = _mapCSWindow.find( name ) ;
         PD_CHECK( iterCS != _mapCSWindow.end(), SDB_SYS, error, PDERROR,
                   "Failed to find freezing item for collection space [%s]",
                   pName ) ;

         iterItem = iterCS->second.find( opID ) ;
         PD_CHECK( iterItem != iterCS->second.end(), SDB_SYS, error, PDERROR,
                   "Failed to find freezing item for collection space [%s], "
                   "op [%llu]", pName, opID ) ;

         iterItem->updateCtxWhiteList( whiteList ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to update white list for collection space, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSFREEZWND_UPDCSCTXWHITELST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFREEZWND_UPDWHOLECTXWHITELST, "_clsFreezingWindow::updateWholeCtxWhiteList" )
   INT32 _clsFreezingWindow::updateWholeCtxWhiteList( UINT64 opID,
                                                      const RTN_CTX_ID_SET &whiteList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSFREEZWND_UPDWHOLECTXWHITELST ) ;

      SDB_ASSERT( 0 != opID, "block ID is invalid" ) ;

      try
      {
         ossScopedLock lock( &_latch ) ;

         OP_SET::iterator iterItem ;

         iterItem = _setWholeID.find( opID ) ;
         PD_CHECK( iterItem != _setWholeID.end(), SDB_SYS, error, PDERROR,
                   "Failed to find freezing item for whole DB, op [%llu]",
                   opID ) ;

         iterItem->updateCtxWhiteList( whiteList ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to update white list for whole DB, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSFREEZWND_UPDWHOLECTXWHITELST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsFreezingWindow::_regWholeInternal( UINT64 opID )
   {
      INT32 rc = SDB_OK ;

      if ( _setWholeID.empty() )
      {
         ++_clCount ;
      }

      try
      {
         _setWholeID.insert( opID ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_OOM ;
         --_clCount ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return rc ;
   }

   INT32 _clsFreezingWindow::_registerCLInternal ( const ossPoolString &name,
                                                   UINT64 opID )
   {
      INT32 rc = SDB_OK ;
      MAP_WINDOW::iterator it = _mapWindow.find( name ) ;

      if ( _mapWindow.end() == it )
      {
         ++_clCount ;

         try
         {
            OP_SET newOpSet ;
            newOpSet.insert( opID ) ;

            _mapWindow[ name ] = newOpSet ;
         }
         catch( std::exception &e )
         {
            rc = SDB_OOM ;
            --_clCount ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }
      else
      {
         try
         {
            it->second.insert( opID ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      return rc ;
   }

   INT32 _clsFreezingWindow::_registerCSInternal( const ossPoolString &name,
                                                  UINT64 opID )
   {
      INT32 rc = SDB_OK ;
      MAP_CS_WINDOW::iterator it = _mapCSWindow.find( name ) ;

      if ( _mapCSWindow.end() == it )
      {
         ++_clCount ;

         try
         {
            OP_SET newOpSet ;
            newOpSet.insert( opID ) ;

            _mapCSWindow[ name ] = newOpSet ;
         }
         catch( std::exception &e )
         {
            rc = SDB_OOM ;
            --_clCount ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }
      else
      {
         try
         {
            it->second.insert( opID ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      return rc ;
   }

   void _clsFreezingWindow::unregisterCL ( const CHAR *pName, UINT64 opID )
   {
      ossScopedLock lock( &_latch ) ;

      if ( pName && *pName )
      {
         try
         {
            ossPoolString name( pName ) ;
            _unregisterCLInternal( name, opID ) ;
         }
         catch( ... )
         {
            _unregisterCLByIter( pName, opID ) ;
         }
      }

      _event.signalAll() ;
   }

   void _clsFreezingWindow::unregisterCS( const CHAR *pName, UINT64 opID )
   {
      ossScopedLock lock( &_latch ) ;

      if ( pName && *pName )
      {
         try
         {
            ossPoolString name( pName ) ;
            _unregisterCSInternal( name, opID ) ;
         }
         catch( ... )
         {
            _unregisterCSByIter( pName, opID ) ;
         }
      }

      _event.signalAll() ;
   }

   void _clsFreezingWindow::unregisterWhole( UINT64 opID )
   {
      ossScopedLock lock( &_latch ) ;

      _unregWholeInternal( opID ) ;

      _event.signalAll() ;
   }

   void _clsFreezingWindow::_unregisterCLByIter( const CHAR *pName,
                                                 UINT64 opID )
   {
      MAP_WINDOW::iterator it = _mapWindow.begin() ;

      clsFreezingItem temp( opID ) ;
      while ( it != _mapWindow.end() )
      {
         if ( 0 == ossStrcmp( it->first.c_str(), pName ) )
         {
            it->second.erase( opID ) ;

            if ( it->second.empty() )
            {
               _mapWindow.erase( it ) ;
               --_clCount ;
            }
            break ;
         }
         ++it ;
      }
   }

   void _clsFreezingWindow::_unregisterCSByIter( const CHAR *pName,
                                                 UINT64 opID )
   {
      MAP_CS_WINDOW::iterator it = _mapCSWindow.begin() ;

      while ( it != _mapCSWindow.end() )
      {
         if ( 0 == ossStrcmp( it->first._name.c_str(), pName ) )
         {
            it->second.erase( opID ) ;

            if ( it->second.empty() )
            {
               _mapCSWindow.erase( it ) ;
               --_clCount ;
            }
            break ;
         }
         ++it ;
      }
   }

   void _clsFreezingWindow::unregisterAll()
   {
      ossScopedLock lock( &_latch ) ;

      _setWholeID.clear() ;
      _mapWindow.clear() ;
      _mapCSWindow.clear() ;
      _clCount = 0 ;
   }

   void _clsFreezingWindow::_unregWholeInternal( UINT64 opID )
   {
      _setWholeID.erase( opID ) ;
      if ( _setWholeID.empty() )
      {
         --_clCount ;
      }
   }

   void _clsFreezingWindow::_unregisterCLInternal ( const ossPoolString &name,
                                                    UINT64 opID )
   {
      MAP_WINDOW::iterator it = _mapWindow.find( name ) ;

      if ( _mapWindow.end() != it )
      {
         it->second.erase( opID ) ;

         if ( it->second.empty() )
         {
            _mapWindow.erase( it ) ;
            --_clCount ;
         }
      }
   }

   void _clsFreezingWindow::_unregisterCSInternal( const ossPoolString &name,
                                                   UINT64 opID )
   {
      MAP_CS_WINDOW::iterator it = _mapCSWindow.find( name ) ;

      if ( _mapCSWindow.end() != it )
      {
         it->second.erase( opID ) ;

         if ( it->second.empty() )
         {
            _mapCSWindow.erase( it ) ;
            --_clCount ;
         }
      }
   }

   void _clsFreezingWindow::_blockCheck( const OP_SET &setID,
                                         UINT64 testOPID,
                                         _pmdEDUCB *cb,
                                         BOOLEAN &result,
                                         BOOLEAN &forceEnd )
   {
      DPS_TRANS_ID transID = cb->getTransID() ;
      INT64 contextID = cb->getCurrentContextID() ;
      OP_SET::const_iterator cit = setID.begin() ;
      while ( cit != setID.end () )
      {
         if ( *cit == testOPID )
         {
            // Self
            result = FALSE ;
            forceEnd = TRUE ;
            break ;
         }
         else if ( *cit < testOPID &&
                   !cit->isInTransWhiteList( transID ) &&
                   !cit->isInCtxWhiteList( contextID ) )
         {
            result = TRUE ;
            break ;
         }
         ++ cit ;
      }
   }

   BOOLEAN _clsFreezingWindow::needBlockOpr( const ossPoolString &name,
                                             UINT64 testOpID,
                                             _pmdEDUCB *cb )
   {
      MAP_WINDOW::iterator it ;
      MAP_CS_WINDOW::iterator itCS ;
      BOOLEAN needBlock = FALSE ;
      BOOLEAN forceEnd = FALSE ;

      ossScopedLock lock( &_latch ) ;

      /// whole block check
      if ( !_setWholeID.empty() )
      {
         _blockCheck( _setWholeID, testOpID, cb, needBlock, forceEnd ) ;
         if ( forceEnd )
         {
            goto done ;
         }
      }

      /// cs block check
      if ( !_mapCSWindow.empty() &&
           _mapCSWindow.end() != ( itCS = _mapCSWindow.find( name ) ) )
      {
         _blockCheck( itCS->second, testOpID, cb, needBlock, forceEnd ) ;
         if ( forceEnd )
         {
            goto done ;
         }
      }

      /// cl block check
      if ( !_mapWindow.empty() &&
           _mapWindow.end() != ( it = _mapWindow.find( name ) ) )
      {
         _blockCheck( it->second, testOpID, cb, needBlock, forceEnd ) ;
      }

   done:
      return needBlock ;
   }

   INT32 _clsFreezingWindow::waitForOpr( const CHAR *pName,
                                         _pmdEDUCB *cb,
                                         BOOLEAN isWrite )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasBlock = FALSE ;

      if ( isWrite && _clCount > 0 )
      {
         try
         {
            ossPoolString clName( pName ) ;
            BOOLEAN needBlock = TRUE ;
            MAP_WINDOW::iterator it ;
            UINT64 opID = cb->getWritingID() ;

            while( needBlock )
            {
               if ( cb->isInterrupted() )
               {
                  rc = SDB_APP_INTERRUPT ;
                  break ;
               }

               needBlock = needBlockOpr( clName, opID, cb ) ;
               if ( needBlock )
               {
                  if ( !hasBlock )
                  {
                     cb->setBlock( EDU_BLOCK_FREEZING_WND, "" ) ;
                     cb->printInfo( EDU_INFO_DOING,
                                    "Waiting for freezing window(Name:%s)",
                                    pName ) ;
                     hasBlock = TRUE ;
                  }

                  _event.reset() ;
                  _event.wait( OSS_ONE_SEC ) ;
               }
            }
         }
         catch( std::exception &e )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      if ( hasBlock )
      {
         cb->unsetBlock() ;
      }
      return rc ;
   }

   /*
      _clsFreezingChecker implement
    */
   _clsFreezingChecker::_clsFreezingChecker( clsFreezingWindow *freezingWindow,
                                             UINT64 blockID,
                                             const CHAR *objName )
   : _freezingWindow( freezingWindow ),
     _checkMask( 0 ),
     _enableMask( 0 ),
     _step( CLS_FREEZING_CHECKER_EDU ),
     _blockID( blockID ),
     _objName( objName ),
     _excludeBlockType( -1 ),
     _transLockID( DMS_INVALID_LOGICCSID, DMS_INVALID_MBID, NULL ),
     _transLockType( DPS_TRANSLOCK_MAX ),
     _canSelfIncomp( FALSE ),
     _selfEDUID( PMD_INVALID_EDUID )
   {
      SDB_ASSERT( NULL != freezingWindow, "freezing window should be valid" ) ;
   }

   _clsFreezingChecker::~_clsFreezingChecker()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFREEZCHK_ENABLEEDUCHK, "_clsFreezingChecker::enableEDUCheck" )
   INT32 _clsFreezingChecker::enableEDUCheck( pmdEDUCB *cb,
                                              EDU_BLOCK_TYPE excludeBlockType )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSFREEZCHK_ENABLEEDUCHK ) ;

      if ( !OSS_BIT_TEST( _checkMask, CLS_FREEZING_CHECKER_MASK_EDU ) )
      {
         _excludeBlockType = excludeBlockType ;
         OSS_BIT_SET( _checkMask, CLS_FREEZING_CHECKER_MASK_EDU ) ;
      }

      PD_TRACE_EXITRC( SDB__CLSFREEZCHK_ENABLEEDUCHK, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFREEZCHK_ENABLECTXCHK, "_clsFreezingChecker::enableCtxCheck" )
   INT32 _clsFreezingChecker::enableCtxCheck( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSFREEZCHK_ENABLECTXCHK ) ;

      if ( !OSS_BIT_TEST( _checkMask, CLS_FREEZING_CHECKER_MASK_CTX ) )
      {
         _selfEDUID = cb->getID() ;

         rc = _getCtxBlockList( cb, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get context block list, "
                      "rc: %d", rc ) ;

         OSS_BIT_SET( _checkMask, CLS_FREEZING_CHECKER_MASK_CTX ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSFREEZCHK_ENABLECTXCHK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFREEZCHK_ENABLETRANSCHK, "_clsFreezingChecker::enableTransCheck" )
   INT32 _clsFreezingChecker::enableTransCheck( pmdEDUCB *cb,
                                                UINT32 logicalCSID,
                                                UINT16 collectionID,
                                                DPS_TRANSLOCK_TYPE transLockType,
                                                BOOLEAN canSelfIncomp )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSFREEZCHK_ENABLETRANSCHK ) ;

      if ( !OSS_BIT_TEST( _checkMask, CLS_FREEZING_CHECKER_MASK_TRANS ) )
      {
         _transLockID = dpsTransLockId( logicalCSID, collectionID, NULL ) ;
         _transLockType = transLockType ;
         _canSelfIncomp = canSelfIncomp ;

         rc = _getTransBlockList( cb, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get transaction block list, "
                      "rc: %d", rc ) ;

         OSS_BIT_SET( _checkMask, CLS_FREEZING_CHECKER_MASK_TRANS ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSFREEZCHK_ENABLETRANSCHK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFREEZCHK_CHK, "_clsFreezingChecker::check" )
   INT32 _clsFreezingChecker::check( pmdEDUCB *cb, clsFreezingCheckResult &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSFREEZCHK_CHK ) ;

      result._isPassed = FALSE ;

      // Step 1. check writing EDU with blocking ID, if no smaller
      //         operation ID than blocking ID on the same collection,
      //         it means all running operations on the same collection
      //         before blocking ID had been finished
      if ( CLS_FREEZING_CHECKER_EDU == _step )
      {
         if ( OSS_BIT_TEST( _checkMask, CLS_FREEZING_CHECKER_MASK_EDU ) )
         {
            rc = _checkEDUs( cb, result ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check EDUs, "
                         "rc: %d", rc ) ;
            if ( !( result._isPassed ) )
            {
               goto done ;
            }
         }
         _step = CLS_FREEZING_CHECKER_CTX ;
      }

      // Step 2. check writing contexts before blocking ID, if context
      //         is writing a related collection or collection space,
      //         we need to wait for them
      if ( CLS_FREEZING_CHECKER_CTX == _step )
      {
         if ( OSS_BIT_TEST( _checkMask, CLS_FREEZING_CHECKER_MASK_CTX ) )
         {
            rc = _checkContexts( cb, result ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check contexts, "
                         "rc: %d", rc ) ;
            if ( !( result._isPassed ) )
            {
               goto done ;
            }
         }
         _step = CLS_FREEZING_CHECKER_TRANS ;
      }

      // Step 3. check transaction with incompatible locks on the same
      //         collection, if no incompatible transactions, it means all
      //         running transactions on the same collection had been
      //         finished, otherwise, add the incompatible transactions
      //         as white list for blocking, so they won't be blocked
      if ( CLS_FREEZING_CHECKER_TRANS == _step )
      {
         if ( OSS_BIT_TEST( _checkMask, CLS_FREEZING_CHECKER_MASK_TRANS ) )
         {
            rc = _checkTransactions( cb, result ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check transactions, "
                         "rc: %d", rc ) ;
            if ( !( result._isPassed ) )
            {
               goto done ;
            }
         }
         _step = CLS_FREEZING_CHECKER_DONE ;
      }

      result._isPassed = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__CLSFREEZCHK_CHK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFREEZCHK_LOOPCHK, "_clsFreezingChecker::loopCheck" )
   INT32 _clsFreezingChecker::loopCheck( pmdEDUCB *cb,
                                         UINT32 maxTimes,
                                         IContext *context )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSFREEZCHK_LOOPCHK ) ;

      UINT32 retryCount = 0 ;

      if ( NULL != context )
      {
         rc = context->pause() ;
         PD_RC_CHECK( rc, PDERROR, "Freezing checker [%s]: failed to pause "
                      "context, rc: %d", _objName, rc ) ;
      }

      while ( TRUE )
      {
         clsFreezingCheckResult result ;

         ++ retryCount ;

         rc = check( cb, result ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check freezing window, "
                      "rc: %d", rc ) ;
         if ( !( result._isPassed ) )
         {
            if ( cb->isInterrupted() )
            {
               // current session is interrupted
               rc = SDB_APP_INTERRUPT ;
            }
            else if ( retryCount < maxTimes )
            {
               // can retry
               ossSleep( OSS_ONE_SEC ) ;
               continue ;
            }
            if ( CLS_FREEZING_CHECKER_EDU == result._step )
            {
               rc = SDB_LOCK_FAILED ;
               PD_LOG_MSG( PDERROR, "Failed to wait for other write EDUs "
                           "to finish, [%s] blocked by EDU [%u] "
                           "block ID [%llu]", _objName, result._blockEDUID,
                           result._blockID ) ;
            }
            else if ( CLS_FREEZING_CHECKER_CTX == result._step )
            {
               rc = SDB_LOCK_FAILED ;
               PD_LOG_MSG( PDERROR, "Failed to wait for other write contexts "
                           "to finish, [%s] blocked by context [%lld] "
                           "block ID [%llu], total %u blocking context",
                           _objName, result._blockCtxID, result._blockID,
                           result._blockCtxNum ) ;
            }
            else if ( CLS_FREEZING_CHECKER_TRANS == result._step )
            {
               // timeout to wait for transaction
               rc = SDB_DPS_TRANS_LOCK_INCOMPATIBLE ;
               PD_LOG_MSG( PDERROR, "Failed to wait for other write "
                           "transactions to finish, [%s] blocked by "
                           "transaction [%s], total %u blocking transactions",
                           _objName,
                           dpsTransIDToString( result._blockTransID ).c_str(),
                           result._blockTransNum ) ;
            }
            else
            {
               SDB_ASSERT( FALSE, "check step is invalid" ) ;
               rc = SDB_LOCK_FAILED ;
            }
            goto error ;
         }

         // passed check
         break ;
      }

      if ( NULL != context )
      {
         rc = context->resume() ;
         PD_RC_CHECK( rc, PDERROR, "Freezing checker [%s]: failed to resume "
                      "context, rc: %d", _objName, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSFREEZCHK_LOOPCHK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFREEZCHK__CHKEDU, "_clsFreezingChecker::_checkEDUs" )
   INT32 _clsFreezingChecker::_checkEDUs( pmdEDUCB *cb,
                                          clsFreezingCheckResult &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSFREEZCHK__CHKEDU ) ;

      pmdEDUMgr *pEDUMgr = cb->getEDUMgr() ;
      PMD_EDU_PROCESS_LIST writingEDUList ;
      BOOLEAN hasWriting = FALSE ;

      result._isPassed = FALSE ;

      rc = pEDUMgr->getWritingEDUs( -1, _blockID, _excludeBlockType,
                                    writingEDUList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get writing EDUs, rc: %d", rc ) ;

      for ( PMD_EDU_PROCESS_LIST::iterator iter = writingEDUList.begin() ;
            iter != writingEDUList.end() ;
            ++ iter )
      {
         UINT64 opID = iter->_opID ;
         const CHAR *processName = iter->_processName.c_str() ;
         BOOLEAN isRelated = FALSE ;

         // check if EDU is related
         rc = _isRelated( cb, processName, isRelated ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check related process "
                      "object [%s], rc: %d", processName, rc ) ;

         if ( isRelated )
         {
            result._blockID = opID ;
            result._blockEDUID = iter->_eduID ;
            result._step = CLS_FREEZING_CHECKER_EDU ;
            result._isPassed = FALSE ;
            hasWriting = TRUE ;
            break ;
         }
      }

      if ( !hasWriting )
      {
         result._isPassed = TRUE ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSFREEZCHK__CHKEDU, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFREEZCHK__CHKCTX, "_clsFreezingChecker::_checkContexts" )
   INT32 _clsFreezingChecker::_checkContexts( pmdEDUCB *cb,
                                              clsFreezingCheckResult &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSFREEZCHK__CHKCTX ) ;

      RTN_CTX_ID_SET blockList ;

      result._isPassed = FALSE ;

      rc = _getCtxBlockList( cb, &result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get context block list, rc: %d",
                   rc ) ;

      if ( result._blockCtxNum > 0 )
      {
         result._isPassed = FALSE ;
         result._step = CLS_FREEZING_CHECKER_CTX ;
      }
      else
      {
         result._isPassed = TRUE ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSFREEZCHK__CHKCTX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFREEZCHK__CHKTRANS, "_clsFreezingChecker::_checkTransactions" )
   INT32 _clsFreezingChecker::_checkTransactions( pmdEDUCB *cb,
                                                  clsFreezingCheckResult &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSFREEZCHK__CHKTRANS ) ;

      result._isPassed = FALSE ;

      rc = _getTransBlockList( cb, &result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get transaction block list, rc: %d",
                   rc ) ;

      if ( result._blockTransNum > 0 )
      {
         result._isPassed = FALSE ;
         result._step = CLS_FREEZING_CHECKER_TRANS ;
      }
      else
      {
         result._isPassed = TRUE ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSFREEZCHK__CHKTRANS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFREEZCHK__GETCTXBLOCKLIST, "_clsFreezingChecker::_getCtxBlockList" )
   INT32 _clsFreezingChecker::_getCtxBlockList( pmdEDUCB *cb,
                                                clsFreezingCheckResult *result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSFREEZCHK__GETCTXBLOCKLIST ) ;

      RTN_CTX_ID_SET blockList ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;

      // get writing context before blocking ID
      RTN_CTX_PROCESS_LIST contextProcessList ;
      rc = rtnCB->dumpWritingContext( contextProcessList, _selfEDUID,
                                      _blockID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get processing writing "
                   "contexts, rc: %d", rc ) ;

      // check if blocking contexts are related
      for ( RTN_CTX_PROCESS_LIST::iterator iter = contextProcessList.begin() ;
            iter != contextProcessList.end() ;
            ++ iter )
      {
         BOOLEAN isRelated = FALSE ;

         INT64 contextID = iter->_ctxID ;
         const CHAR *processName = iter->_processName.c_str() ;

         rc = _isRelated( cb, processName, isRelated ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check related processing "
                      "object, rc: %d", rc ) ;

         // if related, we need to wait for them to finish
         if ( isRelated )
         {
            if ( NULL != result )
            {
               if ( 0 == result->_blockCtxNum )
               {
                  result->_blockID = iter->_opID ;
                  result->_blockCtxID = iter->_ctxID ;
                  result->_blockEDUID = iter->_eduID ;
               }
               ++ ( result->_blockCtxNum ) ;
            }

            try
            {
               blockList.insert( contextID ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to save exclude context list, "
                       "occur exception %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }
         }
      }

      if ( blockList.size() > 0 )
      {
         // add related contexts to white list, so they can finish
         rc = _updateCtxWhiteList( blockList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add context white list "
                      "for [%s], rc: %d", _objName, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSFREEZCHK__GETCTXBLOCKLIST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFREEZCHK__GETTRANSBLOCKLIST, "_clsFreezingChecker::_getTransBlockList" )
   INT32 _clsFreezingChecker::_getTransBlockList( pmdEDUCB *cb,
                                                  clsFreezingCheckResult *result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSFREEZCHK__GETTRANSBLOCKLIST ) ;

      DPS_TRANS_ID_SET blockList ;
      dpsTransCB *transCB = sdbGetTransCB() ;

      // get incompatible transactions by lock again
      rc = transCB->getIncompTrans( cb,
                                    _transLockID,
                                    _transLockType,
                                    _canSelfIncomp,
                                    blockList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get incompatible transactions "
                   "for [%s], rc: %d", _objName, rc ) ;

      if ( blockList.size() > 0 )
      {
         if ( NULL != result )
         {
            result->_blockTransNum = blockList.size() ;
            result->_blockTransID = *( blockList.begin() ) ;
         }

         // still have incompatible transactions, update the white list
         rc = _updateTransWhiteList( blockList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add transaction white list "
                      "for [%s], rc: %d", _objName, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSFREEZCHK__GETTRANSBLOCKLIST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _clsFreezingCLChecker implement
    */
   _clsFreezingCLChecker::_clsFreezingCLChecker( clsFreezingWindow *freezingWindow,
                                                 UINT64 blockID,
                                                 const CHAR *clName,
                                                 const CHAR *mainCLName )
   : _clsFreezingChecker( freezingWindow, blockID, clName ),
     _mainCLName( mainCLName )
   {
   }

   _clsFreezingCLChecker::~_clsFreezingCLChecker()
   {
   }

   INT32 _clsFreezingCLChecker::_isRelated( pmdEDUCB *cb,
                                            const CHAR *objName,
                                            BOOLEAN &isRelated )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != objName, "object name is invalid" ) ;

      isRelated = ( _isRelated( _objName, objName ) ) ||
                  ( _isRelated( _mainCLName, objName ) ) ;

      return rc ;
   }

   BOOLEAN _clsFreezingCLChecker::_isRelated( const CHAR *selfName,
                                              const CHAR *objName )
   {
      BOOLEAN isRelated = FALSE ;

      if ( NULL != selfName && 0 != selfName[ 0 ] &&
           NULL != objName && 0 != objName[ 0 ] )
      {
         UINT32 objNameLen = ossStrlen( objName ) ;
         if ( 0 == ossStrncmp( selfName, objName, objNameLen ) )
         {
            // they are related if
            // - obj is the same collection
            // - obj is the same collection space
            if ( ( 0 == selfName[ objNameLen ] ) ||
                 ( '.' == selfName[ objNameLen ] ) )
            {
               isRelated = TRUE ;
            }
         }
      }

      return isRelated ;
   }

   /*
      _clsFreezingCSChecker implement
    */
   _clsFreezingCSChecker::_clsFreezingCSChecker( clsFreezingWindow *freezingWindow,
                                                 UINT64 blockID,
                                                 const CHAR *csName )
   : _clsFreezingChecker( freezingWindow, blockID, csName )
   {
   }

   _clsFreezingCSChecker::~_clsFreezingCSChecker()
   {
   }

   INT32 _clsFreezingCSChecker::_isRelated( pmdEDUCB *cb,
                                            const CHAR *objName,
                                            BOOLEAN &isRelated )
   {
      INT32 rc = SDB_OK ;

      isRelated = TRUE ;

      SDB_ASSERT( NULL != objName, "object name is invalid" ) ;

      if ( ( !_isRelated( _objName, objName ) ) &&
           ( NULL != objName ) &&
           ( 0 != objName[ 0 ] ) )
      {
         if ( NULL != ossStrchr( objName, '.' ) )
         {
            // check if it is a main-colection and has sub-collections in
            // checking collection space
            clsCatalogSet* pCatSet = NULL ;
            rc = sdbGetShardCB()->getAndLockCataSet( objName, &pCatSet ) ;
            if ( SDB_DMS_NOTEXIST == rc || SDB_DMS_CS_NOTEXIST == rc )
            {
               // it is dropped, no need to care
               isRelated = FALSE ;
               rc = SDB_OK ;
            }
            else if ( SDB_CLS_NO_CATALOG_INFO == rc )
            {
               // don't know the status, regarded as related
               isRelated = TRUE ;
               rc = SDB_OK ;
            }
            else if ( ( NULL != pCatSet ) &&
                      ( ( !pCatSet->isMainCL() ) ||
                        ( !pCatSet->hasSubCLLocateOnCS( _objName ) ) ) )
            {
               // not main-collection or no sub-collections in checking
               // collection space
               isRelated = FALSE ;
            }
            sdbGetShardCB()->unlockCataSet( pCatSet ) ;
         }
         else
         {
            // it is a collection space, not related
            isRelated = FALSE ;
         }
      }

      return rc ;
   }

   BOOLEAN _clsFreezingCSChecker::_isRelated( const CHAR *selfName,
                                              const CHAR *objName )
   {
      BOOLEAN isRelated = FALSE ;

      if ( NULL != selfName && 0 != selfName[ 0 ] &&
           NULL != objName && 0 != objName[ 0 ] )
      {
         UINT32 selfNameLen = ossStrlen( selfName ) ;
         if ( 0 == ossStrncmp( selfName, objName, selfNameLen ) )
         {
            // they are related if
            // - obj is the same collection space
            // - obj is in the same collection space
            if ( ( 0 == objName[ selfNameLen ] ) ||
                 ( '.' == objName[ selfNameLen ] ) )
            {
               isRelated = TRUE ;
            }
         }
      }

      return isRelated ;
   }

}
