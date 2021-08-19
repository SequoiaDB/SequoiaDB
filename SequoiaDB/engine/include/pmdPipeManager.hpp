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

   Source File Name = pmdPipeManager.hpp

   Descriptive Name = Process MoDel Pipe Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for pipe
   manager.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_PIPE_MANAGER_HPP__
#define PMD_PIPE_MANAGER_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossNPipe.hpp"
#include "ossMemPool.hpp"
#include "pmd.hpp"
#include "utilNodeOpr.hpp"
#include "utilPooledObject.hpp"

namespace engine
{

   #define PMD_NPIPE_BUFSZ ( 1024 )

   class _pmdPipeManager ;
   typedef class _pmdPipeManager pmdPipeManager ;

   /*
      _IPmdPipeHandler define
    */
   class _IPmdPipeHandler
   {
   public:
      _IPmdPipeHandler()
      {
      }

      virtual ~_IPmdPipeHandler()
      {
      }

   public:
      virtual INT32 processMessage( CHAR *message,
                                    utilNodePipe &nodePipe ) = 0 ;
   } ;

   typedef class _IPmdPipeHandler IPmdPipeHandler ;

   /*
      _pmdSystemPipeHandler define
    */
   // pmdSystemPipeHandler handles message to stop process
   class _pmdSystemPipeHandler : public IPmdPipeHandler,
                                 public SDBObject
   {
   public:
      _pmdSystemPipeHandler() ;
      virtual ~_pmdSystemPipeHandler() ;

   public:
      virtual INT32 processMessage( CHAR *message,
                                    utilNodePipe &nodePipe ) ;

   protected:
      BOOLEAN _closedStdFds ;
   } ;

   typedef class _pmdSystemPipeHandler pmdSystemPipeHandler ;

   /*
      _pmdInfoPipeHandler define
    */
   // pmdInfoPipeHandler handles messages to acquire system information
   class _pmdInfoPipeHandler : public IPmdPipeHandler,
                               public utilPooledObject
   {
   public:
      _pmdInfoPipeHandler() ;
      virtual ~_pmdInfoPipeHandler() ;

   public:
      virtual INT32 processMessage( CHAR *message,
                                    utilNodePipe &nodePipe ) ;
   } ;

   typedef class _pmdInfoPipeHandler pmdInfoPipeHandler ;

   /*
      _pmdPipeManager define
    */
   class _pmdPipeManager : public SDBObject
   {
   public:
      _pmdPipeManager() ;
      ~_pmdPipeManager() ;

   public:
      // initialize pipe manager
      // system pipe will include system and info handlers
      INT32 init( const CHAR *serviceName,
                  BOOLEAN systemPipe ) ;
      // finalize pipe manager
      INT32 fini() ;

      // active pipe manager
      INT32 active() ;

      // deactive pipe manager
      INT32 deactive() ;

      // helper to start EDU for pipe manager
      INT32 startEDU() ;

      // add handler
      INT32 registerHandler( IPmdPipeHandler *handler ) ;

      // remove all handlers
      void unregisterAllHandlers() ;

      // main function to run pipe manager
      INT32 run( pmdEDUCB *cb ) ;

      // process message with registered handlers
      INT32 processMessage( CHAR *message,
                            INT32 messageSize,
                            utilNodePipe &nodePipe ) ;

   public:
      OSS_INLINE BOOLEAN isInitialized() const
      {
         return _initialized ;
      }

      OSS_INLINE const CHAR *getServiceName() const
      {
         return _serviceName.c_str() ;
      }

   protected:
      typedef ossPoolList< IPmdPipeHandler * > PMD_PIPE_HANDLE_LIST ;

   protected:
      BOOLEAN              _initialized ;
      BOOLEAN              _systemPipe ;
      ossPoolString        _serviceName ;
      utilNodePipe         _nodePipe ;
      PMD_PIPE_HANDLE_LIST _handlers ;
      EDUID                _eduID ;
   } ;

   pmdPipeManager *sdbGetSystemPipeManager() ;

}

#endif // PMD_PIPE_MANAGER_HPP__
