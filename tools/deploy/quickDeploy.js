/****************************************************************
@decription:   Deploy SequoiaDB, MySQL, PostgreSQL

@input:        sdb:        Boolean
               mysql:      Boolean
               pg:         Boolean
               cm:         Number, default: 11790
               mysqlPath:  String, mysql install path. Use it when you have
                           multiple installations of SequoiaSQL-MySQL.
               pgPath:     String, pg install path. Use when have multiple
                           installations of SequoiaSQL-PostgreSQL.

               eg: bin/sdb -f quickDeploy.js -e 'var sdb=true; var mysql=true; var cm=11790"'
               eg: bin/sdb -f quickDeploy.js -e 'var mysql=true; var mysqlPath="/opt/sequoiasql/mysql"'

@author:       Ting YU 2019-04-12
****************************************************************/

var USER_SET_DEPLOY = true ;

// check parameter
if ( typeof( sdb ) === "undefined" &&
     typeof( mysql ) === "undefined" &&
     typeof( pg ) === "undefined" )
{
   var sdb = true ;
   var mysql = true ;
   var pg = true ;
   USER_SET_DEPLOY = false ;
}

if ( typeof( sdb ) === "undefined" )
{
   var sdb = false ;
}
else if( sdb.constructor !== Boolean )
{
   throw "Invalid para[sdb], should be Boolean" ;
}

if ( typeof( mysql ) === "undefined" )
{
   var mysql = false ;
}
else if( mysql.constructor !== Boolean )
{
   throw "Invalid para[mysql], should be Boolean" ;
}

if ( typeof( pg ) === "undefined" )
{
   var pg = false ;
}
else if( pg.constructor !== Boolean )
{
   throw "Invalid para[pg], should be Boolean" ;
}

if ( typeof( cm ) === "undefined" )
{
   var cm = 11790 ;
}
else if( cm.constructor !== Number )
{
   throw "Invalid para[cm], should be Number" ;
}

if ( typeof( mysqlPath ) === "undefined" )
{
   var mysqlPath = "" ;
}
else if( mysqlPath.constructor !== String )
{
   throw "Invalid para[mysqlPath], should be String" ;
}
else
{
   if ( !mysql )
   {
      throw "Para[mysqlPath] can be set only when [mysql] is specified" ;
   }
}

if ( typeof( pgPath ) === "undefined" )
{
   var pgPath = "" ;
}
else if( pgPath.constructor !== String )
{
   throw "Invalid para[pgPath], should be String" ;
}
else
{
   if ( !pg )
   {
      throw "Para[pgPath] can be set only when [pg] is specified" ;
   }
}

// set global variable
var DEPLOY_SEQUOIADB       = sdb ;
var DEPLOY_MYSQL           = mysql ;
var DEPLOY_POSTGRESQL      = pg ;
var LOCAL_CM_PORT          = cm ;
var TMP_COORD_SVC          = 18800 ;
var MY_HOSTNAME            = System.getHostName() ;
var MYSQL_INSTALL_PATH     = mysqlPath ;
var PG_INSTALL_PATH        = pgPath ;
var TMP_COORD_INSTALL_PATH = "" ;

const DB_DATA_ROLE         = "data" ;
const DB_COORD_ROLE        = "coord" ;
const DB_CATA_ROLE         = "catalog" ;

const FIELD_SEQUOIADB_CATA_CONF  = "SEQUOIADB_CATA_CONF" ;
const FIELD_SEQUOIADB_COORD_CONF = "SEQUOIADB_COORD_CONF" ;
const FIELD_SEQUOIADB_DATA_CONF  = "SEQUOIADB_DATA_CONF" ;
const FIELD_MYSQL_INSTALL_PATH   = "MYSQL_INSTALL_PATH" ;
const FIELD_MYSQL_INSTANCE_CONF  = "MYSQL_INSTANCE_CONF" ;
const FIELD_PG_INSTALL_PATH      = "PG_INSTALL_PATH" ;
const FIELD_PG_INSTANCE_CONF     = "PG_INSTANCE_CONF" ;

// run!
main() ;

function main()
{
   var nodesConf ;
   var mysqlInfo ;
   var pgInfo ;

   if ( DEPLOY_SEQUOIADB )
   {
      nodesConf = checkSequoiadb() ;
   }
   if ( DEPLOY_MYSQL )
   {
      mysqlInfo = checkMysql() ;
   }
   if ( DEPLOY_POSTGRESQL )
   {
      pgInfo = checkPostgresql() ;
   }

   if ( DEPLOY_SEQUOIADB && nodesConf !== undefined )
   {
      deploySequoiadb( nodesConf ) ;
   }
   if ( DEPLOY_MYSQL && mysqlInfo !== undefined )
   {
      deployMysql( mysqlInfo ) ;
   }
   if ( DEPLOY_POSTGRESQL && pgInfo !== undefined )
   {
      deployPostgresql( pgInfo ) ;
   }
}

/**
 * Convert ini file to json obj. ini file format as "a=1\nb=2"
 *
 * @param  fileName Name of ini file
 * @return jsonObj  It is json obj, eg: { "a":1, "b":2 }.
 *                  If file not exist, return undefined.
 */
function iniFile2Obj( fileName )
{
   var exist = File.exist( fileName ) ;
   if ( !exist ) return ;

   // get file
   try
   {
      var file = new File( fileName, 0644, SDB_FILE_READONLY ) ;
   }
   catch( e )
   {
      throw e ;
   }

   // get file content
   var infoObj = {} ;
   while( true )
   {
      var aLine ;
      try
      {
         aLine = file.readLine() ;
         aLine = aLine.replace( /[\r\n]/g, "" ) ; // delete last line break
      }
      catch( e )
      {
         if( e == -9 ) break ; // -9: Hit end of file
         println( "Unexpected error[" + e + "] when read a line from " +
                  "configure file!" ) ;
         throw e ;
      }

      var conf = aLine.split( "=" ) ;
      infoObj[ conf[0] ] = conf[1] ;
   }
   return infoObj ;
}

/**
 * Get installation information, eg: user, installPath
 *
 * @param  dbType           database type, mysql or pg
 * @param  ignoreNotInstall Ignore error if installation not exist
 * @param  installPath      install path
 * @return jsonObj          installation info, eg:
 * {
 *   "VERSION": "3.2",
 *   "USER": "sdbadmin",
 *   "INSTALL_DIR": "/opt/sequoiasql/mysql",
 *   "MD5": "818cea64849dff4c1b572a6d6af5d757"
 * }
 */
