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

   Source File Name = bps.hpp

   Descriptive Name = Buffer Pool Service Header

   When/how to use: this program may be used on binary and text-formatted
   versions of bufferpool service component. This file is created for further
   extension purpose. It does not contains any meaningful logic at the moment.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef BPS_HPP_
#define BPS_HPP_

#include "core.hpp"
#include "ossLatch.hpp"
#include "oss.hpp"
#include "ossQueue.hpp"
#include "bpsPrefetch.hpp"
#include "ossAtomic.hpp"
#include "sdbInterface.hpp"

namespace engine
{

   /*
      _bpsCB define
   */
   class _bpsCB : public _IControlBlock
   {
      private :
         ossQueue<_bpsPreLoadReq*>  _requestQueue ;
         ossQueue<_bpsPreLoadReq*>  _dropBackQueue ;
         ossQueue< bpsDataPref >    _dataPrefetchQueue ;
         INT32                      _numPreLoad ;
         UINT32                     _maxPrefPool ;
         std::vector<EDUID>         _preLoaderList ;

      public:
         ossAtomic32                _curPrefAgentNum ;
         ossAtomic32                _idlePrefAgentNum ;

      private :
         INT32    _addPreLoader () ;

      public :
         OSS_INLINE ossQueue<_bpsPreLoadReq*> *getReqQueue ()
         {
            return &_requestQueue ;
         }
         OSS_INLINE ossQueue<_bpsPreLoadReq*> *getDropQueue ()
         {
            return &_dropBackQueue ;
         }
         OSS_INLINE ossQueue< bpsDataPref > *getPrefetchQueue ()
         {
            return &_dataPrefetchQueue ;
         }
         OSS_INLINE BOOLEAN isPreLoadEnabled () const
         {
            return _preLoaderList.size() > 0 ;
         }
         OSS_INLINE BOOLEAN isPrefetchEnabled () const
         {
            return _maxPrefPool > 0 ? TRUE : FALSE ;
         }

      public :
         _bpsCB () :
         _numPreLoad(0), _maxPrefPool(0), _curPrefAgentNum(0),
         _idlePrefAgentNum(0)
         {}

         ~_bpsCB ()
         {
         }

         virtual SDB_CB_TYPE cbType() const { return SDB_CB_BPS ; }
         virtual const CHAR* cbName() const { return "BPSCB" ; }

         virtual INT32  init () ;
         virtual INT32  active () ;
         virtual INT32  deactive () ;
         virtual INT32  fini () ;
         virtual void   onConfigChange() ;

         INT32 sendPreLoadRequest ( const bpsPreLoadReq &request ) ;

         INT32 sendPrefechReq ( const bpsDataPref &request,
                                BOOLEAN inPref = FALSE ) ;

   } ;
   typedef class _bpsCB SDB_BPSCB ;

   /*
      get global bps cb
   */
   SDB_BPSCB* sdbGetBPSCB() ;

}

#endif /* BPS_HPP_ */

