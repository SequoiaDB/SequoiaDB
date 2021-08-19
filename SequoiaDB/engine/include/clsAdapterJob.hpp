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

   Source File Name = clsAdapterJob.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/07/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_ADAPTER_JOB_HPP_
#define CLS_ADAPTER_JOB_HPP_

#include "rtnBackgroundJob.hpp"
#include "../bson/bson.h"
#include "netDef.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   /*
      _clsAdapterJobBase define
    */
   class _clsAdapterJobBase : public _rtnBaseJob
   {
      public :
         _clsAdapterJobBase ( const MsgHeader * requestHeader,
                              const bson::BSONObj & requestObject,
                              const NET_HANDLE & handle ) ;
         virtual ~_clsAdapterJobBase () ;

      public :
         OSS_INLINE virtual const CHAR * name () const
         {
            return _name.c_str() ;
         }

         virtual INT32 doit () ;

      protected :
         OSS_INLINE virtual void _onAttach ()
         {
         }

         OSS_INLINE void _makeName ()
         {
            StringBuilder ss ;
            ss << "AdapterJob[" << _requestHeader.opCode << "]-[" <<
                  _requestObject.toString() << "]" ;
            _name = ss.str() ;
         }

      protected :
         virtual INT32 _handleMessage ( bson::BSONObjBuilder & replyBuilder ) = 0 ;

      protected :
         std::string    _name ;
         MsgHeader      _requestHeader ;
         bson::BSONObj  _requestObject ;
         NET_HANDLE     _handle ;

   } ;

   /*
      _clsAdapterDumpTextIndexJob define
    */
   class _clsAdapterDumpTextIndexJob : public _clsAdapterJobBase
   {
      public :
         _clsAdapterDumpTextIndexJob ( const MsgHeader * requestHeader,
                                       const bson::BSONObj & requestObject,
                                       INT64 peerVersion,
                                       INT64 localVersion,
                                       const NET_HANDLE & handle ) ;

         virtual ~_clsAdapterDumpTextIndexJob () ;

      public :
         OSS_INLINE virtual RTN_JOB_TYPE type () const
         {
            return RTN_JOB_CLS_ADAPTER_TEXT_INDEX ;
         }

         virtual BOOLEAN muteXOn ( const _rtnBaseJob * other ) ;

      protected :
         INT32 _handleMessage ( bson::BSONObjBuilder & replyBuilder ) ;
         INT32 _dumpAllTextIndexInfo ( bson::BSONObjBuilder & replyBuilder,
                                       INT32 & totalTextIndexCount ) ;
         void _dumpTextIndexInfo ( const monCLSimple & clInfo,
                                   const monIndex & idxInfo,
                                   bson::BSONArrayBuilder & idxBuilder ) ;

      protected :
         INT64 _peerVersion ;
         INT64 _localVersion ;
   } ;

   typedef class _clsAdapterDumpTextIndexJob clsAdapterDumpTextIndexJob ;

   INT32 clsStartAdapterDumpTextIndexJob ( const MsgHeader * requestMessage,
                                           const bson::BSONObj & requestObject,
                                           INT64 peerVersion,
                                           INT64 localVersion,
                                           const NET_HANDLE & handle,
                                           EDUID * eduID = NULL,
                                           BOOLEAN returnResult = FALSE ) ;

}

#endif // CLS_ADAPTER_JOB_HPP_
