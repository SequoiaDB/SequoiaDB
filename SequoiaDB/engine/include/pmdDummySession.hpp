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

   Source File Name = pmdDummySession.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date     Who     Description
   ====== ======== ======= ==============================================
          2020/8/8 Ting YU Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_DUMMY_SESSION_HPP_
#define PMD_DUMMY_SESSION_HPP_

#include "pmdSessionBase.hpp"
#include "pmdInnerClient.hpp"

namespace engine
{
   /*
      _pmdDummySession define
   */
   class _pmdDummySession : public _pmdSessionBase
   {
   public:
      _pmdDummySession( BOOLEAN isBusinessSession = FALSE )
      : _pEDUCB( NULL ),
        _eduID( PMD_INVALID_EDUID ),
        _isBusinessSession( isBusinessSession )
      {
      }

      virtual ~_pmdDummySession() {}

   public:
      virtual UINT64           identifyID()    { return 0 ; }
      virtual MsgRouteID       identifyNID() ;
      virtual UINT32           identifyTID()   { return 0 ; }
      virtual UINT64           identifyEDUID() { return 0 ; }

      virtual const CHAR*      sessionName() const { return "Dummy-Session" ; }
      virtual SDB_SESSION_TYPE sessionType() const { return SDB_SESSION_DUMMY ; }
      virtual BOOLEAN          isBusinessSession() const { return _isBusinessSession ; }
      virtual INT32            getServiceType() const { return 0 ; }
      virtual IClient*         getClient()            { return &_client ; }

      virtual void*            getSchedItemPtr()            { return NULL ; }
      virtual void             setSchedItemVer( INT32 ver ) { return ; }

      virtual _pmdEDUCB*       eduCB () const { return _pEDUCB ; }
      virtual EDUID            eduID () const { return _eduID ; }

      void                     attachCB ( _pmdEDUCB *cb ) ;
      void                     detachCB () ;

   protected:
      _pmdEDUCB               *_pEDUCB ;
      EDUID                    _eduID ;
      pmdInnerClient           _client ;
      BOOLEAN                  _isBusinessSession ;
   } ;

   typedef _pmdDummySession pmdDummySession ;
}

#endif //PMD_DUMMY_SESSION_HPP_

