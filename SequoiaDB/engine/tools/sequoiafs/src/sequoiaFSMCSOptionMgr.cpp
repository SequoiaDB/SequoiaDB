/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   ( at your option ) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = sequoiaFSMCSOptionMgr.cpp

   Descriptive Name = MCS options manager.

   When/how to use:  This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date      Who Description
   ====== =========== === ==============================================
      01/07/2021  zyj  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sequoiaFSMCSOptionMgr.hpp"
#include "pmdDef.hpp"
#include "ossVer.hpp"
#include "utilStr.hpp"

using namespace engine;
using namespace sequoiafs;
#define FS_COMMANDS_OPTIONS \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_MCS_HELP,           ",h" ), "Print help message" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_MCS_VERSION,        ",v" ), "Print version message" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_MCS_HOSTS,          ",i" ), po::value<std::string>() , "Host addresses( hostname:svcname ), separated by ',', such as 'localhost:11810,localhost:11910', default:'localhost:11810'" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_MCS_USERNAME,       ",u" ), po::value<std::string>() , "User name of source sdb" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_MCS_PASSWD,         ",p" ), po::value<std::string>() , "User password of source sdb" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_MCS_CONF_PATH,      ",c" ), po::value<std::string>() , "Configure file path." ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_MCS_DIAGLEVEL,      ",g" ), po::value<INT32>() , "Diagnostic level, default:3, value range: [0-5]" ) \
     ( SDB_SEQUOIAFS_MCS_DIAGNUM, po::value<INT32>() , "The max number of diagnostic log files, default:20, -1:unlimited" ) \
     ( SDB_SEQUOIAFS_MCS_DIAGPATH, po::value<std::string>(), "Diagnostic log file path" ) \
     ( SDB_SEQUOIAFS_MCS_PORT, po::value<std::string>(), "listen port of MCS, default:11742" ) \
     ( SDB_SEQUOIAFS_MCS_START_FORCE, po::value<std::string>(), "Force MCS to start, regardless whether other MCS are started or not, default: false" ) \

INT32 _sequoiafsMcsOptionMgr::save()
{
   INT32 rc = SDB_OK ;
   std::string line ;

   if( ossStrcmp( _cfgPath, "" ) != 0 )
   {
      rc = pmdCfgRecord::toString(  line, PMD_CFG_MASK_SKIP_UNFIELD  ) ;
      if (  SDB_OK != rc  )
      {
         PD_LOG(  PDERROR, "Failed to get the line str:%d", rc  ) ;
         goto error ;
      }

      rc = utilWriteConfigFile(  _cfgFileName, line.c_str() , FALSE  ) ;
      PD_RC_CHECK(  rc, PDERROR, "Failed to write config[%s], rc= %d",
                  _cfgFileName, rc  ) ;
   }

done:
   return rc ;
error:
   goto done ;
}

_sequoiafsMcsOptionMgr::_sequoiafsMcsOptionMgr()
{
   ossMemset( _hosts, 0, sizeof( _hosts ) );
   ossMemset( _userName, 0, sizeof( _userName ) );
   ossMemset( _passwd, 0, sizeof( _passwd ) );
   ossMemset( _cfgPath, 0, sizeof( _cfgPath ) );
   ossMemset( _cfgFileName, 0, sizeof( _cfgFileName ) );
   ossMemset( _diagPath, 0, sizeof( _diagPath ) );

   _connectionNum = SDB_SEQUOIAFS_MCS_CONNECTION_DEFAULT_MAX_NUM;
   _diagLevel = PDWARNING;
}

PDLEVEL _sequoiafsMcsOptionMgr::getDiaglogLevel()const
{
   PDLEVEL level = PDWARNING;

   if( _diagLevel < PDSEVERE )
   {
     level = PDSEVERE;
   }
   else if( _diagLevel > PDDEBUG )
   {
     level = PDDEBUG;
   }
   else
   {
     level = ( PDLEVEL )_diagLevel;
   }

   return level;
}

