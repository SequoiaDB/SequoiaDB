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

   Source File Name = sequoiaFSOptionMgr.cpp

   Descriptive Name = sequoiafs options manager.

   When/how to use:  This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date      Who Description
   ====== =========== === ==============================================
      03/05/2018  YWX  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sequoiaFSOptionMgr.hpp"
#include "pmdDef.hpp"
#include "ossVer.hpp"
#include "utilStr.hpp"

using namespace engine;
using namespace sequoiafs;

#define FS_COMMANDS_OPTIONS \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_HELP,           ",h" ), "Print help message" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_HELP_FUSE,      ",h" ), "Print fuse help message" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_VERSION,        ",v" ), "Print version message" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_MOUNTPOINT,     ",m" ), po::value<std::string>() , "Full path of mountpoint" )  \
     ( SDB_SEQUOIAFS_ALIAS, po::value<std::string>() , "Alias name of mountpoint, the value is generally the last level of full path. " ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_HOSTS,          ",i" ), po::value<std::string>() , "Host addresses( hostname:svcname ), separated by ',', such as 'localhost:11810,localhost:11910', default:'localhost:11810'" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_USERNAME,       ",u" ), po::value<std::string>() , "User name of source sdb" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_PASSWD,         ",p" ), po::value<std::string>() , "User password of source sdb" ) \
     ( SDB_SEQUOIAFS_CIPHERFILE, po::value<std::string>() , "Cipher file of source sdb" ) \
     ( SDB_SEQUOIAFS_TOKEN, po::value<std::string>() , "Encryption token for cipher file" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_COLLECTION,     ",l" ), po::value<std::string>() , "The target collection that be mounted" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_META_DIR_CL,    ",d" ), po::value<std::string>() , "The dir meta collection" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_META_FILE_CL,   ",f" ), po::value<std::string>() , "The file meta collection" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_CONNECTION_NUM, ",n" ), po::value<INT32>() , "The max connection num of the connection pool, default:100, value range: [50-1000]" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_DIRCACHE_SIZE,  ",s" ), po::value<INT32>() , "The cache number of dir meta, default:100000 , value range: [20-1000000]" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_DATACACHE_SIZE, ",s" ), po::value<INT32>() , "The cache size of data cache, default:2048 (MB), value range: [200-20480]" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_CONF_PATH,      ",c" ), po::value<std::string>() , "Configure file path." ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_DIAGLEVEL,      ",g" ), po::value<INT32>() , "Diagnostic level, default:3, value range: [0-5]" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_REPLSIZE,       ",r" ), po::value<INT32>() , "Replsize of meta collections, default:2, value range: [-1-7]" ) \
     ( SDB_SEQUOIAFS_DIAGNUM, po::value<INT32>() , "The max number of diagnostic log files, default:20, -1:unlimited" ) \
     ( SDB_SEQUOIAFS_DIAGPATH, po::value<std::string>(), "Diagnostic log file path" ) \
     ( SDB_SEQUOIAFS_FLUSH_FLAG, po::value<INT32>() , "The flush flag, value range:[0:sync, 1:async, 2:direct], default:0:sync" ) \
     ( SDB_SEQUOIAFS_FORCE_MOUNT, po::value<std::string>() , "The froce mount flag, default: different mountpath can not mount to the same collection. value range:[false, true], default:false" ) \
     ( SDB_SEQUOIAFS_PRE_READ, po::value<INT32>() , "The preread block number, default: 4. value range:[1, 20]" ) \
     ( SDB_SEQUOIAFS_CREATECACHE, po::value<std::string>(), "Create file cache, if the mode is true, FS will cache creating tiny file. default: false. " ) \
     ( SDB_SEQUOIAFS_CREATECACHESIZE, po::value<INT32>() , "The cache size of creating file cache, default:1024 (MB), value range: [200-20480]" ) \
     ( SDB_SEQUOIAFS_CREATEPATH, po::value<std::string>(), "Creating file cache path, if filecreatecache is true, the path must be specified." ) \
     ( SDB_SEQUOIAFS_ALLOWOTHER, po::value<std::string>(), "Allow other users access the specified mountpoint, default: true" ) \
     ( SDB_SEQUOIAFS_BIGWRITES, po::value<std::string>(), "Enable larger than 4kB writes, default: true" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_MAXWRITE, "" ), po::value<INT32>() , "Set maximum size of write requests, default: 131072" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_MAXREAD, "" ), po::value<INT32>() ,  "Set maximum size of read requests, default: 131072" ) \
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_MAXREAD, "" ), po::value<INT32>() ,  "Set maximum size of read requests, default: 131072" ) \

