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

   Source File Name = utilSdb.cpp

   Descriptive Name =

   When/how to use: parse Jsons util

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/02/2014  JW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilSdb.hpp"
#include "ossVer.h"
#include <string>
#include <iostream>

#define OPTION_HELP        "help"
#define OPTION_VERSION     "version"

utilSdbTemplet::utilSdbTemplet() : _pData(NULL)
{
}

utilSdbTemplet::~utilSdbTemplet()
{
   std::vector<util_var *>::iterator it ;
   util_var *pVar = NULL ;

   for( it = _argList.begin(); it != _argList.end(); ++it )
   {
      pVar = *it ;
      if ( pVar->varType == UTIL_VAR_STRING && pVar->stringIsMy )
      {
         SAFE_OSS_FREE ( pVar->pVarString ) ;
      }
      SAFE_OSS_DELETE( pVar ) ;
   }
}

#if defined (_LINUX)
void utilTrapHandler ( OSS_HANDPARMS )
{
   CHAR dumpDir[OSS_MAX_PATHSIZE+1] = "./" ;
   if ( signum == OSS_STACK_DUMP_SIGNAL ||
        signum == OSS_STACK_DUMP_SIGNAL_INTERNAL )
   {
      PD_LOG ( PDEVENT,
               "Signal %d is received, "
               "prepare to dump stack for %u:%u", signum,
               ossGetCurrentProcessID(),
               ossGetCurrentThreadID() ) ;
      void *pSyms[1] ;
      CHAR fileName[OSS_MAX_PATHSIZE] = {0} ;
      ossPrimitiveFileOp trapFile ;
      UINT32 strLen = 0 ;
      ossSnprintf ( fileName, OSS_MAX_PATHSIZE, "%d.%d.trap",
                    ossGetCurrentProcessID(),
                    ossGetCurrentThreadID() ) ;
      if ( ossStrlen ( dumpDir ) + ossStrlen ( OSS_PRIMITIVE_FILE_SEP ) +
           ossStrlen ( fileName ) > OSS_MAX_PATHSIZE )
      {
         pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
                 "path + file name is too long" ) ;
         goto error ;
      }


      ossMemset( fileName, 0, sizeof( fileName ) ) ;
      strLen += ossSnprintf( fileName, sizeof( fileName ), "%s%s",
                             dumpDir, OSS_PRIMITIVE_FILE_SEP ) ;
      ossSnprintf( fileName + strLen, sizeof(fileName) - strLen,
                   "%d.%d.trap",
                   ossGetCurrentProcessID(),
                   ossGetCurrentThreadID() ) ;

      backtrace( pSyms, 1 ) ;
      trapFile.Open( fileName ) ;

      if ( trapFile.isValid() )
      {
         trapFile.seekToEnd () ;
         ossDumpStackTrace( OSS_HANDARGS, &trapFile ) ;
      }

      trapFile.Close() ;
   }
   else
   {
      PD_LOG ( PDWARNING, "Unexpected signal is received: %d",
               signum ) ;
   }
done :
   return ;
error :
   goto done ;
}

