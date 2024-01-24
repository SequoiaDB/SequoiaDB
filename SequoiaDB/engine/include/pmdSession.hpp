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

   Source File Name = pmdSession.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/04/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_SESSION_HPP_
#define PMD_SESSION_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossSocket.hpp"
#include "pmdSessionBase.hpp"
#include "pmdIProcessor.hpp"
#include "pmdExternClient.hpp"
#include "schedTaskMgr.hpp"

#include <string>

namespace engine
{

   class _pmdEDUCB ;
   class _dpsLogWrapper ;

   #define PMD_RETBUILDER_DFT_SIZE              ( 96 )
   /*
      _pmdSession define
   */
   class _pmdSession : public _pmdSessionBase
   {
      public:
         _pmdSession( SOCKET fd ) ;
         virtual ~_pmdSession() ;

         virtual UINT64          identifyID() ;
         virtual MsgRouteID      identifyNID() ;
         virtual UINT32          identifyTID() ;
         virtual UINT64          identifyEDUID() ;

         virtual void*           getSchedItemPtr() ;
         virtual void            setSchedItemVer( INT32 ver ) ;

         virtual void            clear() ;

         virtual const CHAR*     sessionName() const ;
         virtual BOOLEAN         isBusinessSession() const { return TRUE ; }
         virtual IClient*        getClient() { return &_client ; }

         virtual _dpsLogWrapper* getDPSCB() { return _pDPSCB ; }
         virtual _pmdEDUCB*      eduCB () const { return _pEDUCB ; }
         virtual EDUID           eduID () const { return _eduID ; }

         virtual INT32           run() = 0 ;

      public:
         UINT64      sessionID () const { return _eduID ; }
         ossSocket*  socket () { return &_socket ; }

         void        attach( _pmdEDUCB * cb ) ;
         void        detach() ;

         CHAR*       getBuff( UINT32 len ) ;
         INT32       getBuffLen () const { return _buffLen ; }

         INT32       allocBuff( UINT32 len, CHAR **ppBuff,
                                UINT32 *pRealSize = NULL ) ;
         void        releaseBuff( CHAR *pBuff ) ;
         INT32       reallocBuff( UINT32 len, CHAR **ppBuff,
                                  UINT32 *pRealSize = NULL ) ;

         void        disconnect() ;
         INT32       sendData( const CHAR *pData, INT32 size,
                               INT32 timeout = -1,
                               BOOLEAN block = TRUE,
                               INT32 *pSentLen = NULL,
                               INT32 flags = 0 ) ;
         INT32       recvData( CHAR *pData, INT32 size,
                               INT32 timeout = -1,
                               BOOLEAN block = TRUE,
                               INT32 *pRecvLen = NULL,
                               INT32 flags = 0 ) ;
         INT32       sniffData( INT32 timeout = OSS_ONE_SEC ) ;

      protected:
         inline BOOLEAN _isAwaitingHandshake () const
         {
            return _awaitingHandshake ;
         }

         inline void   _setHandshakeReceived ()
         {
            _awaitingHandshake = FALSE ;
         }

      protected:

         _pmdEDUCB                        *_pEDUCB ;
         EDUID                            _eduID ;
         ossSocket                        _socket ;
         std::string                      _sessionName ;
         pmdExternClient                  _client ;
         _dpsLogWrapper                   *_pDPSCB ;
         BOOLEAN                          _awaitingHandshake ;

         schedItem                        _infoItem ;

      protected:
         CHAR                             *_pBuff ;
         UINT32                           _buffLen ;

   } ;
   typedef _pmdSession pmdSession ;
}

#endif //PMD_SESSION_HPP_

