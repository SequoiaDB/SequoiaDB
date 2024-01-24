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

   Source File Name = pmdAccessProtocolBase.hpp

   Descriptive Name = Process MoDel Engine Dispatchable Unit Event Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure for events that
   used as inter-EDU communications.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/08/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_ACCESSPROTOCOLBASE_HPP__
#define PMD_ACCESSPROTOCOLBASE_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "sdbInterface.hpp"
#include "pmdSession.hpp"
#include "pmdIProcessor.hpp"

namespace engine
{

   /*
      _IPmdAccessProtocol define
   */
   class _IPmdAccessProtocol : public SDBObject
   {
      public:
         _IPmdAccessProtocol() {}
         virtual ~_IPmdAccessProtocol() {}

         virtual const CHAR*     name() const = 0 ;
         /*
            0 for unlimited
         */
         virtual UINT32          maxConnNum() const { return 0 ; }

      public:
         virtual INT32           init( IResource *pResource ) = 0 ;
         virtual INT32           active() = 0 ;
         virtual INT32           deactive() = 0 ;
         virtual INT32           fini() = 0 ;

         virtual const CHAR*     getServiceName() const = 0 ;
         virtual pmdSession*     getSession( SOCKET fd ) = 0 ;
         virtual void            releaseSession( pmdSession *pSession ) = 0 ;

   } ;
   typedef _IPmdAccessProtocol IPmdAccessProtocol ;

   /*
      export accessprotocol define
      must include in accessprotocol cpp file
   */
   #define PMD_EXPORT_ACCESSPROTOCOL_DLL( apClass ) \
   SDB_EXTERN_C_START \
   SDB_EXPORT IPmdAccessProtocol *createAccessProtocol() \
   { \
      return SDB_OSS_NEW apClass() ; \
   } \
   SDB_EXPORT void releaseAccessProtocol( IPmdAccessProtocol *&pAccessProtocol ) \
   { \
      if ( NULL != pAccessProtocol ) \
      { \
         SDB_OSS_DEL pAccessProtocol ; \
         pAccessProtocol = NULL ;      \
      } \
   } \
   SDB_EXTERN_C_END

}

#endif // PMD_ACCESSPROTOCOLBASE_HPP__