function getSqlInstallInfo( dbType, ignoreNotInstall, installPath )
{
   if ( typeof( installPath ) === "undefined" ) installPath = "" ;

   var dbTypeStr  = "SequoiaSQL-MySQL" ;
   var paraStr    = "--mysql and --mysqlPath" ;
   var systemFile = "sequoiasql-mysql" ;
   var systemDir  = "/etc/default/" ;
   if ( dbType == "pg" )
   {
      systemFile  = "sequoiasql-postgresql" ;
      dbTypeStr   = "SequoiaSQL-PostgreSQL" ;
      paraStr     = "--pg and --pgPath" ;
   }
   else if ( dbType != "mysql" )
   {
      println( "Invalid type[" + dbType + "]!") ;
      throw "ERROR" ;
   }

   // find all /etc/default/sequoiasql-mysql[i]
   var foundOut = false ;
   var bsonArr = File.find( { mode: 'n',
                              value: '"' + systemFile + '*"',
                              pathname: systemDir } ) ;
   if ( bsonArr.size() == 0 && ignoreNotInstall )
   {
      return ;
   }

   if ( bsonArr.size() == 0 )
   {
      println( "ERROR: This machine hasn't installed " + dbTypeStr + "!" ) ;
      throw "ERROR" ;
   }
   if ( installPath == "" && bsonArr.size() > 1 )
   {
      println( "There are multiple "+dbTypeStr+" on this machine. You should "+
               "specify one installation path by " + paraStr + ".") ;
      throw "ERROR" ;
   }

   // loop every /etc/default/sequoiasql-mysql[i]
   var infoObj = {} ;
   while ( bsonArr.more() )
   {
      var systemFileFullName = bsonArr.next().toObj().pathname ;

      infoObj = iniFile2Obj( systemFileFullName ) ;

      if ( installPath != "" && infoObj.INSTALL_DIR == installPath )
      {
         foundOut = true ;
         break ;
      }
   }

   if ( installPath != "" && !foundOut )
   {
      println( "There is not "+dbTypeStr+" installation in "+installPath+"!" ) ;
      throw "ERROR" ;
   }

   if ( infoObj.INSTALL_DIR[0] != '/' )
   {
      println( "Invalid INSTALL_DIR in file[" + systemFileFullName + "]" ) ;
      throw "ERROR" ;
   }

   return infoObj ;
}

// return obj:
// {
//   "NAME": "sdbcm",
//   "SDBADMIN_USER": "sdbadmin",
//   "INSTALL_DIR": "/opt/source/sequoiadb/",
//   "MD5": "818cea64849dff4c1b572a6d6af5d757"
// }
function getSequoiadbInstallInfo( hostName )
{
   try
   {
      var oma = new Oma( MY_HOSTNAME, LOCAL_CM_PORT ) ;
   }
   catch( e )
   {

      println( "Unexpected error[" + e + "] when connecting cm[" + MY_HOSTNAME +
               ":" + LOCAL_CM_PORT + "]!" ) ;
      throw e ;
   }

   try
   {
      var cmPort = oma.getAOmaSvcName( hostName ) ;
      var omaRemote = new Oma( hostName, cmPort ) ;
   }
   catch( e )
   {

      println( "Unexpected error[" + e + "] when connecting cm[" + hostName +
               ":" + cmPort + "]!" ) ;
      throw e ;
   }

   var installInfo = {} ;
   try
   {
      installInfo = omaRemote.getOmaInstallInfo().toObj() ;
   }
   catch( e )
   {
      if ( e == -4 )
      {
         println( "ERROR: This machine has not installed SequoiaDB!" ) ;
         throw "ERROR" ;
      }
      else
      {
         throw e ;
      }
   }

   if ( installInfo.INSTALL_DIR[0] != '/' )
   {
      println( "Invalid INSTALL_DIR in file[/etc/default/sequoiadb]" ) ;
      throw "ERROR" ;
   }

   return installInfo ;
}

/**
 * Check the legality of each field in the mysql and postgresql configuration file
 *
 * The inspection rules for each field are as follows:
 * [ instanceName, port, installPath, coordAddr ]
 * instanceName can't be empty
 * port can't be empty and the range of the port must be between 0 and 65535
 * installPath must start with "[installpath]" or '/'
 * coordAddr can't be empty
 *
 * @param  confFile      the name of conf file( mysql.conf or postgresql.conf )
 * @param  instanceConf  mysql conf or pg conf
 * @param  line          number of rows for instanceConf in the conf file
 * @return jsonObj       mysql conf or pg conf
 *
 * mysql conf
 *
 * [ myinst,3306,/opt/sequoiasql/mysql/database/3306,u-fjiab:11810 ],
 *
 * or pg conf
 *
 * [ myinst,5432,/opt/sequoiasql/postgresql/database/5432,u-fjiab:11810 ],
 *
 */
function checkSqlConf( confFile, instanceConf, line )
{
   var len          = instanceConf.length ;
   var instanceName = instanceConf[0] ;
   var port         = instanceConf[1] ;
   var databaseDir  = instanceConf[2] ;
   var coordAddr    = instanceConf[3] ;

   // check instanceName
   if ( "" == instanceName )
   {
      println( "Invalid configure file[" + confFile + "], line[" + line +
               "]: wrong instanceName" ) ;
      throw "ERROR" ;
   }

   // check port
   if ( "undefined" == typeof( port ) )
   {
      println( "Invalid configure file[" + confFile + "], line[" + line +
               "]: empty port" ) ;
      throw "ERROR" ;
   }
   else if ( "" == port )
   {
      println( "Invalid configure file[" + confFile + "], line[" + line +
               "]: wrong port" ) ;
      throw "ERROR" ;
   }
   else
   {
      var tmpPort = Number( port ) ;
      if ( isNaN( tmpPort ) ||
           ( !isNaN( tmpPort ) && ( tmpPort < 0 || tmpPort > 65535 ) ) )
      {
         println( "Invalid configure file[" + confFile + "], line[" + line +
                  "]: wrong port" ) ; ;
         throw "ERROR" ;
      }
   }

   // check databaseDir
   if ( "undefined" == typeof( databaseDir ) )
   {
      println( "Invalid configure file[" + confFile + "], line[" + line +
               "]: empty databaseDir" ) ;
      throw "ERROR" ;
   }
   else if ( "" == databaseDir )
   {
      println( "Invalid configure file[" + confFile + "], line[" + line +
               "]: wrong databaseDir" ) ; ;
      throw "ERROR" ;
   }
   else
   {
      var databaseDirSplit = databaseDir.split( ']' ) ;
      if ( "[installPath" != databaseDirSplit[0] &&
           '/' != databaseDirSplit[0][0] )
      {
         println( "Invalid configure file[" + confFile + "], line[" + line +
                  "]: wrong databaseDir" ) ; ;
         throw "ERROR" ;
      }
   }

   // check coordAddr
   if ( "undefined" == typeof( coordAddr ) )
   {
      println( "Invalid configure file[" + confFile + "], line[" + line +
               "]: empty coordAddr" ) ;
      throw "ERROR" ;
   }
   else if ( "" == coordAddr )
   {
      println( "Invalid configure file[" + confFile + "], line[" + line +
               "]: wrong coordAddr" ) ; ;
      throw "ERROR" ;
   }

   if ( len == 4 )
   {
      instanceConf[3] = checkAndFormatCoordAddr( coordAddr, confFile ) ;
   }
   else if ( len > 4 )
   {
      coordAddr = "" ;
      for ( var i = 3; i < len ; i++ )
      {
         coordAddr += instanceConf[i] ;
         if ( i != ( len - 1 ) )
         {
            coordAddr += "," ;
         }
      }
      instanceConf[3] = checkAndFormatCoordAddr( coordAddr, confFile ) ;
      instanceConf.splice( 4, len - 4 ) ;
   }

   return instanceConf ;
}

