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

   Source File Name = sptLibssh2Session.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptLibssh2Session.hpp"
#include "pd.hpp"
#include "ossIO.hpp"
#include "ossLatch.hpp"
#include <openssl/crypto.h>
#include "sptUsrSystemCommon.hpp"
#include "utilStr.hpp"
using namespace std ;

#define SPT_COPY_BUF_SIZE  8192

namespace engine
{

   #define SPT_AUTH_PASSWD       0x0001
   #define SPT_AUTH_KEYBOARD     0x0002
   #define SPT_AUTH_PUBKEY       0x0004

   #define SPT_PUBLICKEY_FILE    "./.ssh/id_rsa.pub"
   #define SPT_PRIVATEKEY_FILE   "./.ssh/id_rsa"

   #define MAX_OUT_STRING_LEN    ( 1024 * 1024 )
   #define READ_OUT_STR_LINE_LEN ( 1024 )

   static ossSpinSLatch *locks = NULL ;

   struct SshMapRCItem
   {
      INT32 key ;
      INT32 value ;
      const BOOLEAN end ;
   } ;

   #define MAP_SSH_RC_ITEM( key, value ) { key, value, FALSE }

   static INT32 _sshRCToSdbRC( INT32 retCode )
   {
      INT32 rc = SDB_SYS ;
      static SshMapRCItem sshRCMap[] =
      {
         // map begin
         MAP_SSH_RC_ITEM( LIBSSH2_ERROR_NONE, SDB_OK ),
         MAP_SSH_RC_ITEM( LIBSSH2_ERROR_ALLOC, SDB_OOM ),
         MAP_SSH_RC_ITEM( LIBSSH2_ERROR_AUTHENTICATION_FAILED, SDB_INVALIDARG ),
         MAP_SSH_RC_ITEM( LIBSSH2_ERROR_INVALID_POLL_TYPE, SDB_INVALIDARG ),
         MAP_SSH_RC_ITEM( LIBSSH2_ERROR_INVAL, SDB_INVALIDARG ),
         MAP_SSH_RC_ITEM( LIBSSH2_ERROR_BUFFER_TOO_SMALL, SDB_INVALIDSIZE ),
         MAP_SSH_RC_ITEM( LIBSSH2_ERROR_METHOD_NOT_SUPPORTED, SDB_INVALIDARG ),
         MAP_SSH_RC_ITEM( LIBSSH2_ERROR_EAGAIN, SDB_INTERRUPT ),
         MAP_SSH_RC_ITEM( LIBSSH2_ERROR_TIMEOUT, SDB_TIMEOUT ),
         MAP_SSH_RC_ITEM( LIBSSH2_ERROR_SOCKET_TIMEOUT, SDB_TIMEOUT ),
         MAP_SSH_RC_ITEM( LIBSSH2_ERROR_SOCKET_DISCONNECT, SDB_NETWORK_CLOSE ),
         MAP_SSH_RC_ITEM( LIBSSH2_ERROR_OUT_OF_BOUNDARY, SDB_INVALIDSIZE ),
         MAP_SSH_RC_ITEM( LIBSSH2_ERROR_CHANNEL_PACKET_EXCEEDED, SDB_INVALIDSIZE ),
         MAP_SSH_RC_ITEM( LIBSSH2_ERROR_SOCKET_SEND, SDB_NET_SEND_ERR ),
         MAP_SSH_RC_ITEM( LIBSSH2_ERROR_BAD_SOCKET, SDB_NET_INVALID_HANDLE ),
         // map end
         { 0, 0, TRUE }
      } ;

      SshMapRCItem *item = &sshRCMap[0] ;
      while( item->end != TRUE )
      {
         if( item->key == retCode )
         {
            rc = item->value ;
            break ;
         }
         item++ ;
      }
      return rc ;
   }

   // callback function 1
   void lock_callback( INT32 mode, INT32 type, CHAR *file, INT32 line )
   {
      if ( mode & CRYPTO_LOCK )
         locks[type].get() ;
      else
         locks[type].release() ;
   }

   // callback function 2
   UINT64 thread_id( void )
   {
      return (UINT64)ossGetCurrentThreadID() ;
   }