#define FS_COMMANDS_HIDE_OPTIONS \
     ( SDB_SEQUOIAFS_STANDALONE, po::value<std::string>(), "The standalone mode, if the mode is true, FS will cache directory not rely on MCS. default: true. " ) \
//TODO: hide standalone parameter, if work with MCS. standalone should be false.

INT32 _sequoiafsOptionMgr::parseCollection( const string collection,
                                     string *cs,
                                     string *cl )
{
   INT32 rc = SDB_OK;
   string clFullName;
   size_t is_index = 0;

   clFullName = collection;

   is_index = clFullName.find( '.' );
   if( is_index != std::string::npos )
   {
     *cl = clFullName.substr( is_index + 1 );
     *cs = clFullName.substr( 0, is_index );
   }

   else
   {
     rc = SDB_INVALIDARG;
     ossPrintf( "The input collection's pattern is wrong( error=%d ), "
                "collecion:%s, exit." OSS_NEWLINE, rc, clFullName.c_str() );
     goto error;
   }

done:
   return rc;
error:
   goto done;

}


INT32 _sequoiafsOptionMgr::save()
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
      PD_RC_CHECK(  rc, PDERROR, "Failed to write config[%s], rc: %d",
                  _cfgFileName, rc  ) ;
   }

done:
   return rc ;
error:
   goto done ;
}

_sequoiafsOptionMgr::_sequoiafsOptionMgr()
{
   ossMemset( _hosts, 0, sizeof( _hosts ) );
   ossMemset( _userName, 0, sizeof( _userName ) );
   ossMemset( _passwd, 0, sizeof( _passwd ) );
   ossMemset( _mountpoint, 0, sizeof( _mountpoint ) );
   ossMemset( _alias, 0, sizeof( _alias ) );
   ossMemset( _collection, 0, sizeof( _collection ) );
   ossMemset( _metaFileCollection, 0, sizeof( _metaFileCollection ) );
   ossMemset( _metaDirCollection, 0, sizeof( _metaDirCollection ) );
   ossMemset( _cfgPath, 0, sizeof( _cfgPath ) );
   ossMemset( _cfgFileName, 0, sizeof( _cfgFileName ) );
   ossMemset( _diagPath, 0, sizeof( _diagPath ) );

   _connectionNum = SDB_SEQUOIAFS_CONNECTION_DEFAULT_MAX_NUM;
   //_cacheSize = SDB_SEQUOIAFS_CACHE_DEFAULT_SIZE;
   _diagLevel = PDWARNING;
   _replsize = SDB_SEQUOIAFS_REPLSIZE_DEFAULT_VALUE;
   _fuse_allow_other = TRUE;
   _fuse_big_writes = TRUE;
   _fuse_large_read = TRUE;   
   _fuse_max_write = SDB_SEQUOIAFS_MAXWRITE_DEFAULT_VALUE;
   _fuse_max_read = SDB_SEQUOIAFS_MAXREAD_DEFAULT_VALUE;
}

PDLEVEL _sequoiafsOptionMgr::getDiaglogLevel()const
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