function getSqlConf( dbType, installedPath )
{
   var selfPath = getSelfPath() ;

   var confFile = "" ;
   if ( dbType == "mysql" )
   {
      confFile = "mysql.conf" ;
   }
   else if ( dbType == "pg" )
   {
      confFile = "postgresql.conf" ;
   }
   else
   {
      println( "Invalid db type!" ) ;
      return ;
   }

   var fileFullPath = selfPath + "/" + confFile ;
   try
   {
      var file = new File( fileFullPath, 0644, SDB_FILE_READONLY ) ;
   }
   catch( e )
   {
      if ( e == -4 )
      {
         println( "File[" + fileFullPath + "] not exist!" ) ;
         return ;
      }
      else
      {
         throw e ;
      }
   }

   // check first line     TODO try catche
   var headLine = file.readLine() ;
   headLine =  headLine.replace( /[\r\n]/g, "" ) ; // delete last line break
   if ( headLine != "instanceName,port,databaseDir,coordAddr" )
   {
      println( "Invalide configure file! first line: " + headLine ) ;
      throw "ERROR" ;
   }

   // loop each line
   var allConf = [] ;
   var iLine = 1 ;
   while( true )
   {
      var aLine ;
      try
      {
         aLine = file.readLine() ;
         aLine = aLine.replace( /[\r\n]/g, "" ) ; // delete last line break
      }
      catch( e )
      {
         if( e == -9 ) break ; // -9: Hit end of file
         println( "Unexpected error[" + e + "] when read a line from " +
                  "configure file!" ) ;
         throw e ;
      }

      // check line
      iLine++ ;
      if ( aLine == "" ) continue ;

      if ( aLine.substr( 0,1 ) == "#" ) continue ;   // this line is a note

      // split line
      var instanceConf = aLine.split( "," ) ;
      var instanceConfChecked ;

      instanceConfChecked = checkSqlConf( confFile, instanceConf, iLine,
                                          installedPath ) ;

      // replace installed path
      instanceConfChecked[2] = instanceConfChecked[2].replace( /\[installPath\]/g,
                                                               installedPath ) ;

      // set coord address
      if ( instanceConfChecked[3] == "-" )
      {
         instanceConfChecked[3] = getACoordAddr() ;
      }

      allConf.push( instanceConfChecked ) ;
   }

   return allConf ;
}

function checkAndFormatCoordAddr( coordAddrStr, confFile )
{
   if ( coordAddrStr == "-" )
   {
      return coordAddrStr ;
   }

   // valid format:
   // localhost:50000 or [localhost:50000,localhost:11810]
   var coords = coordAddrStr.split( "," ) ;
   if ( coords.length == 1 )
   {
      if ( coordAddrStr.substr( 0, 1 )  == "[" &&
           coordAddrStr.substr( -1, 1 ) == "]" )
      {
         // delete the '[' at the beginning of the line
         coordAddrStr = coordAddrStr.replace( /(^\[)/, '' ) ;
         // delete the ']' at the end of the line
         coordAddrStr = coordAddrStr.replace( /(\]$)/, '' ) ;
      }
      var coordInfo = coordAddrStr.split( ':' ) ;
      if ( coordInfo.length != 2 )
      {
         println( "Invalid configure file[" + confFile + "]!" ) ;
         throw "ERROR" ;
      }
      var portStr = coordInfo[1] ;
      if ( portStr == "" )
      {
         println( "Invalid configure file[" + confFile + "]!" ) ;
         throw "ERROR" ;
      }
      var port = Number( portStr ) ;
      if ( isNaN( port ) )
      {
         // it is not number
         println( "Invalid configure file[" + confFile + "]!" ) ;
         throw "ERROR" ;
      }
   }
   else
   {
      // check first char is '[', last char is ']'
      if ( coordAddrStr.substr( 0, 1 )  != "[" ||
           coordAddrStr.substr( -1, 1 ) != "]" )
      {
         println( "Invalid configure file[" + confFile + "]!" ) ;
         throw "ERROR" ;
      }

      // delete the '[' at the beginning of the line
      coordAddrStr = coordAddrStr.replace( /(^\[)/, '' ) ;
      // delete the ']' at the end of the line
      coordAddrStr = coordAddrStr.replace( /(\]$)/, '' ) ;

      var coordArr = coordAddrStr.split( ',' ) ;
      for ( var i in coordArr )
      {
         var coordInfo = coordArr[i].split( ':' ) ;
         if ( coordInfo.length != 2 )
         {
            println( "Invalid configure file[" + confFile + "]!" ) ;
            throw "ERROR" ;
         }
         var portStr = coordInfo[1] ;
         if ( portStr == "" )
         {
            println( "Invalid configure file[" + confFile + "]!" ) ;
            throw "ERROR" ;
         }
         var port = Number( portStr ) ;
         if ( isNaN( port ) )
         {
            // it is not number
            println( "Invalid configure file[" + confFile + "]!" ) ;
            throw "ERROR" ;
         }
      }
   }

   return coordAddrStr ;
}

function getACoordAddr()
{
   var coordAddr = "" ;

   var nodesConf = getSequoiadbConf() ;
   for ( var i in nodesConf )
   {
      var aNode = nodesConf[i] ;
      if ( aNode[0] == "coord" )
      {
         coordAddr = aNode[2] + ":" + aNode[3] ;
         break ;
      }
   }

   return coordAddr ;
}

/**
 * Check the legality of each field in the sequoiadb configuration file
 *
 * The inspection rules for each field are as follows:
 * [ role, groupName, hostName, serviceName, dbPath ]
 * role can't be empty and must be "data" or "coord" or "catalog"
 * groupName can't be empty
 * hostName can't br empty
 * serviceName can't be empty
 * the range of serviceName must be between 0 and 65535
 * dbPath must start with "[installpath]" or '/'
 *
 * @param  aNodeConf   sequoiadb node configuration
 * @param  line        number of rows for aNodeConf in the conf file
 * @Param  nodeNameList check for repeat hostName:serviceName in conf file
 * @return null
 *
 */
