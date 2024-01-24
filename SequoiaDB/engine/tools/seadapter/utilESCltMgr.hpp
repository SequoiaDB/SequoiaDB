/*******************************************************************************


   Copyright (C) 2011-2019 SequoiaDB Ltd.

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

   Source File Name = utilESCltMgr.hpp

   Descriptive Name = Elasticsearch client manager.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          23/08/2019  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_ESCLT_MGR_HPP__
#define UTIL_ESCLT_MGR_HPP__

#include "ossLatch.hpp"
#include "utilList.hpp"
#include "utilESClt.hpp"
#include "seAdptDef.hpp"

using namespace engine ;

#define UTIL_ESCLT_DFT_MAX_NUM         (50)
#define UTIL_ESCLT_DFT_CLEAN_INTERVAL  (30 * 60 * 1000)

namespace seadapter
{
   /*
    * Elasticsearch client manager. It manages all the clients connected to ES.
    */
   class _utilESCltMgr : public SDBObject
   {
      typedef _utilList<utilESClt *, UTIL_ESCLT_DFT_MAX_NUM>  CLT_LIST ;
      typedef CLT_LIST::iterator CLT_LIST_ITR ;
      public:
         _utilESCltMgr() ;
         ~_utilESCltMgr() ;

         INT32 init( const CHAR *url, UINT32 limit, UINT32 cleanInterval,
                     INT32 opTimeout ) ;
         void setScrollSize( UINT16 size ) ;
         OSS_INLINE UINT16 getScrollSize() const ;

         INT32 getClient( utilESClt *&client ) ;
         void releaseClient( utilESClt *&client ) ;
         void cleanup() ;

      private:
         CHAR  _url[ SEADPT_SE_SVCADDR_MAX_SZ + 1 ] ;  // Search engine address
         UINT32 _limit ;
         UINT32 _cleanInterval ;
         INT32 _opTimeout ;
         UINT32 _number ;
         ossSpinXLatch _latch ;
         CLT_LIST _cltList ;
         UINT16 _scrollSize ;
   } ;
   typedef _utilESCltMgr utilESCltMgr ;

   OSS_INLINE UINT16 utilESCltMgr::getScrollSize() const
   {
      return _scrollSize ;
   }

   extern utilESCltMgr es_clt_mgr ;
   OSS_INLINE utilESCltMgr* utilGetESCltMgr()
   {
      return &es_clt_mgr ;
   }
}

#endif /* UTIL_ESCLT_MGR_HPP__ */