   // set 2 callback functions
   void ssh2_user_init( void )
   {
      if ( NULL == locks )
      {
         /// _locks is delete[] in ssh2_user_cleanup
         locks = SDB_OSS_NEW ossSpinSLatch[CRYPTO_num_locks()] ;
         if ( NULL == locks )
         {
            PD_LOG ( PDERROR, "Failed to new[] memory, rc = %d", SDB_OOM ) ;
            ossPanic() ;
         }
         // TODO: have not macro for "unsigned long"
         CRYPTO_set_id_callback( (unsigned long(*)())thread_id) ;
         CRYPTO_set_locking_callback((void(*)(INT32, INT32, const CHAR*, INT32))lock_callback) ;
      }
   }

   void ssh2_user_cleanup( void )
   {
      /// when nobody use _locks, delete[] it
      if ( NULL != locks )
      {
         SDB_OSS_DEL[] locks ;
         locks = NULL ;
      }
   }

   /*
      _sptLibSshAssit implement
   */
   class _sptLibSshAssit
   {
      public:
         _sptLibSshAssit() ;
         ~_sptLibSshAssit() ;

   } ;

   _sptLibSshAssit::_sptLibSshAssit()
   {
      libssh2_init( 0 ) ;
      ssh2_user_init() ;
   }

   _sptLibSshAssit::~_sptLibSshAssit()
   {
      libssh2_exit() ;
      ssh2_user_cleanup() ;
   }

   _sptLibSshAssit s_libSshAssit ;

   /*
      Callback function
   */
   static void kbd_callback( const CHAR *name, INT32 name_len,
                             const CHAR *instruction, INT32 instruction_len,
                             INT32 num_prompts,
                             const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
                             LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
                             void **abstract )
   {
      // disable warning
      (void)name ;
      (void)name_len ;
      (void)instruction ;
      (void)instruction_len ;
      (void)prompts ;

      if ( !*abstract )
      {
         return ;
      }
      sptLibssh2Session *pSession = ( sptLibssh2Session* )( *abstract ) ;

      if ( 1 == num_prompts )
      {
#if defined( _WINDOWS )
         responses[ 0 ].text = _strdup( pSession->getPassword() ) ;
#else
         responses[ 0 ].text = strdup( pSession->getPassword() ) ;
#endif // WINDOWS
         responses[ 0 ].length = ossStrlen( pSession->getPassword() ) ;
      }
   }

   /*
      _sptLibssh2Session implement
   */
   _sptLibssh2Session::_sptLibssh2Session( const CHAR *host,
                                           const CHAR *usrname,
                                           const CHAR *passwd,
                                           INT32 *port )
   :_sptSshSession( host, usrname, passwd, port ),
    _session( NULL ),
    _channel( NULL )
   {
   }

   _sptLibssh2Session::~_sptLibssh2Session()
   {
      _clearChannel() ;
      _clearSession() ;
   }