function checkSequoiadbConf( aNodeConf, line, nodeNameList )
{
   var dbRole      = aNodeConf[0] ;
   var groupName   = aNodeConf[1] ;
   var hostname    = aNodeConf[2] ;
   var serviceName = aNodeConf[3] ;
   var dbPath      = aNodeConf[4] ;

   // check dbRole
   if ( DB_CATA_ROLE != dbRole && DB_COORD_ROLE != dbRole &&
        DB_DATA_ROLE != dbRole )
   {
      println( "Invalid configure file[sequoiadb.conf], line[" + line +
               "]: wrong role" ) ;
      throw "ERROR" ;
   }

   // check groupName
   if ( "undefined" == typeof( groupName ) )
   {
      println( "Invalid configure file[sequoiadb.conf], line[" + line +
               "]: empty groupName" ) ;
      throw "ERROR" ;
   }
   else if ( "" == groupName )
   {
      println( "Invalid configure file[sequoiadb.conf], line[" + line +
               "]: wrong groupName" ) ;
      throw "ERROR" ;
   }

   // check hostname
   if ( "undefined" == typeof( hostname ) )
   {
      println( "Invalid configure file[sequoiadb.conf], line[" + line +
               "]: empty hostName" ) ;
      throw "ERROR" ;
   }
   else if ( "" == hostname )
   {
      println( "Invalid configure file[sequoiadb.conf], line[" + line +
               "]: wrong hostName" ) ;
      throw "ERROR" ;
   }

   // check serviceName
   if ( "undefined" == typeof( serviceName ) )
   {
      println( "Invalid configure file[sequoiadb.conf], line[" + line +
               "]: empty serviceName" ) ;
      throw "ERROR" ;
   }
   else if ( "" == serviceName )
   {
      println( "Invalid configure file[sequoiadb.conf], line[" + line +
               "]: wrong serviceName" ) ;
      throw "ERROR" ;
   }
   else
   {
      var tmpServiceName = Number( serviceName ) ;
      if ( isNaN( tmpServiceName ) ||
           ( !isNaN( tmpServiceName ) &&
             ( tmpServiceName < 0 || tmpServiceName > 65535 ) ) )
      {
         println( "Invalid configure file[sequoiadb.conf], line[" + line +
                  "]: wrong serviceName" ) ;
         throw "ERROR" ;
      }
   }

   // check hostName + serviceName is repeat
   if ( nodeNameList.indexOf( hostname + ":" + serviceName ) !== -1 )
   {
      println( "Invalid configure file[sequoiadb.conf], line[" + line +
          "]: repeated hostName:serviceName" ) ;
      throw "ERROR" ;
   }

   // check dbPath
   if ( "undefined" == typeof( dbPath ) )
   {
      println( "Invalid configure file[sequoiadb.conf], line[" + line +
               "]: empty dbPath" ) ;
      throw "ERROR" ;
   }
   else if ( "" == dbPath )
   {
      println( "Invalid configure file[sequoiadb.conf], line[" + line +
               "]: wrong dbPath" ) ;
      throw "ERROR" ;
   }
   else
   {
      var dbPathSplit = dbPath.split( ']' ) ;
      if ( "[installPath" != dbPathSplit[0] && '/' != dbPathSplit[0][0] )
      {
         println( "Invalid configure file[sequoiadb.conf], line[" + line +
                  "]: wrong dbPath" ) ;
         throw "ERROR" ;
      }
   }
}

function getSequoiadbConf( replaceInstallPath )
{
   if ( typeof( replaceInstallPath ) === "undefined" )
   {
      replaceInstallPath = false ;
   }

   var selfPath = getSelfPath() ;
   var fileFullPath = selfPath + "/sequoiadb.conf" ;
   var file = new File( fileFullPath, 0644, SDB_FILE_READONLY ) ;

   // check first line     TODO try catche
   var headLine = file.readLine() ;
   headLine =  headLine.replace( /[\r\n]/g, "" ) ; // delete last line break
   if ( headLine != "role,groupName,hostName,serviceName,dbPath" )
   {
      println( "Invalide configure file! first line: " + headLine ) ;
      throw "ERROR" ;
   }

   // loop each line
   var nodesConf = [] ;
   var iLine = 1 ;
   var nodeNameList = [] ;
   while( true )
   {
      var aLine ;
      try
      {
         aLine = file.readLine() ;
         aLine = aLine.replace( /[\r\n]/g, "" ) ; // delete last line break
      }
      catch( e )
      {
         if( e == -9 ) break ; // -9: Hit end of file
         println( "Unexpected error[" + e + "] when read a line from " +
                  "configure file!" ) ;
         throw e ;
      }

      // check line
      iLine++ ;
      if ( aLine == "" ) continue ;

      if ( aLine.substr( 0,1 ) == "#" ) continue ;   // this line is a note

      var aNode = aLine.split( "," ) ;
      // replace 'localhost' to real hostname
      aNode[2] = aNode[2].replace( /localhost/g, MY_HOSTNAME ) ;

      checkSequoiadbConf( aNode, iLine, nodeNameList ) ;
      nodeNameList.push( aNode[2] + ":" + aNode[3] ) ;

      // replace installed path
      if ( replaceInstallPath )
      {
         var installedPath = getSequoiadbInstallInfo( aNode[2] ).INSTALL_DIR ;
         aNode[4] = aNode[4].replace( /\[installPath\]/g, installedPath ) ;
      }

      nodesConf.push( aNode ) ;
   }

   return nodesConf ;
}

function createTmpCoord()
{
   var oma = new Oma( MY_HOSTNAME, LOCAL_CM_PORT ) ;

   try
   {
      oma.createCoord( TMP_COORD_SVC, TMP_COORD_INSTALL_PATH ) ;
   }
   catch( e )
   {
      if ( e != -145 )  // -145: already exists, ignore error
      {
         println( "Unexpected error[" + e + "] when creating temp coord: " +
                  "localhost:" + TMP_COORD_SVC + "!" ) ;
         throw e ;
      }
   }

   oma.startNode( TMP_COORD_SVC ) ;
}

