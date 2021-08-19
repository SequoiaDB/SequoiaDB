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

   Source File Name = sptLibssh2Session.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_LIBSSH2SESSION_HPP_
#define SPT_LIBSSH2SESSION_HPP_

#include "sptSshSession.hpp"
#include "libssh2.h"

class _OSS_FILE ;
namespace engine
{
   class _sptLibssh2Session : public _sptSshSession
   {
   public:
      _sptLibssh2Session( const CHAR *host,
                          const CHAR *usrname,
                          const CHAR *passwd,
                          INT32 *port = NULL ) ;
      virtual ~_sptLibssh2Session() ;

   public:
      virtual INT32 exec( const CHAR *cmd, INT32 &exit,
                          std::string &outStr ) ;

//      virtual INT32 read( CHAR *buf, UINT32 len, UINT32 &readSize ) ;

//      virtual INT32 done( INT32 &eixtcode, std::string &exitsignal ) ;

      virtual INT32 copy2Remote( SPT_CP_PROTOCOL protocol,
                                 const CHAR *local,
                                 const CHAR *dst,
                                 INT32 mode ) ;

      virtual INT32 copyFromRemote( SPT_CP_PROTOCOL protocol,
                                    const CHAR *remote,
                                    const CHAR *local,
                                    INT32 mode) ;

      virtual void getLastError( std::string &errMsg ) ;

   private:
      virtual INT32 _openSshSession() ;

      INT32 _read( std::string &outStr, INT32 streamId = 0 ) ;

      INT32 _done( INT32 &eixtcode, std::string &exitsignal ) ;

      void _clearChannel() ;

      void _clearSession() ;

      INT32 _scpSend( const CHAR *local, const CHAR *dst, INT32 mode ) ;

      INT32 _scpRecv( const CHAR *remote, const CHAR *local, INT32 mode ) ;

      INT32 _write2Channel( LIBSSH2_CHANNEL *channel,
                            const CHAR *buf, SINT64 len ) ;

      INT32 _readFromChannel( LIBSSH2_CHANNEL *channel,
                              SINT64 len,
                              CHAR *buf ) ;

      INT32 _wirte2File( _OSS_FILE *file, const CHAR *buf, SINT64 len ) ;

      void _getLastError( std::string &errMsg ) ;

      INT32 _getLastErrorRC() ;

   private:
      LIBSSH2_SESSION *_session ;
      LIBSSH2_CHANNEL *_channel ;
      std::string _errmsg ;
   } ;
   typedef class _sptLibssh2Session sptLibssh2Session ;
}

#endif