INT32 utilSdbTemplet::_utilSetupSignalHandler()
{
   INT32 rc = SDB_OK ;
   struct sigaction newact ;
   ossMemset ( &newact, 0, sizeof(newact) ) ;
   sigemptyset ( &newact.sa_mask ) ;
   newact.sa_sigaction = ( OSS_SIGFUNCPTR ) utilTrapHandler ;
   newact.sa_flags |= SA_SIGINFO ;
   newact.sa_flags |= SA_ONSTACK ;
   if ( sigaction ( OSS_STACK_DUMP_SIGNAL, &newact, NULL ) )
   {
      PD_LOG ( PDERROR, "Failed to setup signal handler for dump signal" ) ;
      rc = SDB_SYS ;
      goto error ;
   }
   if ( sigaction ( OSS_STACK_DUMP_SIGNAL_INTERNAL, &newact, NULL ) )
   {
      PD_LOG ( PDERROR, "Failed to setup signal handler for dump signal" ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}
#endif

INT32 utilSdbTemplet::appendArgInt( const CHAR *pKey,
                                    const CHAR *pCmd,
                                    const CHAR *pExplain,
                                    BOOLEAN require,
                                    INT32 minInt,
                                    INT32 maxInt,
                                    INT32 defaultInt )
{
   INT32 rc = SDB_OK ;
   util_var *pVar = SDB_OSS_NEW util_var() ;
   if ( !pVar )
   {
      PD_LOG ( PDERROR, "Failed to allocate memory" ) ;
      rc = SDB_OOM ;
      goto error ;
   }
   pVar->varType  = UTIL_VAR_INT ;
   pVar->pKey     = pKey ;
   pVar->require  = require ;
   pVar->pCmd     = pCmd ;
   pVar->pExplain = pExplain ;
   pVar->varInt   = defaultInt ;
   pVar->minInt   = minInt ;
   pVar->maxInt   = maxInt ;
   _argList.push_back( pVar ) ;
done:
   return rc ;
error:
   goto done ;
}

INT32 utilSdbTemplet::appendArgChar( const CHAR *pKey,
                                     const CHAR *pCmd,
                                     const CHAR *pExplain,
                                     BOOLEAN require,
                                     CHAR defaultChar )
{
   INT32 rc = SDB_OK ;
   util_var *pVar = SDB_OSS_NEW util_var() ;
   if ( !pVar )
   {
      PD_LOG ( PDERROR, "Failed to allocate memory" ) ;
      rc = SDB_OOM ;
      goto error ;
   }
   pVar->varType  = UTIL_VAR_CHAR ;
   pVar->pKey     = pKey ;
   pVar->require  = require ;
   pVar->pCmd     = pCmd ;
   pVar->pExplain = pExplain ;
   pVar->varChar  = defaultChar ;
   _argList.push_back( pVar ) ;
done:
   return rc ;
error:
   goto done ;
}

INT32 utilSdbTemplet::appendArgBool( const CHAR *pKey,
                                     const CHAR *pCmd,
                                     const CHAR *pExplain,
                                     BOOLEAN require,
                                     BOOLEAN defaultBool )
{
   INT32 rc = SDB_OK ;
   util_var *pVar = SDB_OSS_NEW util_var() ;
   if ( !pVar )
   {
      PD_LOG ( PDERROR, "Failed to allocate memory" ) ;
      rc = SDB_OOM ;
      goto error ;
   }
   pVar->varType  = UTIL_VAR_BOOL ;
   pVar->pKey     = pKey ;
   pVar->require  = require ;
   pVar->pCmd     = pCmd ;
   pVar->pExplain = pExplain ;
   pVar->varBool  = defaultBool ;
   _argList.push_back( pVar ) ;
done:
   return rc ;
error:
   goto done ;
}

INT32 utilSdbTemplet::appendArgString( const CHAR *pKey,
                                       const CHAR *pCmd,
                                       const CHAR *pExplain,
                                       BOOLEAN require,
                                       INT32 maxStringSize,
                                       CHAR *pDefaultString )
{
   INT32 rc = SDB_OK ;
   util_var *pVar = SDB_OSS_NEW util_var() ;
   if ( !pVar )
   {
      PD_LOG ( PDERROR, "Failed to allocate memory" ) ;
      rc = SDB_OOM ;
      goto error ;
   }
   pVar->varType       = UTIL_VAR_STRING ;
   pVar->pKey          = pKey ;
   pVar->require       = require ;
   pVar->pCmd          = pCmd ;
   pVar->pExplain      = pExplain ;
   pVar->stringIsMy    = FALSE ;
   pVar->maxStringSize = maxStringSize ;
   pVar->pVarString    = pDefaultString ;
   _argList.push_back( pVar ) ;
done:
   return rc ;
error:
   goto done ;
}

INT32 utilSdbTemplet::appendArgSwitch( const CHAR *pKey,
                                       const CHAR *pCmd,
                                       const CHAR *pExplain,
                                       BOOLEAN require,
                                       const CHAR **ppSwitch,
                                       INT32 switchNum,
                                       INT32 defaultValue )
{
   INT32 rc = SDB_OK ;
   util_var *pVar = SDB_OSS_NEW util_var() ;
   if ( !pVar )
   {
      PD_LOG ( PDERROR, "Failed to allocate memory" ) ;
      rc = SDB_OOM ;
      goto error ;
   }
   pVar->varType       = UTIL_VAR_SWITCH ;
   pVar->pKey          = pKey ;
   pVar->require       = require ;
   pVar->pCmd          = pCmd ;
   pVar->pExplain      = pExplain ;
   pVar->ppSwitch      = ppSwitch ;
   pVar->switchNum     = switchNum ;
   pVar->varInt        = defaultValue ;
   _argList.push_back( pVar ) ;
done:
   return rc ;
error:
   goto done ;
}

void utilSdbTemplet::_initArg ( po::options_description &desc )
{
   std::vector<util_var *>::iterator it ;
   util_var *pVar = NULL ;

   desc.add_options() ( OPTION_HELP ",h", "help" ) ;
   desc.add_options() ( OPTION_VERSION, "version" ) ;
   for( it = _argList.begin(); it != _argList.end(); ++it )
   {
      pVar = *it ;
      desc.add_options() ( pVar->pCmd, boost::program_options::value<std::string>(), pVar->pExplain ) ;
   }
}

void utilSdbTemplet::_displayArg ( po::options_description &desc )
{
   std::cout << desc << std::endl ;
}

void *utilSdbTemplet::_findKey( const CHAR *pKey )
{
   std::vector<util_var *>::iterator it ;
   util_var *pVar = NULL ;
   UINT32 keySize = ossStrlen( pKey ) ;

   for( it = _argList.begin(); it != _argList.end(); ++it )
   {
      pVar = *it ;
      if ( keySize == ossStrlen( pVar->pKey ) &&
           ossStrncmp( pVar->pKey, pKey, keySize ) == 0 )
      {
         return pVar ;
      }
   }
   return NULL ;
}

INT32 utilSdbTemplet::getArgInt( const CHAR *pKey, INT32 *pVarValue )
{
   INT32 rc = SDB_OK ;
   util_var *pVar = NULL ;

   pVar = (util_var *)_findKey( pKey ) ;
   if ( !pVar )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   *pVarValue = pVar->varInt ;
done:
   return rc ;
error:
   goto done ;
}

INT32 utilSdbTemplet::getArgChar( const CHAR *pKey, CHAR *pVarValue )
{
   INT32 rc = SDB_OK ;
   util_var *pVar = NULL ;

   pVar = (util_var *)_findKey( pKey ) ;
   if ( !pVar )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   *pVarValue = pVar->varChar ;
done:
   return rc ;
error:
   goto done ;
}

INT32 utilSdbTemplet::getArgBool( const CHAR *pKey, BOOLEAN *pVarValue )
{
   INT32 rc = SDB_OK ;
   util_var *pVar = NULL ;

   pVar = (util_var *)_findKey( pKey ) ;
   if ( !pVar )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   *pVarValue = pVar->varBool ;
done:
   return rc ;
error:
   goto done ;
}

INT32 utilSdbTemplet::getArgString( const CHAR *pKey, CHAR **ppVarValue )
{
   INT32 rc = SDB_OK ;
   util_var *pVar = NULL ;

   pVar = (util_var *)_findKey( pKey ) ;
   if ( !pVar )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   *ppVarValue = pVar->pVarString ;
done:
   return rc ;
error:
   goto done ;
}

INT32 utilSdbTemplet::getArgSwitch( const CHAR *pKey, INT32 *pVarValue )
{
   INT32 rc = SDB_OK ;
   util_var *pVar = NULL ;

   pVar = (util_var *)_findKey( pKey ) ;
   if ( !pVar )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   *pVarValue = pVar->varInt ;
done:
   return rc ;
error:
   goto done ;
}

INT32 utilSdbTemplet::_resolveArgument ( po::options_description &desc,
                                         INT32 argc,
                                         CHAR **argv,
                                         const CHAR *pPName )
{
   INT32 rc = SDB_OK ;
   INT32 tempStrSize = 0 ;
   BOOLEAN isFind = FALSE ;
   CHAR tempChar = 0 ;
   CHAR c = 0 ;
   std::vector<util_var *>::iterator it ;
   const CHAR *pTempStr = NULL ;
   util_var *pVar = NULL ;
   po::variables_map vm ;

   try
   {
      po::store ( po::parse_command_line ( argc, argv, desc ), vm ) ;
      po::notify ( vm ) ;
   }
   catch ( po::unknown_option &e )
   {
      pdLog ( PDWARNING, __FUNC__, __FILE__, __LINE__,
            ( ( std::string ) "Unknown argument: " +
                e.get_option_name ()).c_str () ) ;
              std::cerr <<  "Unknown argument: "
                        << e.get_option_name () << std::endl ;
              rc = SDB_INVALIDARG ;
      goto error ;
   }
   catch ( po::invalid_option_value &e )
   {
      pdLog ( PDWARNING, __FUNC__, __FILE__, __LINE__,
             ( ( std::string ) "Invalid argument: " +
               e.get_option_name() ).c_str() ) ;
      std::cerr <<  "Invalid argument: "
                << e.get_option_name() << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   catch( po::error &e )
   {
      std::cerr << e.what() << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( vm.count ( OPTION_HELP ) )
   {
      _displayArg ( desc ) ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }

   if ( vm.count ( OPTION_VERSION ) )
   {
      ossPrintVersion( pPName ) ;
      rc = SDB_PMD_VERSION_ONLY ;
      goto done ;
   }

   for( it = _argList.begin(); it != _argList.end(); ++it )
   {
      isFind = FALSE ;
      pVar = *it ;
      if ( vm.count ( pVar->pKey ) )
      {
         if ( pVar->varType == UTIL_VAR_INT )
         {
            pVar->varInt = ossAtoi(
                  vm[ pVar->pKey ].as<std::string>().c_str() ) ;
            if ( pVar->varInt < pVar->minInt )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG ( PDERROR, "%s must be greater than %d",
                        pVar->pKey, pVar->minInt ) ;
               ossPrintf( "%s must be greater than %d" OSS_NEWLINE,
                          pVar->pKey, pVar->minInt ) ;
               goto error ;
            }
            else if ( pVar->varInt > pVar->maxInt )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG ( PDERROR, "%s must be less than %d",
                        pVar->pKey, pVar->maxInt ) ;
               ossPrintf( "%s must be less than %d" OSS_NEWLINE,
                          pVar->pKey, pVar->maxInt ) ;
               goto error ;
            }
         }
         else if ( pVar->varType == UTIL_VAR_CHAR )
         {
            pTempStr = vm[ pVar->pKey ].as<std::string>().c_str() ;
            tempStrSize = ossStrlen ( pTempStr ) ;
            if ( tempStrSize == 1 )
            {
               tempChar = pTempStr[0] ;
            }
            else if ( tempStrSize > 1 &&
                      pTempStr[0] == '0' &&
                      pTempStr[1] == 'x' )
            {
               tempChar = 0 ;
               if ( tempStrSize > 4 )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG ( PDERROR, "%s must be 1 char \
of 16 hex format ( e.g. 0x09 )", pVar->pKey ) ;
                  ossPrintf( "%s must be 1 char \
of 16 hex format ( e.g. 0x09 )" OSS_NEWLINE, pVar->pKey ) ;
                  goto error ;
               }
               for ( INT32 i = 3; i <= tempStrSize; ++i )
               {
                  tempChar *= 16 ;
                  c = pTempStr[i-1] ;
                  if ( c >= '0' && c <= '9' )
                  {
                     tempChar += c - '0' ;
                  }
                  else if ( c >= 'a' && c <= 'f' )
                  {
                     tempChar += c - 'a' + 10 ;
                  }
                  else if ( c >= 'A' && c <= 'F' )
                  {
                     tempChar += c - 'A' + 10 ;
                  }
               }
            }
            else
            {
               rc = SDB_INVALIDARG ;
               PD_LOG ( PDERROR, "%s must be 1 char \
of 16 hex format ( e.g. 0x09 )", pVar->pKey ) ;
               ossPrintf( "%s must be 1 char \
of 16 hex format ( e.g. 0x09 )" OSS_NEWLINE, pVar->pKey ) ;
               goto error ;
            }
            pVar->varChar = tempChar ;
         }
         else if ( pVar->varType == UTIL_VAR_BOOL )
         {
            ossStrToBoolean ( vm[ pVar->pKey ].as<std::string>().c_str(),
                              &(pVar->varBool) ) ;
         }
         else if ( pVar->varType == UTIL_VAR_STRING )
         {
            pTempStr = vm[ pVar->pKey ].as<std::string>().c_str() ;
            tempStrSize = ossStrlen ( pTempStr ) ;
            if ( pVar->maxStringSize < 0 ||
                 ( tempStrSize <= pVar->maxStringSize && 0 < tempStrSize ) )
            {
               pVar->pVarString = (CHAR *)SDB_OSS_MALLOC( tempStrSize + 1 ) ;
               if ( !pVar->pVarString )
               {
                  PD_LOG ( PDERROR, "Failed to allocate memory for %d bytes",
                           tempStrSize ) ;
                  rc = SDB_OOM ;
                  goto error ;
               }
               pVar->pVarString[ tempStrSize ] = '\0' ;
               pVar->stringIsMy = TRUE ;
               ossStrncpy ( pVar->pVarString, pTempStr, tempStrSize ) ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
               ossPrintf( "%s max size is %d" OSS_NEWLINE,
                          pVar->pKey, pVar->maxStringSize );
               PD_LOG ( PDERROR, "%s max size is %d",
                        pVar->pKey, pVar->maxStringSize ) ;
               goto error ;
            }
         }
         else if ( pVar->varType == UTIL_VAR_SWITCH )
         {
            pTempStr = vm[ pVar->pKey ].as<std::string>().c_str() ;
            tempStrSize = ossStrlen( pTempStr ) ;
            isFind = FALSE ;
            for ( INT32 i = 0; i < pVar->switchNum; ++i )
            {
               if (( tempStrSize == (INT32)ossStrlen( pVar->ppSwitch[ i ] ) ) &&
                    ossStrncasecmp ( pTempStr, pVar->ppSwitch[ i ],
                                     tempStrSize ) == 0 )
               {
                  pVar->varInt = i ;
                  isFind = TRUE ;
                  break ;
               }
            }
            if ( !isFind )
            {
               rc = SDB_INVALIDARG ;
               ossPrintf( "%s unknow %s cmd" OSS_NEWLINE, pVar->pKey, pTempStr );
               PD_LOG ( PDERROR, "%s unknow %s cmd", pVar->pKey, pTempStr ) ;
               goto error ;
            }
         }
      }
      else if ( pVar->require )
      {
         rc = SDB_INVALIDARG ;
         ossPrintf ( "%s must input, rc=%d" OSS_NEWLINE, pVar->pKey, rc ) ;
         PD_LOG ( PDERROR, "%s must input, rc=%d", pVar->pKey, rc ) ;
         goto error ;
      }
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 utilSdbTemplet::init( util_sdb_settings &setting, void *pData )
{
   _setting = setting ;
   _pData = pData ;
   return SDB_OK ;
}

INT32 utilSdbTemplet::run( INT32 argc, CHAR **argv, const CHAR *pPName )
{
   INT32 rc = SDB_OK ;
   po::options_description desc ( "Command options" ) ;
   _initArg ( desc ) ;

   rc = _resolveArgument ( desc, argc, argv, pPName ) ;
   if ( rc )
   {
      if ( SDB_PMD_HELP_ONLY != rc && SDB_PMD_VERSION_ONLY != rc )
      {
         PD_LOG ( PDERROR, "Invalid argument" ) ;
         _displayArg ( desc ) ;
      }
      goto done ;
   }

   rc = _setting.on_init( _pData ) ;
   if ( rc )
   {
      goto error ;
   }

#if defined (_LINUX)
   // signal handler
   rc = _utilSetupSignalHandler () ;
   if ( rc )
   {
      goto error ;
   }
#endif

   rc = _setting.on_preparation( _pData ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _setting.on_main( _pData ) ;
   if ( rc )
   {
      goto error ;
   }
done:
   _setting.on_end( _pData ) ;
   return rc ;
error:
   goto done ;
}