function checkCataPrimary( db )
{
   var hasPrimary = false;

   for( var i = 0; i < 10*600; i++ )//wait for cata group to select primary node
   {
      try
      {
         sleep( 100 ) ;
         var cataRG = db.getRG( "SYSCatalogGroup" ) ;
         hasPrimary = true ;
         break ;
      }
      catch(e)
      {
         if( e !== -71 )
         {
            println( "Unexpected error[" + e + "] when " +
                     "db.getRG( \"SYSCatalogGroup\" )!" ) ;
            throw e;
         }
      }
   }

   if( hasPrimary === false )
   {
      println( "Fail to select primary node in group[SYSCatalogGroup] " +
               "after 10 minute" ) ;
      return false ;
   }

   return true ;
}

function checkeDataPrimary( db, groupName )
{
   var hasPrimary = false ;

   for( var i = 0; i < 10*600; i++ )//wait for data group to select primary node
   {
      try
      {
         sleep(100);
         db.getRG( groupName ).getMaster();
         hasPrimary = true;
         break;
      }
      catch(e)
      {
         if( e !== -71 )
         {
            println( "Unexpected error[" + e + "] when getting group[" +
                     groupName + "]!" ) ;
            throw e;
         }
      }
   }

   if( hasPrimary === false )
   {
      println( "Fail to select primary node in group[" + groupName +
               "] after 10 minute" ) ;
      return false ;
   }

   return true ;
}

function addCataAddr2TmpCoord( cataHostName, cataSvc )
{
   var oma = new Oma( MY_HOSTNAME, LOCAL_CM_PORT ) ;

   var cataPort = parseInt( cataSvc ) + 3 ;
   var cataAddrSetting = cataHostName + ":" + cataPort ;
   oma.updateNodeConfigs( TMP_COORD_SVC, { catalogaddr: cataAddrSetting } ) ;

   oma.stopNode( TMP_COORD_SVC ) ;
   oma.startNode( TMP_COORD_SVC ) ;

   var db = new Sdb( MY_HOSTNAME, TMP_COORD_SVC ) ;

   return db ;
}

function createCatalog( nodesConf )
{
   if ( nodesConf.length == 0 ) return ;

   var db = new Sdb( MY_HOSTNAME, TMP_COORD_SVC ) ;

   for ( var i in nodesConf )
   {
      var aNodeConf = nodesConf[i] ;
      var hostName = aNodeConf[2] ;
      var service = aNodeConf[3] ;
      var dbPath = aNodeConf[4] ;

      if ( i == 0 )
      {
         try
         {
            db.createCataRG( hostName, service, dbPath ) ;
         }
         catch( e )
         {
            if ( e == -145 || e == -200 ) // -145: already exists, ignore error
            {
               db = addCataAddr2TmpCoord( hostName, service ) ;
            }
            else
            {
               println( "Unexpected error[" + e + "] when creating catalog " +
                        "node: " + hostName + ":" + service + "!" ) ;
               throw e ;
            }
         }
      }
      else
      {
         try
         {
            var rg = db.getCatalogRG() ;
            var node = rg.createNode( hostName, service, dbPath ) ;
         }
         catch( e )
         {
            if ( e != -145 ) // -145: already exists, ignore error
            {
               println( "Unexpected error[" + e + "] when creating catalog " +
                         "node: " + hostName + ":" + service + "!" ) ;
               throw e ;
            }
         }
         try
         {
            rg.getNode( hostName, service ).start() ;
         }
         catch( e )
         {
            println( "Unexpected error[" + e + "] when starting catalog node: "
                     + hostName + ":" + service + "!" ) ;
            throw e ;
         }
      }

      println( "Create catalog: " + hostName + ":" + service ) ;
   }

   var rc = checkCataPrimary( db ) ;
   if ( !rc )
   {
      println( "Failed to wait catalog group change primary!" ) ;
      throw "ERROR" ;
   }
}

function createCoord( nodesConf )
{
   if ( nodesConf.length == 0 ) return ;

   var db = new Sdb( MY_HOSTNAME, TMP_COORD_SVC ) ;

   try
   {
      db.createCoordRG() ;
   }
   catch( e )
   {
      if ( e != -153 ) // -153: group already exist, ignore error
      {
         println( "Unexpected error[" + e + "] when creating coord group!" ) ;
         throw e ;
      }
   }

   for ( var i in nodesConf )
   {
      var aNodeConf = nodesConf[i] ;
      var hostName = aNodeConf[2] ;
      var service = aNodeConf[3] ;
      var dbPath = aNodeConf[4] ;

      try
      {
         var rg = db.getCoordRG() ;
         rg.createNode( hostName, service, dbPath ) ;
      }
      catch( e )
      {
         if ( e != -145 ) // -145: node already exist, ignore error
         {
            println( "Unexpected error[" + e + "] when creating coord node!" ) ;
            throw e ;
         }
      }

      println( "Create coord:   " + hostName + ":" + service ) ;
   }

   try
   {
      rg.start() ;
   }
   catch( e )
   {
      println( "Unexpected error[" + e + "] when starting coord group!" ) ;
      throw e ;
   }
}

/**
 * Map, similar to STL map.
 * Different is second value is array, and if first value has been existed in
 * UtilMap, second value will add to array.
 *
 * @function  add( "group1", "hostname1" )
 * @function  delete( "group1" )
 * @function  hasNext()
 * @function  curFirst()
 * @function  curSecond()
 */
function UtilMap()
{
   this._dataFirst = [] ;   // [group1,       group2,       group3 ...]
   this._dataSecond= [] ;   // [[host1,host2],[host1,host2],[host1,host2]...]
   this.add        = _add ;
   this.delete     = _delete ;
   this.hasNext    = _hasNext ;
   this.curFirst   = _curFirst ;
   this.curSecond  = _curSecond ;
   this._curPos    = -1 ;
   function _add( first, second )
   {
      var pos = this._dataFirst.indexOf( first ) ;
      if ( pos == -1 )
      {
         this._dataFirst.push( first );
         this._dataSecond.push( [second] );
      }
      else
      {
         var i = this._dataSecond[pos].indexOf( second ) ;
         if ( i == -1 )
         {
            this._dataSecond[pos].push( second ) ;
         }
      }
   }
   function _delete( first )
   {
      var pos = this._dataFirst.indexOf( first ) ;
      if ( pos == -1 ) return ;

      delete this._dataFirst[pos] ;
      delete this._dataSecond[pos] ;
   }
   function _hasNext()
   {
      this._curPos++ ;
      if ( this._curPos >= this._dataFirst.length )
      {
         return false ;
      }
      return true ;
   }
   function _curFirst()
   {
      if ( this._curPos >= this._dataFirst.length )
      {
         return ;
      }
      return this._dataFirst[this._curPos] ;
   }
   function _curSecond()
   {
      if ( this._curPos >= this._dataSecond.length )
      {
         return ;
      }
      return this._dataSecond[this._curPos] ;
   }
}

