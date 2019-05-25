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

   Source File Name = sptSshSession.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_SSHSESSION_HPP_
#define SPT_SSHSESSION_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossSocket.hpp"
#include <string>

using namespace std ;

namespace engine
{
   enum SPT_CP_PROTOCOL
   {
      SPT_CP_PROTOCOL_SCP = 0,
      SPT_CP_PROTOCOL_FTP,
      SPT_CP_PROTOCOL_SFTP,
   } ;

   #define SPT_SSH_PORT             ( 22 )

   class _sptSshSession : public SDBObject
   {
   public:
      _sptSshSession( const CHAR *host,
                      const CHAR *usrname,
                      const CHAR *passwd,
                      INT32 *port = NULL ) ;
      virtual ~_sptSshSession() ;

      const CHAR* getPassword() const { return _passwd.c_str() ; }

      string getLocalIPAddr() ;
      string getPeerIPAddr() ;

      void   close() ;
      BOOLEAN isOpened() const { return _isOpen ; }

   public:
      INT32 open() ;

      virtual INT32 exec( const CHAR *cmd, INT32 &exit,
                          std::string &outStr ) = 0 ;



      virtual INT32 copy2Remote( SPT_CP_PROTOCOL protocol,
                                 const CHAR *local,   /// full path
                                 const CHAR *dst,   /// full path
                                 INT32 mode ) = 0 ;

      virtual INT32 copyFromRemote( SPT_CP_PROTOCOL protocol,
                                    const CHAR *remote,
                                    const CHAR *local,
                                    INT32 mode ) = 0 ;

      virtual void getLastError( std::string &errMsg ) = 0 ;

   private:
      virtual INT32 _openSshSession() = 0 ;

   protected:
      std::string _host ;
      INT32       _port ;
      std::string _usr ;
      std::string _passwd ;
      BOOLEAN     _isOpen ;
      _ossSocket *_sock ;

   } ;
   typedef class _sptSshSession sptSshSession ;
}

#endif

