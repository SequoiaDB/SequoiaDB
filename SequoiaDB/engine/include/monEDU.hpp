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

   Source File Name = monEDU.hpp

   Descriptive Name = Monitor Engine Dispatchable Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of monitoring component. This file contains structure for
   EDU snapshot/list.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MONEDU_HPP_
#define MONEDU_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "monCB.hpp"
#include "ossMemPool.hpp"
#include <string>

namespace engine
{

   /*
      Common Define
   */
   #define MON_EDU_STATUS_SZ        (19)
   #define MON_EDU_TYPE_SZ          (19)
   #define MON_EDU_NAME_SZ          (127)
   #define MON_EDU_DOING_SZ         (128)

   /*
      _monEDUSimple define
   */
   class _monEDUSimple : public SDBObject
   {
   public :
      UINT64   _eduID ;
      UINT32   _tid ;
      CHAR     _eduStatus[MON_EDU_STATUS_SZ+1] ;
      CHAR     _eduType[MON_EDU_TYPE_SZ+1] ;
      CHAR     _eduName[MON_EDU_NAME_SZ+1] ;
      CHAR     _source[MON_EDU_NAME_SZ+1] ;
      UINT64   _relatedNID ;
      UINT64   _relatedEDUID ;
      UINT32   _relatedTID ;

      _monEDUSimple()
      {
         ossMemset ( _eduStatus, 0, sizeof(_eduStatus) ) ;
         ossMemset ( _eduType, 0, sizeof(_eduType) ) ;
         ossMemset ( _eduName, 0, sizeof(_eduName) ) ;
         ossMemset ( _source, 0, sizeof(_source) ) ;
         _eduID = 0 ;
         _tid = 0 ;
         _relatedNID = 0 ;
         _relatedEDUID = PMD_INVALID_EDUID ;
         _relatedTID = 0 ;
      }
      OSS_INLINE BOOLEAN operator< (const _monEDUSimple &r) const
      {
         return _eduID < r._eduID ;
      }
   } ;
   typedef _monEDUSimple monEDUSimple ;

   /*
      _monEDUFull Define
   */
   class _monEDUFull : public SDBObject
   {
   public :
      UINT64   _eduID ;
      UINT32   _tid ;
      UINT32   _queueSize ;
      UINT32   _memPoolSize ;
      UINT64   _processEventCount ;
      BOOLEAN  _isBlock ;
      CHAR     _eduStatus[MON_EDU_STATUS_SZ+1] ;
      CHAR     _eduType[MON_EDU_TYPE_SZ+1] ;
      CHAR     _eduName[MON_EDU_NAME_SZ+1] ;
      CHAR     _doing[MON_EDU_DOING_SZ+1] ;
      CHAR     _source[MON_EDU_NAME_SZ+1] ;
      UINT64   _relatedNID ;
      UINT64   _relatedEDUID ;
      UINT32   _relatedTID ;
      ossPoolSet<SINT64> _eduContextList ;

      monAppCB _monApplCB ;

   #if defined ( _WINDOWS )
      HANDLE _threadHdl ;
   #elif defined ( _LINUX )
      OSSTID _threadHdl ;
   #endif

      _monEDUFull()
      {
         ossMemset ( _eduStatus, 0, sizeof(_eduStatus) ) ;
         ossMemset ( _eduType, 0, sizeof(_eduType) ) ;
         ossMemset ( _eduName, 0, sizeof(_eduName) ) ;
         ossMemset ( _doing, 0, sizeof(_doing) ) ;
         ossMemset ( _source, 0, sizeof(_source) ) ;
         _eduID = 0 ;
         _tid = 0 ;
         _queueSize = 0 ;
         _memPoolSize = 0 ;
         _processEventCount = 0 ;
         _isBlock = FALSE ;
         _threadHdl = 0 ;
         _relatedNID = 0 ;
         _relatedEDUID = PMD_INVALID_EDUID ;
         _relatedTID = 0 ;
      }
      OSS_INLINE BOOLEAN operator< (const _monEDUFull &r) const
      {
         return _eduID < r._eduID ;
      }
      _monEDUFull &operator= ( const _monEDUFull &rhs )
      {
         _eduID          = rhs._eduID ;
         _tid            = rhs._tid ;
         _queueSize      = rhs._queueSize ;
         _memPoolSize    = rhs._memPoolSize ;
         _processEventCount = rhs._processEventCount ;
         _isBlock = rhs._isBlock ;
         ossStrcpy( _eduStatus, rhs._eduStatus ) ;
         ossStrcpy( _eduType, rhs._eduType ) ;
         ossStrcpy( _eduName, rhs._eduName ) ;
         ossStrcpy( _doing, rhs._doing ) ;
         ossStrcpy( _source, rhs._source ) ;
         _eduContextList = rhs._eduContextList ;
         _monApplCB      = rhs._monApplCB ;
         _threadHdl      = rhs._threadHdl ;
         _relatedNID     = rhs._relatedNID ;
         _relatedEDUID   = rhs._relatedEDUID ;
         _relatedTID     = rhs._relatedTID ;

         return *this ;
      }
   } ;
   typedef _monEDUFull monEDUFull ;

   /*
      _monContextFull Define
   */
   class _monContextFull : public SDBObject
   {
   public :
      SINT64       _contextID ;
      std::string  _typeDesp ;
      std::string  _info ;
      MsgQueryID   _queryID ;
      monContextCB _monContext ;

      _monContextFull ( SINT64 cid, const monContextCB &monCtxCB )
      {
         _contextID = cid ;
         _monContext = monCtxCB ;
      }

      BOOLEAN operator< (const _monContextFull &r) const
      {
         return _contextID < r._contextID ;
      }

      _monContextFull &operator= ( const _monContextFull &rhs )
      {
         _contextID  = rhs._contextID ;
         _monContext = rhs._monContext ;
         return *this ;
      }
   } ;
   typedef _monContextFull monContextFull ;

}

#endif // MONEDU_HPP_
