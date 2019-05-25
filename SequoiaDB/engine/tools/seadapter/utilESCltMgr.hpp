/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_SECLT_MGR_HPP_
#define UTIL_SECLT_MGR_HPP_

#include "ossLatch.hpp"
#include "utilESClt.hpp"

#define UTIL_ESCLT_DFT_CACHE_NUM    100
namespace seadapter
{
   class _utilESCltMgr : public SDBObject
   {
   public:
      _utilESCltMgr( UINT32 cacheNum = UTIL_ESCLT_DFT_CACHE_NUM ) ;
      ~_utilESCltMgr() ;

      INT32 init( const std::string &url ) ;
      INT32 getClt( utilESClt **seClt ) ;
      void releaseClt( utilESClt *&seClt ) ;

   private:
      std::string          _url ;      // Search engine address
      UINT32               _cacheNum ;
      ossSpinXLatch        _latch ;
      vector<utilESClt *>  _vecSEClt ;
   } ;
   typedef _utilESCltMgr utilESCltMgr ;
}

#endif /* UTIL_SECLT_MGR_HPP_ */

