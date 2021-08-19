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

   Source File Name = seAdptOptionsMgr.cpp

   Descriptive Name = Search engine adapter options manager

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/08/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "seAdptOptionsMgr.hpp"
#include "seAdptDef.hpp"
#include "ossVer.hpp"

using namespace engine ;

#define COMMANDS_OPTIONS \
   ( PMD_COMMANDS_STRING (PMD_OPTION_HELP, ",h"), "help" ) \
   ( PMD_OPTION_VERSION, "version" ) \
   ( PMD_COMMANDS_STRING (PMD_OPTION_CONFPATH, ",c"), boost::program_options::value<string>(), "Configure file path" ) \
   ( PMD_COMMANDS_STRING (PMD_OPTION_DIAGLEVEL, ",v"), boost::program_options::value<int>(), "Diagnostic level,default:3,value range:[0-5]" ) \
   ( SEADPT_DNODE_HOST, boost::program_options::value<string>(), "Data node address" ) \
   ( SEADPT_DNODE_PORT, boost::program_options::value<string>(), "Data node service name or port" ) \
   ( SEADPT_SE_HOST, boost::program_options::value<string>(), "Search engine address" ) \
   ( SEADPT_SE_PORT, boost::program_options::value<string>(), "Search engine service name or port" ) \
   ( PMD_COMMANDS_STRING (SEADPT_SE_IDXPREFIX, ",p"), boost::program_options::value<string>(), "Prefix of index names on search engine,default:none, valid value length:[1-16]") \
   ( SEADPT_BULK_BUFF_SIZE, boost::program_options::value<int>(), "Bulk operation buffer size,unit:MB,default:10,value range:[1-32]" ) \
   ( PMD_COMMANDS_STRING (PMD_OPTION_OPERATOR_TIMEOUT, ",t"), boost::program_options::value<int>(), "Rest operation timeout in millisecond,default:10000,value range[3000-3600000]" ) \
   ( PMD_COMMANDS_STRING (SEADPT_STR_MAP_TYPE, ",s"), boost::program_options::value<int>(), "String map type,default 1, value range[1-3]" ) \
   ( PMD_COMMANDS_STRING (SEADPT_CONN_LIMIT, ",l"), boost::program_options::value<int>(), "Max connection number between adapter and search engine,default:50, value range[1-65535]") \
   ( PMD_COMMANDS_STRING (SEADPT_CONN_TIMEOUT, ",o"), boost::program_options::value<int>(), "Max idle time of connection between adapter and search engine,unit:second, default:1800, value range[60-86400]") \
   ( SEADPT_SCROLL_SIZE, boost::program_options::value<int>(), "Scroll size when fetch data from search engine,default:1000,value range:[50-10000]")

namespace seadapter
{
   _seAdptOptionsMgr::_seAdptOptionsMgr()
   {
      ossMemset( _cfgFileName, 0, sizeof( _cfgFileName ) ) ;
      ossMemset( _serviceName, 0, sizeof( _serviceName ) ) ;
      ossMemset( _dbHost, 0, sizeof( _dbHost ) ) ;
      ossMemset( _dbService, 0, sizeof( _dbService ) ) ;
      ossMemset( _seHost, 0, sizeof( _seHost ) ) ;
      ossMemset( _seService, 0, sizeof( _seService ) ) ;
      ossMemset( _seIdxPrefix, 0, sizeof( _seIdxPrefix ) ) ;
      _diagLevel = PDWARNING ;
      _timeout = SEADPT_DFT_TIMEOUT ;
      _bulkBuffSize = SEADPT_DFT_BULKBUFF_SZ ;
      _strMapType = SEADPT_DFT_STR_MAP_TYPE ;
      _seConnLimit = SEADPT_DFT_CONN_LIMIT ;
      _seConnTimeout = SEADPT_DFT_CONN_TIMEOUT ;
      _seScrollSize = SEADPT_DFT_SCROLL_SIZE ;
   }