INT32 _sequoiafsOptionMgr::init( INT32 argc,
                            CHAR **argv,
                            vector<string> *options4fuse )
{
   INT32 rc = SDB_OK;
   CHAR *cfgPath;
   CHAR tempPath[OSS_MAX_PATHSIZE + 1] = {0};
   CHAR fsName[OSS_MAX_PATHSIZE + 1] = { 0 } ;
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

   if(vmFromCmd.count(SDB_SEQUOIAFS_HELP))
   {
      std::cout << display << std::endl;
      fuse_str.push_back("--helpsfs");
      rc = SDB_PMD_HELP_ONLY;
      goto done;
   }

   if(vmFromCmd.count(SDB_SEQUOIAFS_VERSION))
   {
      ossPrintVersion("SequoiaFS version");
      fuse_str.push_back("--version");
      rc = SDB_PMD_VERSION_ONLY;
      goto done;
   }

   if(vmFromCmd.count(SDB_SEQUOIAFS_HELP_FUSE))
   {                
      fuse_str.push_back("--help");
      rc = SDB_PMD_HELP_ONLY;
      goto done;
   }

   //3. get conf path
   cfgTempPath = (vmFromCmd.count(SDB_SEQUOIAFS_CONF_PATH)) ? 
                    (vmFromCmd[SDB_SEQUOIAFS_CONF_PATH].as<string>().c_str()) : 
                    PMD_CURRENT_PATH;  

   ossMemset(tempPath, 0, sizeof(tempPath));
   cfgPath = ossGetRealPath(cfgTempPath, tempPath, OSS_MAX_PATHSIZE);
   if(!cfgPath)
   {
      if(vmFromCmd.count(SDB_SEQUOIAFS_CONF_PATH))
         std::cerr << "ERROR: Failed to get real path for "<< cfgTempPath<< endl;
      else
         SDB_ASSERT(FALSE, "Current path is impossible to be NULL");

      rc = SDB_INVALIDPATH;
      goto error;
   }

   rc = engine::utilBuildFullPath(tempPath, SDB_SEQUOIAFS_CFG_FILE_NAME,
                                  OSS_MAX_PATHSIZE, _cfgFileName);
   if(SDB_OK != rc)
   {
      std::cerr << "ERROR: Make configuration file name failed, rc:" <<
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
      goto error;
   }

   //5. add fuse arg
   //add mountpoint to fuse
   //fsname=alias(pid), subtype=sequoiafs
   //if arg is "--fuse_xxx false", do nothing
   //if arg is "--fuse_yyy true", add "-o yyy" to fuse
   //if arg is "--fuse_zzz 131027", add "-o zzz=131027" to fuse
   fuse_str.push_back(getmountpoint());
   
   ossSnprintf( fsName, OSS_MAX_PATHSIZE + 1, "fsname=%s(%d)",
                _alias, ossGetCurrentProcessID() ) ;
   fuse_str.push_back( SDB_SEQUOIAFS_FUSE_ARG );
   fuse_str.push_back( fsName );

   fuse_str.push_back( SDB_SEQUOIAFS_FUSE_ARG );
   fuse_str.push_back( "subtype=sequoiafs" );
   
   fuse_str.push_back( "-f" );

   rc = toBSON( allCfg, 0 ) ;
   if ( rc != SDB_OK )
   {
      std::cerr << "ERROR: convert configuration to bson failed, rc:" << rc << endl;
      goto error ;
   }
   for(BSONObj::iterator i = allCfg.begin(); i.more();) 
   {
      BSONElement e = i.next();
      const char* fieldName = e.fieldName();
      if(ossStrcmp(fieldName, "fuse_fsname") == 0 || ossStrcmp(fieldName, "fuse_subtype") == 0)
      {
         continue;
      }
      
      if (0 == ossStrncasecmp(fieldName, SDB_SEQUOIAFS_FUSE_PREFIX, ossStrlen(SDB_SEQUOIAFS_FUSE_PREFIX)))
      {
         string fusename = std::string( fieldName ).substr( ossStrlen(SDB_SEQUOIAFS_FUSE_PREFIX)) ; 
         if(e.type() == String)
         {
            const char* valuestr = e.valuestr();
            if(0 == ossStrncasecmp(valuestr, "false", ossStrlen(valuestr)))
            {
               continue;
            }
            else if(0 != ossStrlen(valuestr) && 0 != ossStrncasecmp(valuestr, "true", strlen(valuestr)))
            {
               fusename.append(SDB_SEQUOIAFS_FUSE_EQUAL);
               fusename.append(valuestr);
            }
         } 
         else 
         {
            string valuestr = e.toString(false, false);
            fusename.append(SDB_SEQUOIAFS_FUSE_EQUAL);
            fusename.append(valuestr);
         }
         fuse_str.push_back( SDB_SEQUOIAFS_FUSE_ARG );
         fuse_str.push_back( fusename );
      }
   }
   
done:
   *options4fuse = fuse_str;
   return rc;

error:
   goto done;
}

