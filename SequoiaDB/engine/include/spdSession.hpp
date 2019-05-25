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

   Source File Name = spdSession.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPDSESSION_HPP_
#define SPDSESSION_HPP_

#include "core.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{
   class _spdFuncDownloader ;
   class _pmdEDUCB ;
   class _spdFMPMgr ;
   class _spdFMP ;

   class _spdSession : public SDBObject
   {
   public:
      _spdSession() ;
      virtual ~_spdSession() ;

   public:
      INT32 eval( const BSONObj &procedures,
                  _spdFuncDownloader *downloader,
                  _pmdEDUCB *cb ) ;

      INT32 next( BSONObj &obj ) ;

      const BSONObj &getErrMsg() { return _errmsg ; }
      const BSONObj &getRetMsg() { return _resmsg ; }
      INT32 resType() const { return _resType ; }
   private:
      INT32 _eval( const BSONObj &procedures,
                   _spdFuncDownloader *downloader ) ;

      INT32 _resIsOk( const BSONObj &res ) ;

   private:
      BSONObj _resmsg ;
      BSONObj _errmsg ;
      INT32 _resType ;
      _spdFMPMgr *_fmpMgr ;
      _pmdEDUCB *_cb ;
      _spdFMP *_fmp ;
   } ;

   typedef class _spdSession spdSession ;
}

#endif

