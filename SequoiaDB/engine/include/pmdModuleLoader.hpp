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

   Source File Name = pmdModuleLoader.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/27/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MODULE_LOADER_HPP_
#define _SDB_MODULE_LOADER_HPP_

#include "oss.hpp"
#include "ossDynamicLoad.hpp"
#include "ossTypes.h"
#include "pmdAccessProtocolBase.hpp"

#define OSS_FAP_CREATE  ( IPmdAccessProtocol*(*)() )
#define OSS_FAP_RELEASE ( void(*)(IPmdAccessProtocol *&) )
#define CREATE_FAP_NAME "createAccessProtocol"
#define RELEASE_FAP_NAME "releaseAccessProtocol"

#define FAP_MODULE_NAME_PREFIX "fap"
#define FAP_MODULE_PATH FAP_MODULE_NAME_PREFIX OSS_FILE_SEP
#define FAP_MODULE_NAME_SIZE 255
#define MONGO_MODULE_NAME "mongo"


namespace engine
{
   /*
      _pmdEDUParam define
   */
   class _pmdEDUParam : public SDBObject
   {
   public:
      _pmdEDUParam() : pSocket( NULL ), protocol( NULL )
      {}

      virtual ~_pmdEDUParam()
      {
         pSocket = NULL ;
         protocol = NULL ;
      }

      void               *pSocket ;
      IPmdAccessProtocol *protocol ;
   };
   typedef _pmdEDUParam pmdEDUParam ;

   /*
      _pmdModuleLoader define
   */
   class _pmdModuleLoader : public SDBObject
   {
   public:
      _pmdModuleLoader() ;
      virtual ~_pmdModuleLoader() ;

      INT32 getFunction( const CHAR *funcName,
                         OSS_MODULE_PFUNCTION *function ) ;
      INT32 create ( IPmdAccessProtocol *&protocol ) ;
      INT32 release( IPmdAccessProtocol *&protocol ) ;

      INT32 load( const CHAR *mudule, const CHAR *path,
                  UINT32 mode = 0 ) ;
      void  unload() ;

   protected:
      OSS_MODULE_PFUNCTION  _function ;
      ossModuleHandle      *_loadModule ;
   };

   typedef _pmdModuleLoader pmdModuleLoader ;
}

#endif // _SDB_MODULE_LOADER_HPP_