INT32 _sequoiafsOptionMgr::doDataExchange(pmdCfgExchange *pEX)
{
   resetResult();
   //--hosts
   rdxString(pEX, SDB_SEQUOIAFS_HOSTS, _hosts, sizeof(_hosts), FALSE,
             PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_HOSTS_DEFAULT_VALUE);
   //--username
   rdxString(pEX, SDB_SEQUOIAFS_USERNAME, _userName, sizeof(_userName),
             FALSE, PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_USER_DEFAULT_NAME);
   //--passwd
   rdxString(pEX, SDB_SEQUOIAFS_PASSWD, _passwd, sizeof(_passwd), FALSE,
             PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_USER_DEFAULT_PASSWD);
   //--cipherfile
   rdxString(pEX, SDB_SEQUOIAFS_CIPHERFILE, _cipherFile, sizeof(_cipherFile), FALSE,
             PMD_CFG_CHANGE_FORBIDDEN, "");    
   //--token
   rdxString(pEX, SDB_SEQUOIAFS_TOKEN, _token, sizeof(_token), FALSE,
             PMD_CFG_CHANGE_FORBIDDEN, "");
   //--mountpoint
   rdxString(pEX, SDB_SEQUOIAFS_MOUNTPOINT, _mountpoint, sizeof(_mountpoint), FALSE,
             PMD_CFG_CHANGE_FORBIDDEN, "");
   rdvNotEmpty(pEX, _mountpoint);
   //--mountpoint_alias
   rdxString(pEX, SDB_SEQUOIAFS_ALIAS, _alias, sizeof(_alias), FALSE,
             PMD_CFG_CHANGE_FORBIDDEN, "");
   //--collection
   rdxString(pEX, SDB_SEQUOIAFS_COLLECTION, _collection, sizeof(_collection),
             FALSE, PMD_CFG_CHANGE_FORBIDDEN, "");
   rdvNotEmpty(pEX, _collection);
   //--metafilecollection
   rdxString(pEX, SDB_SEQUOIAFS_META_FILE_CL, _metaFileCollection,
             sizeof(_metaFileCollection), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "");
   //--metadircollection
   rdxString(pEX, SDB_SEQUOIAFS_META_DIR_CL, _metaDirCollection,
             sizeof(_metaDirCollection), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "");

   //--connectionnum
   rdxInt(pEX, SDB_SEQUOIAFS_CONNECTION_NUM, _connectionNum, FALSE,
          PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_CONNECTION_DEFAULT_MAX_NUM);
   rdvMinMax(pEX, _connectionNum, 50, 1000, TRUE);

/*
   //--cachesize
   rdxInt(pEX, SDB_SEQUOIAFS_CACHE_SIZE, _cacheSize, FALSE,
          PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_CACHE_DEFAULT_SIZE);
   rdvMinMax(pEX, _cacheSize, 20, 1000000, TRUE);
*/
   //--maxdircachesize
   rdxInt(pEX, SDB_SEQUOIAFS_DIRCACHE_SIZE, _dirCacheSize, FALSE,
          PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_DIR_CACHE_DEFAULT_SIZE);
   rdvMinMax(pEX, _dirCacheSize, 10, 1000000, TRUE);

   //--maxdatacachesize
   rdxInt(pEX, SDB_SEQUOIAFS_DATACACHE_SIZE, _dataCacheSize, FALSE,
          PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_DATA_CACHE_DEFAULT_SIZE);
   rdvMinMax(pEX, _dataCacheSize, 200, 20480, TRUE);

   //--diaglevel
   rdxUShort(pEX, SDB_SEQUOIAFS_DIAGLEVEL, _diagLevel, FALSE,
             PMD_CFG_CHANGE_RUN, (UINT16)PDWARNING);
   rdvMinMax(pEX, _diagLevel, PDSEVERE, PDDEBUG, TRUE);

   //--replsize
   rdxInt(pEX, SDB_SEQUOIAFS_REPLSIZE, _replsize, FALSE,
             PMD_CFG_CHANGE_RUN, SDB_SEQUOIAFS_REPLSIZE_DEFAULT_VALUE);
   rdvMinMax(pEX, _replsize, -1, 7, TRUE);

   //--confpath
   rdxPath(pEX, SDB_SEQUOIAFS_CONF_PATH, _cfgPath, sizeof(_cfgPath),
           FALSE, PMD_CFG_CHANGE_FORBIDDEN, "");
   //--diagpath
   rdxPath(pEX, SDB_SEQUOIAFS_DIAGPATH, _diagPath, sizeof(_diagPath),
           FALSE, PMD_CFG_CHANGE_FORBIDDEN, PMD_CURRENT_PATH);
   //--diagnum
   rdxInt(pEX, SDB_SEQUOIAFS_DIAGNUM, _diagnum, FALSE, PMD_CFG_CHANGE_RUN,
          PD_DFT_FILE_NUM);
   rdvMinMax( pEX, _diagnum, (INT32)PD_MIN_FILE_NUM, (INT32)OSS_SINT32_MAX, TRUE ) ;
   //--fflag
   rdxInt(pEX, SDB_SEQUOIAFS_FLUSH_FLAG, _flushflag, FALSE, PMD_CFG_CHANGE_RUN,
          0);
   rdvMinMax(pEX, _flushflag, 0, 2, TRUE);
   //--forcemountflag
   rdxBooleanS(pEX, SDB_SEQUOIAFS_FORCE_MOUNT, _forcemount, FALSE, 
          PMD_CFG_CHANGE_RUN, FALSE);
   //--prereadblock
   rdxInt(pEX, SDB_SEQUOIAFS_PRE_READ, _prereadblock, FALSE, 
          PMD_CFG_CHANGE_RUN, SDB_SEQUOIAFS_PRE_READ_DEFAULT_NAME);
   rdvMinMax(pEX, _prereadblock, 1, 20, TRUE);
   //--standalone
   rdxBooleanS(pEX, SDB_SEQUOIAFS_STANDALONE, _standalone, FALSE, 
          PMD_CFG_CHANGE_RUN, TRUE);
   //--filecreatecache
   rdxBooleanS(pEX, SDB_SEQUOIAFS_CREATECACHE, _filecreatecache, FALSE, 
          PMD_CFG_CHANGE_RUN, FALSE);
   //--filecreatecachesize
   rdxInt(pEX, SDB_SEQUOIAFS_CREATECACHESIZE, _filecreatecachesize, FALSE,
          PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_CREAT_FILE_DEFAULT_SIZE);
   rdvMinMax(pEX, _dataCacheSize, 200, 20480, TRUE);       
   //--createfilepath
   rdxPath(pEX, SDB_SEQUOIAFS_CREATEPATH, _createfilePath, sizeof(_createfilePath),
           FALSE, PMD_CFG_CHANGE_FORBIDDEN, "");       
   //--fuse_allow_other
   rdxBooleanS(pEX, SDB_SEQUOIAFS_ALLOWOTHER, _fuse_allow_other, FALSE, 
          PMD_CFG_CHANGE_FORBIDDEN, TRUE);
   //--fuse_big_writes
   rdxBooleanS(pEX, SDB_SEQUOIAFS_BIGWRITES, _fuse_big_writes, FALSE, 
          PMD_CFG_CHANGE_FORBIDDEN, TRUE);
   //--fuse_large_read
   //rdxBooleanS(pEX, SDB_SEQUOIAFS_LARGEREAD, _fuse_large_read, FALSE,
     //     PMD_CFG_CHANGE_FORBIDDEN, TRUE);
   //--fuse_max_write
   rdxInt(pEX, SDB_SEQUOIAFS_MAXWRITE, _fuse_max_write, FALSE,
          PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_MAXWRITE_DEFAULT_VALUE);
   //--fuse_max_read
   rdxInt(pEX, SDB_SEQUOIAFS_MAXREAD, _fuse_max_read, FALSE,
          PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_MAXREAD_DEFAULT_VALUE);

   return getResult();
}