/**
 * Special queue, it is a priority queue.
 * The element which is ranked in front of the array has higher priority than
 * the element behind. If you want to lower the priority, use put2Tail().
 *
 * @function  add( "hostname1" )
 * @function  put2Tail( "hostname1" )
 * @function  getPriority( "hostname1" )
 * @function  print()
 */
function UtilPriorityQueue()
{
   this._dataStore = [] ;
   this.add        = _add ;
   this.put2Tail   = _put2Tail ;
   this.getPriority= _getPriority ;
   this.print      = _print ;
   function _add( data )
   {
      if ( this._dataStore.indexOf( data ) != -1 )
      {
         return false ;
      }
      this._dataStore.push( data ) ;
      return true ;
   }
   function _getPriority( data )
   {
      return this._dataStore.indexOf( data ) ;
   }
   function _put2Tail( data )
   {
      var pos = this._dataStore.indexOf( data ) ;
      if ( pos == -1 )
      {
         return false ;
      }
      this._dataStore.splice( pos, 1 ) ;
      this._dataStore.push( data ) ;
      return true ;
   }
   function _print()
   {
      println( this._dataStore ) ;
   }
}

/**
 * Balance weight of vote, to make the primary node evenly on different hosts
 *
 * @param  nodesInfo, as below
 * [
 *    [ "group1", "hostname1", 11810, "dbpath1", {} ],
 *    [ "group2", "hostname2", 11820, "dbpath2", {} ],
 *    ...
 * ]
 * @return undefined, but set nodesInfo as below
 * [
 *    [ "group1", "hostname1", 11810, "dbpath1", { weight: 20 } ],
 *    [ "group2", "hostname2", 11820, "dbpath2", {} ],
 *    ...
 * ]
 */
function balanceVoteWeight( nodesInfo )
{
   var hostPriorityQue = new UtilPriorityQueue() ;
   var groupMap = new UtilMap() ;
   var highWeightNodes = [] ;

   // construct host and group info
   for ( var i in nodesInfo )
   {
      var aNode     = nodesInfo[i] ;
      var groupName = aNode[1] ;
      var hostName  = aNode[2] ;
      hostPriorityQue.add( hostName ) ;
      groupMap.add( groupName, hostName ) ;
   }

   // loop every group to find out higher priority node
   while( groupMap.hasNext() )
   {
      var groupName = groupMap.curFirst() ;
      var hostList  = groupMap.curSecond() ;

      if( hostList.length < 2 )
      {
         // if only one node, don't need to specified { weight: 20 }
         continue ;
      }

      var higherPri = Infinity ;
      var higherPriHost = "" ;
      for( var i in hostList )
      {
         var pri = hostPriorityQue.getPriority( hostList[i] ) ;
         if ( pri < higherPri )
         {
            higherPri = pri ;
            higherPriHost = hostList[i] ;
         }
      }
      hostPriorityQue.put2Tail( higherPriHost ) ;
      highWeightNodes.push( { "GroupName": groupName,
                              "HostName":  higherPriHost } ) ;
   }

   // set node configure: {weight: 20}. default node weight is 10.
   for ( var i in nodesInfo )
   {
      var aNode     = nodesInfo[i] ;
      var groupName = aNode[1] ;
      var hostName  = aNode[2] ;
      var nodeConf  = aNode[5] ;
      for( var j in highWeightNodes )
      {
         var hwGroupName = highWeightNodes[j].GroupName ;
         var hwHostName  = highWeightNodes[j].HostName ;
         if ( hwGroupName == groupName &&
              hwHostName  == hostName )
         {
            // set weight
            nodeConf.weight = 20 ;
            // lower priority of this host
            highWeightNodes.splice( j, 1 ) ;
            break ;
         }
      }
   }
}

function createData( nodesConf )
{
   if ( nodesConf.length == 0 ) return ;

   balanceVoteWeight( nodesConf ) ;

   var db = new Sdb( MY_HOSTNAME, TMP_COORD_SVC ) ;

   for ( var i in nodesConf )
   {
      var aNodeConf = nodesConf[i] ;
      var groupName = aNodeConf[1] ;
      var hostName = aNodeConf[2] ;
      var service = aNodeConf[3] ;
      var dbPath = aNodeConf[4] ;
      var configure = aNodeConf[5] ;

      try
      {
         var rg = db.getRG( groupName ) ;
      }
      catch( e )
      {
         if ( e == -154 )
         {
            var rg = db.createRG( groupName ) ;
         }
         else
         {
            println( "Unexpected error[" + e + "] when get data group[" +
                     groupName + "]!" ) ;
            throw e ;
         }
      }

      try
      {
         rg.createNode( hostName, service, dbPath, configure ) ;
      }
      catch( e )
      {
         if ( e != -145 )
         {
            println( "Unexpected error[" + e + "] when creating data node[" +
                     hostName + ":" + service + "]!" ) ;
            throw e ;
         }
      }

      println( "Create data:    " + hostName + ":" + service ) ;
   }

   for ( var i in nodesConf )
   {
      var aNodeConf = nodesConf[i] ;
      var groupName = aNodeConf[1] ;

      try
      {
         var rg = db.getRG( groupName ) ;
         rg.start() ;
      }
      catch( e )
      {
         println( "Unexpected error[" + e + "] when starting data group[" +
                  groupName + "]!" ) ;
         throw e ;
      }
   }
}

function removeTmpCoord()
{
   var oma = new Oma( MY_HOSTNAME, LOCAL_CM_PORT ) ;
   var service = TMP_COORD_SVC ;

   try
   {
      oma.removeCoord( service ) ;
   }
   catch( e )
   {
      println( "Unexpected error[" + e + "] when removing temp coord[localhost:"
               + service + "]!" ) ;
      throw e ;
   }
}

function checkUser( dbType, installInfo )
{
   if ( dbType == "sequoiadb" )
   {
      var expUser = installInfo.SDBADMIN_USER ;
   }
   else
   {
      var expUser = installInfo.USER ;
   }

   var curUser = System.getUserEnv().toObj().USER ;

   if ( expUser != curUser )
   {
      println( "You should execute this script by user[" + expUser + "], " +
               "but current user is [" + curUser + "]!" ) ;
      throw "ERROR" ;
   }
}

function checkFilePath( filePath, hostname, expectExist )
{
   try
   {
      var remote = new Remote( hostname, LOCAL_CM_PORT ) ;
   }
   catch( e )
   {
      println( "Unexpected error[" + e + "] when connect to host[" +
               hostName + ":" + LOCAL_CM_PORT + "]!" ) ;
      throw e ;
   }

   var isExist = remote.getFile().exist( filePath ) ;
   if( expectExist && !isExist )
   {
      println( "The file[" + filePath + "] of the host[" + hostname +
               "] doesn't exist" ) ;
      throw "ERROR" ;
   }

   if( !expectExist && isExist )
   {
      println( "The file[" + filePath + "] of the host[" + hostname +
               "] already exists" ) ;
      throw "ERROR" ;
   }
}