INT32 _sequoiafsMcsOptionMgr::init( INT32 argc,
                            CHAR **argv)
{
   INT32 rc = SDB_OK;
   CHAR *cfgPath;
   CHAR tempPath[OSS_MAX_PATHSIZE + 1] = {0};
   const CHAR *cfgTempPath;
   string tempStr;
   BSONObj allCfg ;
   namespace po = boost::program_options;

   ossSnprintf( tempPath, sizeof( tempPath ), "Command options" );
   //1. init options
   po::options_description desc( tempPath );
   po::options_description display ( tempPath ) ;
   po::variables_map vmFromCmd;
   po::variables_map vmFromFile;
   vector<string> fuse_str;
   vector<string> path_str;
   
   PMD_ADD_PARAM_OPTIONS_BEGIN(desc)
      FS_COMMANDS_OPTIONS
      ( "*", po::value<std::string>(), "" )
   PMD_ADD_PARAM_OPTIONS_END

   PMD_ADD_PARAM_OPTIONS_BEGIN(display)
      FS_COMMANDS_OPTIONS
   PMD_ADD_PARAM_OPTIONS_END
 
   //2. init cmd line
   rc = engine::utilReadCommandLine2(argc, argv, desc, vmFromCmd, TRUE);
   if(SDB_OK != rc)
   {
      goto error;
   }

   if(vmFromCmd.count(SDB_SEQUOIAFS_MCS_HELP))
   {
      std::cout << display << std::endl;
      fuse_str.push_back("--helpsfs");
      rc = SDB_PMD_HELP_ONLY;
      goto done;
   }

   if(vmFromCmd.count(SDB_SEQUOIAFS_MCS_VERSION))
   {
      ossPrintVersion("SequoiaFS version");
      fuse_str.push_back("--version");
      rc = SDB_PMD_VERSION_ONLY;
      goto done;
   }

   //3. get conf path
   cfgTempPath = (vmFromCmd.count(SDB_SEQUOIAFS_MCS_CONF_PATH)) ? 
                    (vmFromCmd[SDB_SEQUOIAFS_MCS_CONF_PATH].as<string>().c_str()) : 
                    PMD_CURRENT_PATH;  

   ossMemset(tempPath, 0, sizeof(tempPath));
   cfgPath = ossGetRealPath(cfgTempPath, tempPath, OSS_MAX_PATHSIZE);
   if(!cfgPath)
   {
      if(vmFromCmd.count(SDB_SEQUOIAFS_MCS_CONF_PATH))
         std::cerr << "ERROR: Failed to get real path for "<< cfgTempPath<< endl;
      else
         SDB_ASSERT(FALSE, "Current path is impossible to be NULL");

      rc = SDB_INVALIDPATH;
      goto error;
   }

   rc = engine::utilBuildFullPath(tempPath, SDB_SEQUOIAFS_MCS_CFG_FILE_NAME,
                                  OSS_MAX_PATHSIZE, _cfgFileName);
   if(SDB_OK != rc)
   {
      std::cerr << "ERROR: Make configuration file name failed, rc=" <<
                rc << endl;
      goto error;
   }

   //4. read config file
   rc = ossAccess( _cfgFileName, OSS_MODE_READ );
   if ( SDB_OK == rc )
   {
      rc = engine::utilReadConfigureFile(_cfgFileName, desc, vmFromFile);
      if(SDB_OK != rc)
      {
         std::cerr << "ERROR: Read configuration file[" << _cfgFileName <<
                         "] failed[" << rc << "]" << endl;
         goto error;
      }
   }

   rc = pmdCfgRecord::init(&vmFromFile, &vmFromCmd);
   if(SDB_OK != rc)
   {
      std::cerr << "ERROR: Init configuration record failed[" << rc << "]" << endl;
      goto error;
   }
   
done:
   return rc;

error:
   goto done;
}

INT32 _sequoiafsMcsOptionMgr::doDataExchange(pmdCfgExchange *pEX)
{
   resetResult();
   //--hosts
   rdxString(pEX, SDB_SEQUOIAFS_MCS_HOSTS, _hosts, sizeof(_hosts), FALSE,
             PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_MCS_HOSTS_DEFAULT_VALUE);
   //--username
   rdxString(pEX, SDB_SEQUOIAFS_MCS_USERNAME, _userName, sizeof(_userName),
             FALSE, PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_MCS_USER_DEFAULT_NAME);
   //--passwd
   rdxString(pEX, SDB_SEQUOIAFS_MCS_PASSWD, _passwd, sizeof(_passwd), FALSE,
             PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_MCS_USER_DEFAULT_PASSWD);
   
   //--connectionnum
   rdxInt(pEX, SDB_SEQUOIAFS_MCS_CONNECTION_NUM, _connectionNum, FALSE,
          PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_MCS_CONNECTION_DEFAULT_MAX_NUM);
   rdvMinMax(pEX, _connectionNum, 50, 1000, TRUE);

   //--diaglevel
   rdxUShort(pEX, SDB_SEQUOIAFS_MCS_DIAGLEVEL, _diagLevel, FALSE,
             PMD_CFG_CHANGE_RUN, (UINT16)PDWARNING);
   rdvMinMax(pEX, _diagLevel, PDSEVERE, PDDEBUG, TRUE);

   //--mcsconfpath
   rdxPath(pEX, SDB_SEQUOIAFS_MCS_CONF_PATH, _cfgPath, sizeof(_cfgPath),
           FALSE, PMD_CFG_CHANGE_FORBIDDEN, "");
   //--mcsdiagpath
   rdxPath(pEX, SDB_SEQUOIAFS_MCS_DIAGPATH, _diagPath, sizeof(_diagPath),
           FALSE, PMD_CFG_CHANGE_FORBIDDEN, PMD_CURRENT_PATH);
   //--diagnum
   rdxInt(pEX, SDB_SEQUOIAFS_MCS_DIAGNUM, _diagnum, FALSE, PMD_CFG_CHANGE_FORBIDDEN,
          PD_DFT_FILE_NUM);
   rdvMinMax( pEX, _diagnum, (INT32)PD_MIN_FILE_NUM, (INT32)OSS_SINT32_MAX, TRUE ) ;

   //--port
   rdxString(pEX, SDB_SEQUOIAFS_MCS_PORT, _port, sizeof(_port),
           FALSE, PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_MCS_DEFAULT_PORT);

   //--fuse_allow_other
   rdxBooleanS(pEX, SDB_SEQUOIAFS_MCS_START_FORCE, _forcestart, FALSE, 
          PMD_CFG_CHANGE_FORBIDDEN, FALSE);
          
   return getResult();
}