   INT32 _seAdptOptionsMgr::init( INT32 argc, CHAR **argv,
                                  const CHAR *exePath )
   {
      INT32 rc = SDB_OK ;
      CHAR cfgTempPath[ OSS_MAX_PATHSIZE + 1 ] = {0} ;
      po::options_description all( "Command options" ) ;
      po::options_description display( "Command options (display)" ) ;
      po::variables_map vmFromCmd ;
      po::variables_map vmFromFile ;

      PMD_ADD_PARAM_OPTIONS_BEGIN( all )
         COMMANDS_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END

      rc = utilReadCommandLine( argc, argv, all, vmFromCmd ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( vmFromCmd.count( PMD_OPTION_HELP ) )
      {
         std::cout << all << std::endl ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
      if ( vmFromCmd.count( PMD_OPTION_VERSION ) )
      {
         ossPrintVersion( "sdbseadapter version" ) ;
         rc = SDB_PMD_VERSION_ONLY ;
         goto done ;
      }

      // Get the configuration file path. If not privided, search in the current
      // path.
      if ( vmFromCmd.count( PMD_OPTION_CONFPATH ) )
      {
         CHAR *cfgPath =
            ossGetRealPath( vmFromCmd[PMD_OPTION_CONFPATH].as<string>().c_str(),
                            cfgTempPath, OSS_MAX_PATHSIZE ) ;
         if ( !cfgPath )
         {
            std::cerr << "ERROR: Failed to get real path for "
                      << vmFromCmd[PMD_OPTION_CONFPATH].as<string>().c_str()
                      << endl ;
            rc = SDB_INVALIDPATH ;
            goto error ;
         }
      }
      else
      {
         CHAR *cfgPath = ossGetRealPath( PMD_CURRENT_PATH, cfgTempPath,
                                         OSS_MAX_PATHSIZE ) ;
         if ( !cfgPath )
         {
            SDB_ASSERT( FALSE, "Current path is impossible to be NULL" ) ;
            rc = SDB_INVALIDPATH ;
            goto error ;
         }
      }

      rc = utilBuildFullPath( cfgTempPath, SEADPT_CFG_FILE_NAME,
                              OSS_MAX_PATHSIZE, _cfgFileName ) ;
      if ( rc )
      {
         std::cerr << "ERROR: Make configuration file name failed, rc: " << rc
                   << std::endl ;
         goto error ;
      }

      rc = utilReadConfigureFile( _cfgFileName, all, vmFromFile ) ;
      if ( rc )
      {
         if ( vmFromCmd.count( PMD_OPTION_CONFPATH ) )
         {
            // Read the configuration file specified by command failed.
            std::cerr << "ERROR: Read configuration file[ " << _cfgFileName
                      << "] failed[ " << rc << " ]" << std::endl ;
            goto error ;
         }
         else
         {
            // Read the default configuration file failed.
            std::cerr << "ERROR: Read default configuration file[ "
               << _cfgFileName << " ] failed[ " << rc << " ]" << std::endl ;
            goto error ;
         }
      }

      rc = pmdCfgRecord::init( &vmFromFile, &vmFromCmd ) ;
      if ( rc )
      {
         std::cerr << "ERROR: Init configuration record failed[ " << rc
            << " ]" << std::endl ;
         goto error ;
      }

      if ( !_validateIdxPrefix() )
      {
         rc = SDB_INVALIDARG;
         std::cerr << "ERROR: Index prefix[" << _seIdxPrefix << "] is invalid["
                   << rc << "]" << std::endl ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptOptionsMgr::doDataExchange( pmdCfgExchange *pEX )
   {
      resetResult() ;

      rdxString( pEX, SEADPT_DNODE_HOST, _dbHost, sizeof( _dbHost ),
                 TRUE, PMD_CFG_CHANGE_FORBIDDEN , "" ) ;
      rdvNotEmpty( pEX, _dbHost ) ;

      rdxString( pEX, SEADPT_DNODE_PORT, _dbService,
                 sizeof( _dbService ), TRUE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;
      rdvNotEmpty( pEX, _dbService ) ;

      rdxUShort( pEX, SEADPT_DIAGLEVEL, _diagLevel,
                 FALSE, PMD_CFG_CHANGE_RUN, (UINT16)PDWARNING ) ;
      rdvMinMax( pEX, _diagLevel, PDSEVERE, PDDEBUG, TRUE ) ;

      rdxString( pEX, SEADPT_SE_HOST, _seHost, sizeof( _seHost ),
                 TRUE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;
      rdvNotEmpty( pEX, _seHost ) ;

      rdxString( pEX, SEADPT_SE_PORT, _seService, sizeof( _seService ),
                 TRUE, PMD_CFG_CHANGE_FORBIDDEN, SEADPT_SE_DFT_SERVICE ) ;
      rdvNotEmpty( pEX, _seService ) ;

      rdxInt( pEX, PMD_OPTION_OPERATOR_TIMEOUT, _timeout,
              FALSE, PMD_CFG_CHANGE_FORBIDDEN, SEADPT_DFT_TIMEOUT ) ;
      rdvMinMax( pEX, _timeout, 3000, 3600000, TRUE ) ;

      rdxUInt( pEX, SEADPT_BULK_BUFF_SIZE, _bulkBuffSize,
               FALSE, PMD_CFG_CHANGE_RUN, SEADPT_DFT_BULKBUFF_SZ ) ;
      rdvMinMax( pEX, _bulkBuffSize, 1, 32, TRUE ) ;

      rdxString( pEX, SEADPT_SE_IDXPREFIX, _seIdxPrefix, sizeof( _seIdxPrefix ),
                 FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;

      rdxUShort( pEX, SEADPT_STR_MAP_TYPE, _strMapType,
                 FALSE, PMD_CFG_CHANGE_REBOOT, SEADPT_DFT_STR_MAP_TYPE ) ;
      rdvMinMax( pEX, _strMapType, 1, 3, FALSE ) ;

      rdxUInt( pEX, SEADPT_CONN_LIMIT, _seConnLimit, FALSE,
               PMD_CFG_CHANGE_REBOOT, SEADPT_DFT_CONN_LIMIT ) ;
      rdvMinMax( pEX, _seConnLimit, 1, 65535, TRUE ) ;

      rdxUInt( pEX, SEADPT_CONN_TIMEOUT, _seConnTimeout, FALSE,
               PMD_CFG_CHANGE_REBOOT, SEADPT_DFT_CONN_TIMEOUT ) ;
      rdvMinMax( pEX, _seConnTimeout, 60, 86400, TRUE ) ;

      rdxUShort( pEX, SEADPT_SCROLL_SIZE, _seScrollSize, FALSE,
                 PMD_CFG_CHANGE_REBOOT, SEADPT_DFT_SCROLL_SIZE ) ;
      rdvMinMax( pEX, _seScrollSize, 50, 10000, TRUE ) ;

      return getResult() ;
   }

   void _seAdptOptionsMgr::setSvcName( const CHAR *svcName )
   {
      SDB_ASSERT( svcName, "Service name can't be NULL" ) ;
      ossStrncpy( _serviceName, svcName, OSS_MAX_SERVICENAME ) ;
      _serviceName[ OSS_MAX_SERVICENAME ] = '\0' ;
   }

   PDLEVEL _seAdptOptionsMgr::getDiagLevel() const
   {
      PDLEVEL level = PDWARNING ;
      if ( _diagLevel < PDSEVERE )
      {
         level = PDSEVERE ;
      }
      else if ( _diagLevel > PDDEBUG )
      {
         level = PDDEBUG ;
      }
      else
      {
         level = (PDLEVEL)_diagLevel ;
      }

      return level ;
   }

   INT32 _seAdptOptionsMgr::getTimeout() const
   {
      return _timeout ;
   }

   UINT32 _seAdptOptionsMgr::getBulkBuffSize() const
   {
      return _bulkBuffSize ;
   }

   UINT16 _seAdptOptionsMgr::getStrMapType() const
   {
      return _strMapType ;
   }

   UINT32 _seAdptOptionsMgr::getSEConnLimit() const
   {
      return _seConnLimit ;
   }

   UINT32 _seAdptOptionsMgr::getSEConnTimeout() const
   {
      return _seConnTimeout;
   }

   UINT16 _seAdptOptionsMgr::getSEScrollSize() const
   {
      return _seScrollSize ;
   }

   // Index prefix can only contains english characters, numbers, and '_', and
   // can not start with '_'.
   BOOLEAN _seAdptOptionsMgr::_validateIdxPrefix() const
   {
      UINT8 i = 0 ;
      UINT8 len =  ossStrlen( _seIdxPrefix ) ;
      while ( i < len &&
              ( std::isdigit( (UINT8)_seIdxPrefix[i] ) ||
                std::isalpha( (UINT8)_seIdxPrefix[i] ) ||
                '_' == _seIdxPrefix[i] ) )
      {
         ++i ;
      }
      return ( 0 == len ) ||
             ( i == len && '_' != _seIdxPrefix[0] &&
               0 != ossStrcasecmp( _seIdxPrefix, "sys" ) ) ;
   }
}