function checkPort( port, hostname )
{
   try
   {
      var remote = new Remote( hostname, LOCAL_CM_PORT ) ;
   }
   catch( e )
   {
      println( "Unexpected error[" + e + "] when connect to host["
               + hostName + ":" + LOCAL_CM_PORT + "]" ) ;
      throw e ;
   }

   if( !remote.getSystem().sniffPort( port ).toObj().Usable )
   {
      println( "The port[" + port + "] of the host[" + hostname +
               "] has been used" ) ;
      throw "ERROR" ;
   }
}

function checkNodeConf( nodesConf )
{
   if ( nodesConf.length == 0 )
   {
      return ;
   }

   for ( var i in nodesConf )
   {
      var aNodeConf = nodesConf[i] ;
      var dbHostname = aNodeConf[2] ;
      var service = aNodeConf[3] ;
      var dbPath = aNodeConf[4] ;
      checkPort( service, dbHostname ) ;
      checkFilePath( dbPath, dbHostname, false ) ;
   }
}

/**
 * Check and return sequoiadb nodes configuration: data configuration,
 *                                                 coord configuration and
 *                                                 cata configuration.
 * The configuration include instance role, groupName, hostName,
 *                           serviceName and dbPath
 * @param  null
 * @return jsonObj          sequoiadb nodes configuration
 * {
 *   "SEQUOIADB_DATA_CONF" :
 *   [
 *      [ data,group1,localhost,11820,/opt/sequoiadb/database/data/11820 ],
 *      [ data,group2,localhost,11830,/opt/sequoiadb/database/data/11830 ],
 *      ...
 *   ]
 *   "SEQUOIADB_COORD_CONF":
 *   [
 *      [ coord,SYSCoord,localhost,11810,/opt/sequoiadb/database/coord/11810 ],
 *      ...
 *   ]
 *   "SEQUOIADB_CATA_CONF" :
 *   [
 *      [ catalog,SYSCatalogGroup,localhost,11800,
 *        /opt/sequoiadb/database/catalog/11800 ],
 *      ...
 *   ]
 * }
 */
function checkSequoiadb()
{
   // check it has installation or not
   var installInfo = getSequoiadbInstallInfo( MY_HOSTNAME ) ;
   if ( installInfo == undefined )
   {
      throw "ERROR" ;
   }
   else
   {
      TMP_COORD_INSTALL_PATH = installInfo.INSTALL_DIR + "/database/coord/" +
                               TMP_COORD_SVC ;
   }

   // check user
   checkUser( "sequoiadb", installInfo ) ;

   // get node configure
   var nodesConf = {} ;
   var catalogConf = [] ;
   var coordConf = [] ;
   var dataConf = [] ;
   var nodesConf = getSequoiadbConf( true ) ;
   for ( var i in nodesConf )
   {
      var aNodeConf = nodesConf[i] ;
      var role = aNodeConf[0] ;
      aNodeConf[ aNodeConf.length ] = {} ; // default node configure is null
      if ( role == "catalog" )
      {
         catalogConf.push( aNodeConf ) ;
      }
      else if ( role == "coord" )
      {
         coordConf.push( aNodeConf ) ;
      }
      else if ( role == "data" )
      {
         dataConf.push( aNodeConf ) ;
      }
      else
      {
         println( "Unexpect configure: role[" + role + "]" ) ;
         throw "ERROR" ;
      }
   }

   if ( catalogConf.length < 1 )
   {
      println( "SequoiaDB need at least 1 catalog!" ) ;
      throw "ERROR" ;
   }

   // check sequoiadb cluster conf
   checkPort( TMP_COORD_SVC, MY_HOSTNAME ) ;
   checkFilePath( TMP_COORD_INSTALL_PATH, MY_HOSTNAME, false ) ;

   checkNodeConf( catalogConf ) ;
   checkNodeConf( coordConf ) ;
   checkNodeConf( dataConf ) ;

   nodesConf[FIELD_SEQUOIADB_CATA_CONF] = catalogConf ;
   nodesConf[FIELD_SEQUOIADB_COORD_CONF] = coordConf ;
   nodesConf[FIELD_SEQUOIADB_DATA_CONF] = dataConf ;

   return nodesConf ;
}

/**
 * Check and return mysql info: mysql installation path and
 *                              mysql instance configuration
 *                              include instance name, port, databaseDir and
 *                              coordAddr
 * @param  null
 * @return jsonObj          mysql info
 * {
 *   "MYSQL_INSTALL_PATH":  "/opt/sequoiasql/mysql",
 *   "MYSQL_INSTANCE_CONF":
 *   [
 *      [ myinst1,3306,/opt/sequoiasql/mysql/database/3306,u-fjiab:11810 ],
 *      [ myinst2,3307,/opt/sequoiasql/mysql/database/3307,u-fjiab:11810 ],
 *      ...
 *   ]
 * }
 */
function checkMysql()
{
   var mysqlInfo = {} ;
   var ignoreNotInstall = !USER_SET_DEPLOY ;

   // check it has installation or not
   var installInfo = getSqlInstallInfo( "mysql", ignoreNotInstall,
                                        MYSQL_INSTALL_PATH ) ;
   if ( installInfo == undefined && ignoreNotInstall )
   {
      if( USER_SET_DEPLOY == true )
      {
         println("sequoiasql-mysql does not exist, please install ");
      }
      return ;
   }
   var installedPath = installInfo.INSTALL_DIR ;
   mysqlInfo[FIELD_MYSQL_INSTALL_PATH] = installedPath ;

   // check user
   checkUser( "mysql", installInfo ) ;

   // get configure
   var allConf = getSqlConf( "mysql", installedPath ) ;

   // check mysql instance conf
   for ( var i in allConf )
   {
      var instanceConf = allConf[i] ;
      var port = instanceConf[1] ;
      var databaseDir = instanceConf[2] ;
      checkPort( port, MY_HOSTNAME ) ;
      checkFilePath( databaseDir, MY_HOSTNAME, false ) ;
   }

   mysqlInfo[FIELD_MYSQL_INSTANCE_CONF] = allConf ;

   return mysqlInfo ;
}

/**
 * Check and return pg info: pg installation path and pg instance configuration
 *                           include instance name, port, databaseDir and
 *                           coordAddr
 * @param  null
 * @return jsonObj       pg info
 * {
 *   "PG_INSTALL_PATH":  "/opt/sequoiasql/postgresql",
 *   "PG_INSTANCE_CONF":
 *   [
 *      [ myinst1,5432,/opt/sequoiasql/postgresql/database/5432,u-fjiab:11810 ],
 *      [ myinst2,5433,/opt/sequoiasql/postgresql/database/5433,u-fjiab:11810 ],
 *      ...
 *   ]
 * }
 */