   INT32 _sptLibssh2Session::_openSshSession()
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != _sock, "can not be null" ) ;
      CHAR *authList = NULL ;
      INT32 authType = 0 ;

      _session = libssh2_session_init_ex( NULL, NULL, NULL, (void*)this ) ;
      if ( NULL == _session )
      {
         PD_LOG( PDERROR, "failed to init libssh2 session" ) ;
         _errmsg.assign("failed to init libssh2 session" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = libssh2_session_handshake( _session, _sock->native() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to do handshake with remote:%d", rc ) ;
         rc = SDB_SYS ;
         _getLastError( _errmsg ) ;
         goto error ;
      }

      // check auth type
      authList = libssh2_userauth_list( _session, _usr.c_str(),
                                        ossStrlen( _usr.c_str() ) ) ;

      if ( ossStrstr( authList, "password" ) != NULL )
      {
         authType |= SPT_AUTH_PASSWD ;
      }
      if ( ossStrstr( authList, "keyboard-interactive" ) != NULL )
      {
         authType |= SPT_AUTH_KEYBOARD ;
      }
      if ( ossStrstr( authList, "publickey" ) != NULL )
      {
         authType |= SPT_AUTH_PUBKEY ;
      }

      if ( _passwd.size() > 0 && authType & SPT_AUTH_PASSWD )
      {
         rc = libssh2_userauth_password( _session, _usr.c_str(),
                                         _passwd.c_str() ) ;
      }
      else if ( _passwd.size() > 0 && authType & SPT_AUTH_KEYBOARD )
      {
         rc = libssh2_userauth_keyboard_interactive( _session, _usr.c_str(),
                                                     &kbd_callback ) ;
      }
      else
      {
         string homePath ;
         string err ;
         CHAR publicKeyFile[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR privateKeyFile[ OSS_MAX_PATHSIZE + 1 ] ;
         rc = _sptUsrSystemCommon::getHomePath( homePath, err ) ;
         if( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "%s", err.c_str() ) ;
            goto error ;
         }

         // build public key file path
         rc = utilBuildFullPath( homePath.c_str(), SPT_PUBLICKEY_FILE,
                                 OSS_MAX_PATHSIZE, publicKeyFile ) ;
         if( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to build public key file path, rc: %d",
                    rc ) ;
            goto error ;
         }

         // build private key file path
         rc = utilBuildFullPath( homePath.c_str(), SPT_PRIVATEKEY_FILE,
                                 OSS_MAX_PATHSIZE, privateKeyFile ) ;
         if( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to build private key file path, rc: %d",
                    rc ) ;
            goto error ;
         }

         rc = libssh2_userauth_publickey_fromfile( _session, _usr.c_str(),
                                                   publicKeyFile,
                                                   privateKeyFile,
                                                   "" ) ;
      }
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to do user auth: %d", rc ) ;
         rc = SDB_INVALIDARG ;
         _getLastError( _errmsg ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      _clearSession() ;
      goto done ;
   }

   INT32 _sptLibssh2Session::exec( const CHAR *cmd,
                                   INT32 &exit,
                                   std::string &outStr )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != cmd, "can not be null" ) ;
      SDB_ASSERT( NULL != _session, "call open first" ) ;
      SDB_ASSERT( NULL == _channel, "do not share a session in multi threads" ) ;

      string sig ;

      _channel = libssh2_channel_open_session( _session ) ;
      if ( NULL == _channel )
      {
         PD_LOG( PDERROR, "failed to open channel in sesison" ) ;
         _getLastError( _errmsg ) ;
         rc = _getLastErrorRC() ;
         goto error ;
      }

      rc = libssh2_channel_exec( _channel, cmd ) ;
      if ( SDB_OK != rc )
      {
         _getLastError( _errmsg ) ;
         PD_LOG( PDERROR, "failed to exec cmd on remote node:%d", rc ) ;
         rc = _getLastErrorRC() ;
         goto error ;
      }

      rc = _read( outStr, 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read output:%d", rc ) ;
         goto error ;
      }

      rc = _done( exit, sig ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "exit of cmd is:%d", exit ) ;
         goto error ;
      }

      if ( SDB_OK != exit )
      {
         PD_LOG( PDERROR, "exit number is:%d", exit ) ;
         _read( outStr, SSH_EXTENDED_DATA_STDERR ) ;
      }

   done:
      _clearChannel() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptLibssh2Session::_read( std::string &outStr,
                                    INT32 streamId )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != _channel, "can not be null" ) ;

      CHAR szReadBuf[ READ_OUT_STR_LINE_LEN + 1 ] = { 0 } ;
      BOOLEAN addSoOn = FALSE ;

      UINT32 readSize = outStr.size() ;

      if ( libssh2_channel_eof( _channel ) )
      {
         _getLastError( _errmsg ) ;
         PD_LOG( PDDEBUG, "get eof from the channel" ) ;
         goto done ;
      }

      while ( TRUE )
      {
         rc = libssh2_channel_read_ex( _channel, streamId,
                                       szReadBuf, READ_OUT_STR_LINE_LEN ) ;
         if ( 0 == rc )
         {
            PD_LOG( PDDEBUG, "read 0 bytes from channel" ) ;
            goto done ;
         }
         else if ( readSize >= MAX_OUT_STRING_LEN )
         {
            if ( !addSoOn )
            {
               outStr += "..." ;
               addSoOn = TRUE ;
            }
            continue ;
         }
         else if ( 0 < rc )
         {
            readSize += rc ;
            outStr += szReadBuf ;
            ossMemset( szReadBuf, 0, sizeof( szReadBuf ) ) ;
            rc = SDB_OK ;
         }
         else
         {
            PD_LOG( PDERROR, "failed to read from channel:%d", rc ) ;
            rc = _getLastErrorRC() ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptLibssh2Session::copy2Remote( SPT_CP_PROTOCOL protocol,
                                          const CHAR *local,
                                          const CHAR *dst,
                                          INT32 mode )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != local && NULL != dst, "can not be null" ) ;

      if ( SPT_CP_PROTOCOL_SCP == protocol )
      {
         rc = _scpSend( local, dst, mode ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to scp file:%s, rc:%d", local, rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "this protocao is not supported yet." ) ;
         goto error ;
      }

   done:
      _clearChannel() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptLibssh2Session::copyFromRemote( SPT_CP_PROTOCOL protocol,
                                             const CHAR *remote,
                                             const CHAR *local,
                                             INT32 mode )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != local && NULL != remote, "can not be null" ) ;

      if ( SPT_CP_PROTOCOL_SCP == protocol )
      {
         rc = _scpRecv( remote, local, mode ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to scp_recv file:%s, rc:%d", remote, rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "this protocao is not supported yet." ) ;
         goto error ;
      }
   done:
      _clearChannel() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptLibssh2Session::_done( INT32 &eixtcode, string &exitsignal )
   {
      INT32 rc = SDB_OK ;
      CHAR *sig = NULL ;

      if ( NULL != _channel )
      {
         rc = libssh2_channel_close( _channel ) ;
         if ( SDB_OK != rc )
         {
            _getLastError( _errmsg ) ;
            rc = _getLastErrorRC() ;
            PD_LOG( PDERROR, "failed to close channel:%d", rc ) ;
            goto error ;
         }

         eixtcode = libssh2_channel_get_exit_status( _channel ) ;

         /// we don't own the signal's mem.
         libssh2_channel_get_exit_signal(_channel, &sig,
                                         NULL, NULL, NULL, NULL, NULL);
         if ( NULL != sig )
         {
            exitsignal.assign( sig ) ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void _sptLibssh2Session::getLastError( std::string &errMsg )
   {
      errMsg = _errmsg ;
      _errmsg.clear() ;
      return ;
   }

   void _sptLibssh2Session::_getLastError( std::string &errMsg )
   {
      CHAR *msg = NULL ;
      INT32 errLen = 0 ;

      /// we do not want to own the mem. set want_buf = 0.
      if ( NULL != _session )
      {
         libssh2_session_last_error( _session, &msg, &errLen, 0 ) ;
         if ( NULL != msg )
         {
            errMsg.assign( msg ) ;
         }
      }
      return ;
   }

   INT32 _sptLibssh2Session::_write2Channel( LIBSSH2_CHANNEL *channel,
                                        const CHAR *buf, SINT64 len )
   {
      INT32 rc = SDB_OK ;
      SINT64 need = len ;
      SINT64 write = 0 ;

      while ( 0 < need )
      {
         const CHAR *writeBuf = buf + write ;
         SINT64 once = libssh2_channel_write( channel, writeBuf, need ) ;
         if ( 0 <= once )
         {
            need -= once ;
            write += once ;
         }
         else
         {
            _getLastError( _errmsg ) ;
            PD_LOG( PDERROR, "failed to send data:%lld", once ) ;
            rc = _getLastErrorRC() ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptLibssh2Session::_readFromChannel( LIBSSH2_CHANNEL *channel,
                                               SINT64 len,
                                               CHAR *buf )
   {
      INT32 rc = SDB_OK ;
      SINT64 totalLen = len ;
      SINT64 read = 0 ;
      SDB_ASSERT( 0 < len, "impossible" ) ;

      while ( 0 < totalLen )
      {
         CHAR *readBuf = buf + read ;
         SINT64 once = libssh2_channel_read( channel, readBuf, totalLen ) ;
         if ( 0 == once )
         {
            continue ;
         }
         else if ( 0 < once )
         {
            totalLen -= once ;
            read += once ;
         }
         else
         {
            _getLastError( _errmsg ) ;
            PD_LOG( PDERROR, "failed to recv data:%lld", once ) ;
		    rc = _getLastErrorRC() ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptLibssh2Session::_scpSend( const CHAR *local, const CHAR *dst, INT32 mode )
   {
      INT32 rc = SDB_OK ;
      OSSFILE file ;
      INT64 fileSize = 0 ;

      rc = ossOpen( local, OSS_READONLY | OSS_SHAREREAD, OSS_RU, file ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open file: %s, rc: %d", local, rc ) ;
         goto error ;
      }

      rc = ossGetFileSize( &file, &fileSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get filesize of file:%s, rc:%d",
                 local, rc ) ;
         goto error ;
      }

      _channel = libssh2_scp_send64( _session, dst, mode, fileSize, 0, 0 ) ;
      if ( NULL == _channel )
      {
         _getLastError( _errmsg ) ;
         PD_LOG( PDERROR, "failed to create scp channel" ) ;
         rc = _getLastErrorRC() ;
         goto error ;
      }

      {
      CHAR buf[SPT_COPY_BUF_SIZE] ;
      SINT64 readLen = 0 ;
      SINT64 sendLen = 0 ;
      do
      {
         readLen = 0 ;
         rc = ossRead( &file, buf,
                       SPT_COPY_BUF_SIZE, &readLen ) ;
         if ( SDB_OK == rc )
         {
            rc = _write2Channel( _channel, buf, readLen ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to write to channel:%d", rc ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            sendLen += readLen ;
         }
         else if ( SDB_EOF == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to read from file:%d", rc ) ;
            goto error ;
         }
      } while ( TRUE ) ;

      PD_LOG( PDDEBUG, "%lld bytes were written this time", sendLen ) ;

      libssh2_channel_send_eof( _channel ) ;

      }
   done:
      if ( file.isOpened() )
      {
         INT32 rc = ossClose( file ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to close file:%d", rc ) ;
         }
      }

      return rc ;
   error:
      goto done ;
   }

   INT32 _sptLibssh2Session::_scpRecv( const CHAR *remote,
                                       const CHAR *local,
                                       INT32 mode )
   {
      INT32 rc = SDB_OK ;
      struct stat sb ;
      SINT64 fileSize = 0 ;
      INT32 fMode = OSS_WRITEONLY | OSS_REPLACE ;

      _OSS_FILE file ;
      _channel = libssh2_scp_recv( _session, remote, &sb ) ;
      if ( NULL == _channel )
      {
         _getLastError( _errmsg ) ;
         PD_LOG( PDERROR, "failed to crate scp recv channel" ) ;
         rc = _getLastErrorRC() ;
         goto error ;
      }

      fileSize = sb.st_size ;

      rc = ossOpen( local,
                    fMode,
                    mode, file ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open file:%s, rc:%d", local, rc ) ;
         goto error ;
      }

      {
      CHAR buf[SPT_COPY_BUF_SIZE] ;
      while ( 0 < fileSize )
      {
         SINT64 need = SPT_COPY_BUF_SIZE < fileSize ?
                       SPT_COPY_BUF_SIZE : fileSize ;

         rc = _readFromChannel( _channel, need, buf ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to recv data from channel:%d", rc ) ;
            goto error ;
         }

         rc = _wirte2File( &file, (const CHAR *)buf, need ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         fileSize -= need ;
      }
      }

      rc = ossClose( file ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to close file:%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      if ( file.isOpened() )
      {
         INT32 rc = ossClose( file ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to close file:%d", rc ) ;
         }
         rc = ossDelete( local ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to remove file:%d", rc ) ;
         }
      }
      goto done ;
   }

   void _sptLibssh2Session::_clearChannel()
   {
      if ( NULL != _channel )
      {
         libssh2_channel_close( _channel ) ;
         libssh2_channel_free( _channel ) ;
         _channel = NULL ;
      }
      return ;
   }

   void _sptLibssh2Session::_clearSession()
   {
      if ( NULL != _session )
      {
         libssh2_session_disconnect( _session, "" ) ;
         libssh2_session_free( _session ) ;
         _session = NULL ;
      }

      //libssh2_exit() ;

      return ;
   }

   INT32 _sptLibssh2Session::_wirte2File( OSSFILE *file,
                                          const CHAR *buf,
                                          SINT64 len )
   {
      INT32 rc = SDB_OK ;
      SINT64 total = len ;
      while ( 0 < total )
      {
         SINT64 written = 0 ;
         rc = ossWrite( file, buf, total, &written ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to write to file:%d", rc ) ;
            goto error ;
         }

         total -= written ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptLibssh2Session::_getLastErrorRC()
   {
      INT32 rc = SDB_OK ;
      INT32 err = SDB_OK ;
      INT32 sshErrno = libssh2_session_last_errno( _session ) ;
      if( ( LIBSSH2_ERROR_SOCKET_SEND == sshErrno ||
            LIBSSH2_ERROR_SOCKET_RECV == sshErrno )
            && ( ( err = SOCKET_GETLASTERROR ) != SDB_OK ) )
      {
#if defined (_WINDOWS)
         if( WSAETIMEDOUT == err )
#else
         if( ETIMEDOUT == err )
#endif
         {
            rc = SDB_TIMEOUT ;
         }
         else
         {
            rc = SDB_NETWORK ;
         }
      }
      else
      {
         rc = _sshRCToSdbRC( sshErrno ) ;
      }
      return rc ;
   }
}