INT32 _sequoiafsOptionMgr::postLoaded( PMD_CFG_STEP step )
{
   INT32 rc = SDB_OK;
   string tempCSStr;
   string tempCLStr;
   string tempDirStr;
   string tempFileStr;
   CHAR tempPath[OSS_MAX_PATHSIZE + 1] = {0};

   rc = parseCollection(_collection, &tempCSStr, &tempCLStr);
   if(SDB_OK != rc)
   {
      goto error;
   }

   if(0 == ossStrlen(_metaFileCollection))
   {
       tempFileStr= _collection + SEQUOIAFS_META_FILE_SUFFIX;
       ossSnprintf(_metaFileCollection, sizeof(_metaFileCollection),
                  "%s", tempFileStr.c_str());
       _addToFieldMap(SDB_SEQUOIAFS_META_FILE_CL,
                      _metaFileCollection,
                      true,
                      true );
   }

   if(0 == ossStrlen(_metaDirCollection))
   {
      tempDirStr = _collection + SEQUOIAFS_META_DIR_SUFFIX;
      ossSnprintf(_metaDirCollection, sizeof(_metaDirCollection),
                  "%s", tempDirStr.c_str());
      _addToFieldMap(SDB_SEQUOIAFS_META_DIR_CL,
                     _metaDirCollection,
                     true,
                     true );
   }

   if(NULL == ossGetRealPath(_mountpoint, tempPath, OSS_MAX_PATHSIZE))
   {
      std::cerr << "ERROR: Failed to get real path for mountpoint: "<< _mountpoint<< endl;
      rc = SDB_INVALIDPATH;
      goto error;
   }
   ossMemcpy(_mountpoint, tempPath, OSS_MAX_PATHSIZE);

   rc = ossAccess(_mountpoint, OSS_MODE_ACCESS | OSS_MODE_READWRITE);
   if(rc != SDB_OK)
   {
      std::cerr << "ERROR: Failed to access the mountpoint: "<< _mountpoint<< endl;
      rc = SDB_INVALIDPATH;
      goto error;
   }

   if(0 == ossStrlen(_alias))
   {
      rc = parseAliasName();
      if(SDB_OK != rc)
      {
         goto error;
      }
      _addToFieldMap( SDB_SEQUOIAFS_ALIAS, _alias, true, true );
   }
   
done:
return rc ;
error:
goto done ;

}

INT32 _sequoiafsOptionMgr::parseAliasName()
{
   INT32 rc = SDB_OK;
   vector<string> path_str;

   //if mountpoint is "/opt/guestdir/", split the mountpoint with seperator "/"
   //the last one of split results is "guestdir", "guestdir" will be alias
   utilSplitStr(_mountpoint, path_str, OSS_FILE_SEP);
   if(path_str.size() >= 1 && path_str[path_str.size()-1].length() > 0)
   { 
      ossStrncpy(_alias, path_str[path_str.size()-1].c_str(), sizeof(_alias) -1 );
   }
   else
   {
      std::cerr << "ERROR: Parse alias failed. mountpoint is ["<< _mountpoint <<"], exit."<< endl;
      rc = SDB_INVALIDARG;
      goto error;
   }

done:
return rc ;
error:
goto done ;
   
}