function checkPostgresql()
{
   var pgInfo = {} ;
   var ignoreNotInstall = !USER_SET_DEPLOY ;

   // check it has installation or not
   var installInfo = getSqlInstallInfo( "pg", ignoreNotInstall,
                                        PG_INSTALL_PATH ) ;
   if ( installInfo == undefined && ignoreNotInstall )
   {
      if( USER_SET_DEPLOY == true )
      {
         println("sequoiasql-postgresql does not exist, please install ");
      }
      return ;
   }
   var installedPath = installInfo.INSTALL_DIR ;
   pgInfo[FIELD_PG_INSTALL_PATH] = installedPath ;

   // check user
   checkUser( "pg", installInfo ) ;

   // get configure
   var allConf = getSqlConf( "pg", installedPath ) ;

   // check pg conf
   for ( var i in allConf )
   {
      var instanceConf = allConf[i] ;
      var port = instanceConf[1] ;
      var databaseDir = instanceConf[2] ;
      checkPort( port, MY_HOSTNAME ) ;
      checkFilePath( databaseDir, MY_HOSTNAME, false ) ;
   }

   pgInfo[FIELD_PG_INSTANCE_CONF] = allConf ;

   return pgInfo ;
}

function deploySequoiadb( nodesConf )
{
   println( "\n************ Deploy SequoiaDB ************************" ) ;

   // get node configure
   var catalogConf = nodesConf[FIELD_SEQUOIADB_CATA_CONF] ;
   var coordConf = nodesConf[FIELD_SEQUOIADB_COORD_CONF] ;
   var dataConf = nodesConf[FIELD_SEQUOIADB_DATA_CONF] ;

   // create sequoiadb cluster
   createTmpCoord() ;
   createCatalog( catalogConf ) ;
   createCoord( coordConf ) ;
   createData( dataConf ) ;

   removeTmpCoord() ;
}

function deployMysql( mysqlInfo )
{
   println( "\n************ Deploy SequoiaSQL-MySQL *****************" ) ;
   var installedPath = mysqlInfo[FIELD_MYSQL_INSTALL_PATH] ;
   var allConf = mysqlInfo[FIELD_MYSQL_INSTANCE_CONF] ;
   var sqlCtl = installedPath + "/bin/sdb_sql_ctl" ;
   var cmd = new Cmd() ;

   // create instance
   for ( var i in allConf )
   {
      var instanceConf = allConf[i] ;
      var instanceName = instanceConf[0] ;
      var port = instanceConf[1] ;
      var databaseDir = instanceConf[2] ;
      var coordAddr = instanceConf[3] ;
      var newInst = true ;

      try
      {
         // add instance
         var command = sqlCtl + " addinst " + instanceName + " -D " +
                       databaseDir + " -p " + port ;
         cmd.run( command ) ;
      }
      catch( e )
      {
         var rc = cmd.getLastRet() ;
         if ( rc == 8 ) // 8: instance exist
         {
            newInst = false ;
         }
         else
         {
            println( cmd.getLastOut() ) ;
            throw e ;
         }
      }

      try
      {
         // set coord address
         var coordSetting = "sequoiadb_conn_addr=\"" + coordAddr + "\"" ;
         var file = new File( databaseDir + "/auto.cnf" ) ;
         var content = file.read() ;
         content = content.replace( /sequoiadb_conn_addr=(.*)/g, coordSetting ) ;
         content = content.replace( /#sequoiadb_conn_addr/g, "sequoiadb_conn_addr" ) ;
         content = content.replace( /# sequoiadb_conn_addr/g, "sequoiadb_conn_addr" ) ;
         if ( content.indexOf( "sequoiadb_conn_addr=" ) == -1 )
         {
            content = content.replace( /\[mysqld\]/g,
                                       "[mysqld]\n" + coordSetting ) ;
         }
         file.seek( 0 ) ;
         file.write( content ) ;

         // restart instance to make the configuration take effect
         var command = sqlCtl + " restart " + instanceName ;
         cmd.run( command ) ;
      }
      catch( e )
      {
         println( cmd.getLastOut() ) ;
         throw e ;
      }

      println( "Create instance: [name: " + instanceName + ", port: " + port +
               "]" ) ;
   }
}

function deployPostgresql( pgInfo )
{
   println( "\n************ Deploy SequoiaSQL-PostgreSQL ************"  ) ;
   var installedPath = pgInfo[FIELD_PG_INSTALL_PATH] ;
   var allConf = pgInfo[FIELD_PG_INSTANCE_CONF] ;
   var sqlCtl = installedPath + "/bin/sdb_sql_ctl" ;
   var psql = installedPath + "/bin/psql" ;
   var cmd = new Cmd() ;

   // create instance
   var dbName = "foo" ;
   for ( var i in allConf )
   {
      var instanceConf = allConf[i] ;
      var instanceName = instanceConf[0] ;
      var port = instanceConf[1] ;
      var databaseDir = instanceConf[2] ;
      var coordAddr = instanceConf[3] ;
      var newInst = true ;

      try
      {
         // add instance
         var command = sqlCtl + " addinst "+ instanceName +" -D " + databaseDir
                       + " -p " + port ;
         cmd.run( command ) ;
      }
      catch( e )
      {
         var rc = cmd.getLastRet() ;
         if ( rc == 8 ) // 8: instance exist
         {
            newInst = false ;
         }
         else
         {
            println( cmd.getLastOut() ) ;
            throw e ;
         }
      }

      try
      {
         // start instance
         var command = sqlCtl + " start " + instanceName ;
         cmd.run( command ) ;

         // create db
         var command = sqlCtl + " createdb " + dbName + " " + instanceName ;
         cmd.run( command ) ;

         // set coord address
         var envCmd = "export LD_LIBRARY_PATH=" + installedPath + "/lib; " ;
         var command = envCmd + psql + " -p " + port + " " + dbName +
                       " -c \"create extension sdb_fdw\"" ;
         cmd.run( command ) ;

         var command = envCmd + psql + " -p " + port + " " + dbName
                              + " -c \"create server sdb_server foreign "
                              + "data wrapper sdb_fdw options(address '"
                              + coordAddr + "', transaction 'off' );\"" ;
         cmd.run( command ) ;
      }
      catch( e )
      {
         println( cmd.getLastOut() ) ;
         throw e ;
      }

      println( "Create instance: [name: " + instanceName + ", port: " + port +
               "]" ) ;
   }
}
