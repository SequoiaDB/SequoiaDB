/*******************************************************************************
*@description JavaScript common function library
*@Modify list:
*   2014-2-24 Jianhui Xu  Init
*******************************************************************************/

/***********************************************
func.js 中方法：
   1、判断
      判断是否为独立模式     commIsStandalone(db)
      比较结果集             commCompareResults(cursor,expRecs,exceptId)
      判断两个对象是否相等   commCompareObject(expObj,actObj)
      比较错误码是否一致     commCompareErrorCode(e,code)
      判断执行机架构是否为arm  commIsArmArchitecture ()
      
   2、创建
      创建并返回 cs          commCreateCS(db,csName,ignoreExisted,message,options)
      创建并返回 cl          commCreateCL(db,csName,clName,optionObj,autoCreateCS,ignoreExisted,message) 
      创建索引               commCreateIndex(cl,indexName,indexDef,options,ignoreExist)
      创建并启动 group       commCreateRG(db,rgName,nodeNum,hostname,nodeOption)
      在指定主机创建目录     commMakeDir(hostName,dir)
      创建并返回 domain      commCreateDomain(db,domainName,groupNames,options)
      
   3、删除
      删除 cs                commDropCS(db,csName,ignoreNotExist,message,options)   
      删除 cl                commDropCL(db,csName,clName,ignoreCSNotExist,ignoreCLNotExist,message)   
      删除索引               commDropIndex(cl,indexName,ignoreNotExist)   
      删除 domain            commDropDomain(db,domainName,ignoreNotExist)

   4、检查
      检查索引一致性         commCheckIndexConsistency(cl,indexName,exist,timeout)
      commCheckIndexConsistency存在漏洞，建议使用commCheckIndexConsistent
      检查索引一致性         commCheckIndexConsistent ( db, csname, clname, idxname, isExist )
      检查集群状态(retry)    commCheckBusinessStatus(db,timeout,checkLSN)
      检测主备 LSN           commCheckLSN(db,groupNames,timeout)
   
   5、获取
      获取 cl 所属 group     commGetCLGroups(db,fullClName)   
      获取 cl 所在节点       commGetCLNodes(db,fullclName)
      获取 cs 所属 group     commGetCSGroups(db,csName)
      获取 group 详细信息    commGetGroups(db,print,filter,excludeCata,excludeCoord,excludeSpare)
      获取 group 个数        commGetGroupsNum(db)
      获取所有 data group    commGetDataGroupNames(db)
      获取 group 所有节点    commGetGroupNodes(db,groupName)
      获取 backup            commGetBackups(db,filter,path,isSubDir,cond,grpNameArray)
      获取 procedure         commGetProcedures(db,filter)
      获取 sdb 安装路径      commGetInstallPath()
      获取指定快照类型       commGetSnapshot(db,snapshotType,condObj,selObj,sortObj,skipNum,limitNum,optionsObj)
      远程获取集群安装路径    commGetRemoteInstallPath ( cmHostName, cmSvcName )
      
   6、其他
      将游标结果存入数组     commCursor2Array(cursor,fieldName,filter)
      随机生成数据           commDataGenerator()
      封装错误信息           commThrowError(e,msg)
*****************************************************************************************/
import( "../lib/assert.js" );
// begin global variable configuration
//cm端口号
if( typeof ( CMSVCNAME ) == "undefined" ) { CMSVCNAME = "11790"; }
//公共CS前缀
if( typeof ( CHANGEDPREFIX ) == "undefined" ) { CHANGEDPREFIX = "local_test"; }
//协调节点端口号，CI默认11810
if( typeof ( COORDSVCNAME ) == "undefined" ) { COORDSVCNAME = "50000"; }
//编目节点端口号
if( typeof ( CATASVCNAME ) == "undefined" ) { CATASVCNAME = "11800"; }
//协调节点主机名
if( typeof ( COORDHOSTNAME ) == "undefined" ) { COORDHOSTNAME = 'localhost'; }
//用例预留端口最小值
if( typeof ( RSRVPORTBEGIN ) == "undefined" ) { RSRVPORTBEGIN = '26000'; }
//用例预留端口最大值
if( typeof ( RSRVPORTEND ) == "undefined" ) { RSRVPORTEND = '27000'; }
//用例创建节点数据目录
if( typeof ( RSRVNODEDIR ) == "undefined" ) { RSRVNODEDIR = "/opt/sequoiadb/database/"; }
//用例存放临时文件目录
if( typeof ( WORKDIR ) == "undefined" ) { WORKDIR = "/tmp/jstest/"; }
//ES服务端主机名，CI默认传入192.168.28.143
if( typeof ( ESHOSTNAME ) == "undefined" ) { ESHOSTNAME = 'localhost'; }
//ES服务端端口号，CI默认传入9200
if( typeof ( ESSVCNAME ) == "undefined" ) { ESSVCNAME = '9200'; }
//ES全文索引前缀，与工程名相关
if( typeof ( FULLTEXTPREFIX ) == "undefined" ) { FULLTEXTPREFIX = ''; }
if( typeof ( CLEANFORFAIL ) == "undefined" ) { var CLEANFORFAIL = false; }
//数据源端主机名，CI默认传入localhost
if( typeof ( DSHOSTNAME ) == "undefined" ) { DSHOSTNAME = 'localhost'; }
//数据源端端口号，CI默认传入11810
if( typeof ( DSSVCNAME ) == "undefined" ) { DSSVCNAME = '11810'; }
//远程机器用户名
if( typeof ( REMOTEUSER ) == "undefined" ) { REMOTEUSER = "sdbadmin"; }
//远程机器用户密码
if( typeof ( REMOTEPASSWD ) == "undefined" ) { REMOTEPASSWD = "Admin@1024"; }
// CHANGEDPREFIX = local_test

var cmd = new Cmd();
var hostname = cmd.run( "hostname" ).split( "\n" )[0];
hostname = hostname.replace( /-/g, "_" );
var COMMCSNAME = CHANGEDPREFIX + "_" + hostname + "_cs";
var COMMCLNAME = CHANGEDPREFIX + "_cl";
var COMMDUMMYCLNAME = "test_dummy_cl";
//public capped cs
var COMMCAPPEDCSNAME = CHANGEDPREFIX + "_capped_cs";
var COMMCAPPEDCLNAME = CHANGEDPREFIX + "_capped_cl";

var DATA_GROUP_ID_BEGIN = 1000;
var CATALOG_GROUPNAME = "SYSCatalogGroup";
var COORD_GROUPNAME = "SYSCoord";
var DOMAINNAME = "_SYS_DOMAIN_";
var SPARE_GROUPNAME = "SYSSpare";
var CATALOG_GROUPID = 1;
var COORD_GROUPID = 2;
var SPARE_GROUPID = 5;
// end global variable configuration

// control variable
var funcCommDropCSTimes = 0;
var funcCommCreateCSTimes = 0;
var funcCommCreateCLTimes = 0;
var funcCommCreateCLOptTimes = 0;
var funcCommDropCLTimes = 0;
// end control variable

/******************************************************************************
@description 判断集群是否是独立模式
     （请勿直接使用，新用例中用 testConf.skipStandAlone = true 来实现）
@author Jianhui Xu
@return  {boolean}  是否是独立模式
       true   :  是独立模式
       false  :  不是独立模式
***************************************************************************** */
function commIsStandalone ( db ) 
{
   try
   {
      db.listReplicaGroups();
      return false;
   }
   catch( e )
   {
      if( commCompareErrorCode( e, SDB_RTN_COORD_ONLY ) )
      {
         return true;
      }
      else
      {
         throw e;
      }
   }
}

/******************************************************************************
@description 判断当前系统架构是否为arm架构
@author Yongqin Liang
@return  {boolean}  是否是arm架构
       true   :  是arm架构
       false  :  不是arm架构
***************************************************************************** */
function commIsArmArchitecture ()
{
   try
   {
      var arch = cmd.run( 'uname -m' );
      if( arch == "aarch64\n" || arch == "aarch32\n" )
      {
         return true;
      }
      else
      {
         return false;
      }
   }
   catch( e )
   {
      throw new Error( "Fail to check the system architecture:" + e );
   }
}

/******************************************************************************
@description  创建并返回 cs 对象
@author Jianhui Xu
@parameter
   ignoreExisted  {boolean}  :  默认为 false，重复创建 cs 报错
   message        {string}   :  默认为 ""，创建或获取 cs 失败时报错信息
   options        {object}   :  默认为 {}，创建 cs 指定的可选属性
@return  {object}  创建或者获取的 cs 对象
***************************************************************************** */
function commCreateCS ( db, csName, ignoreExisted, message, options )
{
   ++funcCommCreateCSTimes;
   if( ignoreExisted == undefined ) { ignoreExisted = false; }
   if( message == undefined ) { message = ""; }
   if( options == undefined ) { options = {}; }
   commCheckType( ignoreExisted, "boolean" );
   commCheckType( message, "string" );
   commCheckType( options, "object" );

   try
   {
      return db.createCS( csName, options );
   }
   catch( e )
   {
      if( commCompareErrorCode( e, SDB_DMS_CS_EXIST ) && ignoreExisted )
      {
         // think right
      } else
      {
         commThrowError( e, "commCreateCS[" + funcCommCreateCSTimes + "] Create collection space[" + csName + "] failed: " + e + ", message: " + message );
      }
   }
   // get collection space object
   try
   {
      return db.getCS( csName );
   }
   catch( e )
   {
      commThrowError( e, "commCreateCS[" + funcCommCreateCSTimes + "] Get existed collection space[" + csName + "] failed: " + e + ",message: " + message );
   }
}

/******************************************************************************
@description  创建并返回 cl 对象
@author Jianhui Xu
@parameter
   optionObj      {object}   :  默认为 {}，创建 cl 指定的可选属性
   autoCreateCS   {boolean}  :  默认为 true，cs 不存在时自动创建 cs
   ignoreExisted  {boolean}  :  默认为 false，重复创建 cl 报错
   message        {string}   :  默认为 ""，创建或获取 cl 失败时报错信息，（逐步废弃，新用例请勿使用）
@return  {object}  创建或者获取的 cl 对象
***************************************************************************** */
function commCreateCL ( db, csName, clName, optionObj, autoCreateCS, ignoreExisted, message )
{
   ++funcCommCreateCLOptTimes;
   if( optionObj == undefined ) { optionObj = {}; }
   if( autoCreateCS == undefined ) { autoCreateCS = true; }
   if( ignoreExisted == undefined ) { ignoreExisted = false; }
   if( message == undefined ) { message = ""; }
   commCheckType( optionObj, "object" );
   commCheckType( autoCreateCS, "boolean" );
   commCheckType( ignoreExisted, "boolean" );
   commCheckType( message, "string" );

   // try create or get cs 
   var csObj;
   if( autoCreateCS )
   {
      csObj = commCreateCS( db, csName, true, "commCreateCL auto to create collection space" );
   }
   else
   {
      try
      {
         csObj = db.getCS( csName );
      }
      catch( e )
      {
         commThrowError( e, "commCreateCL[" + funcCommCreateCLOptTimes + "] get collection space[" + csName + "] failed: " + e );
      }
   }

   try
   {
      return csObj.createCL( clName, optionObj );
   }
   catch( e )
   {
      if( commCompareErrorCode( e, SDB_DMS_EXIST ) && ignoreExisted )
      {
         // think right
         csObj.dropCL( clName );
         return csObj.createCL( clName, optionObj );
      } else
      {
         commThrowError( e, "commCreateCL[" + funcCommCreateCLOptTimes + "] create collection[" + csName + "." + clName + "] failed: " + e + ",message: " + message );
      }
   }

}

/******************************************************************************
@description  删除 cs
@author Jianhui Xu
@parameter
   ignoreNotExist {boolean}  :  默认为 true，要删除的 cs 不存在时不报错
   message        {string}   :  默认为 ""，删除 cs 失败时报错信息
   options        {object}   :  默认为 {}，删除 cs 指定选项
***************************************************************************** */
function commDropCS ( db, csName, ignoreNotExist, message, options )
{
   ++funcCommDropCSTimes;
   if( ignoreNotExist == undefined ) { ignoreNotExist = true; }
   if( message == undefined ) { message = ""; }
   if( options == undefined ) { options = {}; }
   commCheckType( ignoreNotExist, "boolean" );
   commCheckType( message, "string" );
   commCheckType( options, "object" );

   try
   {
      if( JSON.stringify( options ) == "{}" )
      {
         db.dropCS( csName );
      } else
      {
         db.dropCS( csName, options );
      }

   }
   catch( e )
   {
      if( commCompareErrorCode( e, SDB_DMS_CS_NOTEXIST ) && ignoreNotExist )
      {
         // think right
      }
      else
      {
         commThrowError( e, "commDropCS[" + funcCommDropCSTimes + "] Drop collection space[" + csName + "] failed: " + e + ",message: " + message )
      }
   }
}

/******************************************************************************
@description  删除 cl
@author Jianhui Xu
@parameter
   ignoreCSNotExist   {boolean}  :  默认为 true，要获取的 cs 不存在时不报错
   ignoreCLNotExist   {boolean}  :  默认为 true，要删除的 cl 不存在时不报错
   message            {object}   :  默认为 ""，删除 cl 失败时报错信息吗，（逐步废弃，新用例请勿使用）
***************************************************************************** */
function commDropCL ( db, csName, clName, ignoreCSNotExist, ignoreCLNotExist, message )
{
   ++funcCommDropCLTimes;
   if( message == undefined ) { message = ""; }
   if( ignoreCSNotExist == undefined ) { ignoreCSNotExist = true; }
   if( ignoreCLNotExist == undefined ) { ignoreCLNotExist = true; }
   commCheckType( ignoreCSNotExist, "boolean" );
   commCheckType( ignoreCLNotExist, "boolean" );
   commCheckType( message, "string" );

   try
   {
      db.getCS( csName ).dropCL( clName );
   }
   catch( e )
   {
      if( ( commCompareErrorCode( e, SDB_DMS_CS_NOTEXIST ) && ignoreCSNotExist ) || ( commCompareErrorCode( e, SDB_DMS_NOTEXIST ) && ignoreCLNotExist ) )
      {
         // think right
      }
      else
      {
         commThrowError( e, "commDropCL[" + funcCommDropCLTimes + "] Drop collection[" + csName + "." + clName + "] failed: " + e + ",message: " + message )
      }
   }
}

/******************************************************************************
@description  创建索引
@author Jianhui Xu
@parameter
   indexDef        {object}   :  必填项，索引键
   options         {object}   :  默认为 {}，创建索引指定选项
   ignoreExist     {boolean}  :  默认为 false，创建索引失败报错
***************************************************************************** */
function commCreateIndex ( cl, indexName, indexDef, options, ignoreExist )
{
   if( options == undefined ) { options = {}; }
   if( ignoreExist == undefined ) { ignoreExist = false; }
   commCheckType( options, "object" );
   commCheckType( ignoreExist, "boolean" );

   try
   {
      cl.createIndex( indexName, indexDef, options );
   }
   catch( e )
   {
      if( ignoreExist && ( commCompareErrorCode( e, SDB_IXM_EXIST ) || commCompareErrorCode( e, SDB_IXM_REDEF ) ) )
      {
         // ok
      }
      else
      {
         commThrowError( e, "commCreateIndex: create index[" + indexName + "] failed: " + e );
      }
   }
}

/******************************************************************************
@description  删除索引
@author Jianhui Xu
@parameter
   ignoreNotExist     {boolean}  :  默认为 false，删除索引失败报错
***************************************************************************** */
function commDropIndex ( cl, indexName, ignoreNotExist )
{
   if( ignoreNotExist == undefined ) { ignoreNotExist = false; }
   commCheckType( ignoreNotExist, "boolean" );

   try
   {
      cl.dropIndex( indexName );
   }
   catch( e )
   {
      if( ignoreNotExist && commCompareErrorCode( e, SDB_IXM_NOTEXIST ) )
      {
         // ok
      }
      else
      {
         commThrowError( e, "commDropIndex: drop index[" + indexName + "] failed: " + e );
      }
   }
}

/******************************************************************************
@description  检查索引一致性，在超时时间内所有节点存在/不存在索引通过检测
@author Jianhui Xu
@parameter
   exist     {boolean}  :  默认为 true，检测索引存在
   timeout   {number}   :  默认为 300，检测超时时间
******************************************************************************/
function commCheckIndexConsistency ( cl, indexName, exist, timeout )
{
   if( exist == undefined ) { exist = true; }
   if( timeout == undefined ) { timeout = 300; }
   commCheckType( exist, "boolean" );
   commCheckType( timeout, "number" );

   //cl.toString = hostname:svc.csName.clName
   println( "cl :" + cl.toString() );
   var parts = cl.toString().split( ":" );
   var infoArr = parts[1].split( "." );
   var csName = infoArr[1];
   var clName = infoArr[2];
   var nodes = commGetCLNodes( db, csName + "." + clName );

   var timecount = 0;
   while( true )
   {
      for( var j = 0; j < nodes.length; j++ )
      {
         try
         {
            var nodeConn = new Sdb( nodes[j].HostName, nodes[j].svcname );
            var tmpInfo = nodeConn.getCS( csName ).getCL( clName ).getIndex( indexName );
            nodeConn.close();
         }
         catch( e )
         {
            tmpInfo = undefined;
            break;
         }
         if( tmpInfo != undefined )
         {
            var tmpObj = tmpInfo.toObj();
            if( tmpObj.IndexDef.name != indexName )
            {
               println( "commCheckIndexConsistency: get index name[" + tmpObj.IndexDef.name + "] is not the same with name[" + name + "]" );
               println( tmpInfo );
               throw new Error( "check name error" );
            }
         }
      }
      //var tmpInfo = cl.getIndex( indexName ) ;
      if( ( exist && tmpInfo == undefined ) ||
         ( !exist && tmpInfo != undefined ) )
      {
         if( timecount < timeout )
         {
            ++timecount;
            sleep( 1000 );
            continue;
         }
         throw new Error( "commCheckIndexConsistency: check index[" + indexName + "] time out" );
      }
      break;
   }
}

/*******************************************************************************
@description  获取 cl 所属的 groups
@author  xiaojun Hu
@parameter  
   fullClName    {string}  :  必填项，"csName.clName"
@return  {array}  存放 groupName 的数组
   e.g: 
      ["grou1","group2","group3"]
***************************************************************************** */
function commGetCLGroups ( db, fullClName )
{
   if( typeof ( fullClName ) != "string" || fullClName.length == 0 )
   {
      throw new Error( "commGetCLGroups: Invalid fullClName parameter or fullClName is empty" );
   }

   if( commIsStandalone( db ) )
   {
      return new Array();
   }

   var tmpArray = new Array();
   try
   {
      var cursor = db.snapshot( SDB_SNAP_CATALOG, { Name: fullClName } );
      while( cursor.next() )
      {
         var cataInfo = cursor.current().toObj()['CataInfo'];
         for( var i = 0; i < cataInfo.length; ++i )
         {
            tmpArray.push( cataInfo[i].GroupName );
         }
      }
   }
   catch( e )
   {
      commThrowError( e, "commGetCLGroups: snapshot SDB_SNAP_CATALOG  failed: " + e );
   } finally
   {
      if( cursor != null )
      {
         cursor.close();
      }
   }

   return tmpArray;
}

/******************************************************************************
@description  获取 cl 所在的节点
@author  luweikang
@parameter
   fullClName    {string}  :  必填项，"csName.clName"
@return  {array}   存放节点 HostName 和 ServiceName 的数组；
   e.g:
     array[0] {"HostName": "sdbserver1", "svcname": "11820"}
     array[1] {"HostName": "sdbserver2", "svcname": "11820"}
     ...
******************************************************************************/
function commGetCLNodes ( db, fullclName )
{
   if( typeof ( fullclName ) != "string" || fullclName.length == 0 )
   {
      throw new Error( "commGetCLGroups: Invalid fullclName parameter or fullclName is empty" );
   }

   if( commIsStandalone( db ) )
   {
      return [{ "HostName": COORDHOSTNAME, "svcname": COORDSVCNAME }];
   }

   var clGroups = commGetCLGroups( db, fullclName );
   var nodes = [];
   for( var i = 0; i < clGroups.length; i++ )
   {
      nodes = nodes.concat( commGetGroupNodes( db, clGroups[i] ) );
   }
   return nodes;
}

/******************************************************************************
@description 获取 cs 所属的 group
@author Jianhui Xu
@return  {array}  存放 groupName 的数组
   e.g: 
      ["grou1","group2"]
******************************************************************************/
function commGetCSGroups ( db, csName )
{
   if( typeof ( csName ) != "string" || csName.length == 0 )
   {
      throw new Error( "commGetCSGroups: Invalid csname parameter or csname is empty" );
   }

   if( commIsStandalone( db ) )
   {
      return new Array();
   }

   var tmpArray = [];
   var tmpInfo = commGetSnapshot( db, SDB_SNAP_COLLECTIONSPACES, { Name: csName } );

   for( var i = 0; i < tmpInfo.length; ++i )
   {
      for( var j = 0; j < tmpInfo[i].Group.length; ++j )
      {
         tmpArray.push( tmpInfo[i].Group[j] );
      }
   }
   return tmpArray;
}

/******************************************************************************
@description   获取 group 的详细信息，默认只获取数据组
@author  Jianhui Xu
@parameter 
   print           {boolean}   :    已废弃
   filter          {string}    :    对 GroupName 模糊查询，不指定返回所有查询到的 group
   excludeCata     {boolean}   :    默认为 true，不获取 Catalog group 信息
   excludeCoord    {boolean}   :    默认为 true，不获取 Coord group 信息
   excludeSpare    {boolean}   :    默认为 true，不获取 spare group 信息
@return  {array}
   e.g:
      array[0]
         array[0][0] {"GroupName":"group1","GroupID":1000,"Status":1,"Version":7,"Role":0,"PrimaryNode":1002,"PrimaryPos":3,"Length":3}
         array[0][1] {"HostName":"sdbserver1","dbpath":"/opt/sequoiadb/database/data/11820/","svcname":"11820","NodeID":1000}
         array[0][2] {"HostName":"sdbserver2","dbpath":"/opt/sequoiadb/database/data/11820/","svcname":"11820","NodeID":1001}
         array[0][3] {"HostName":"sdbserver3","dbpath":"/opt/sequoiadb/database/data/11820/","svcname":"11820","NodeID":1002}
      array[1]
         array[1][0] {"GroupName":"group2","GroupID":1001,"Status":1,"Version":7,"Role":0,"PrimaryNode":1005,"PrimaryPos":3,"Length":3}
         array[1][1] {"HostName":"sdbserver1","dbpath":"/opt/sequoiadb/database/data/11830/","svcname":"11830","NodeID":1003}
         array[1][2] {"HostName":"sdbserver2","dbpath":"/opt/sequoiadb/database/data/11830/","svcname":"11830","NodeID":1004}
         array[1][3] {"HostName":"sdbserver3","dbpath":"/opt/sequoiadb/database/data/11830/","svcname":"11830","NodeID":1005}
      ...
***************************************************************************** */
function commGetGroups ( db, print, filter, excludeCata, excludeCoord, excludeSpare )
{
   if( undefined == excludeCata ) { excludeCata = true; }
   if( undefined == excludeCoord ) { excludeCoord = true; }
   if( undefined == excludeSpare ) { excludeSpare = true; }
   commCheckType( excludeCata, "boolean" );
   commCheckType( excludeCoord, "boolean" );
   commCheckType( excludeSpare, "boolean" );

   var tmpArray = [];
   var tmpInfoCur;
   try
   {
      tmpInfoCur = db.listReplicaGroups();
   }
   catch( e )
   {
      if( commCompareErrorCode( e, SDB_RTN_COORD_ONLY ) )
      {
         return tmpArray;
      }
      else
      {
         commThrowError( e, "commGetGroups failed: " + e );
      }
   }
   var tmpInfo = commCursor2Array( tmpInfoCur, "GroupName", filter );

   var index = 0;
   for( var i = 0; i < tmpInfo.length; ++i )
   {
      var tmpObj = tmpInfo[i];
      if( true == excludeCata && tmpObj.GroupID == CATALOG_GROUPID )
      {
         continue;
      }
      if( true == excludeCoord && tmpObj.GroupID == COORD_GROUPID )
      {
         continue;
      }
      if( true == excludeSpare && tmpObj.GroupID == SPARE_GROUPID )
      {
         continue;
      }
      if( tmpObj.Group.length == 0 ) continue;
      tmpArray[index] = Array();
      tmpArray[index][0] = new Object();
      tmpArray[index][0].GroupName = tmpObj.GroupName;         // GroupName
      tmpArray[index][0].GroupID = tmpObj.GroupID;             // GroupID
      tmpArray[index][0].Status = tmpObj.Status;               // Status
      tmpArray[index][0].Version = tmpObj.Version;             // Version
      tmpArray[index][0].Role = tmpObj.Role;                   // Role
      if( typeof ( tmpObj.PrimaryNode ) == "undefined" )
      {
         tmpArray[index][0].PrimaryNode = -1;
      }
      else
      {
         tmpArray[index][0].PrimaryNode = tmpObj.PrimaryNode;     // PrimaryNode
      }
      tmpArray[index][0].PrimaryPos = 0;                       // PrimaryPos

      var tmpGroupObj = tmpObj.Group;
      tmpArray[index][0].Length = tmpGroupObj.length;          // Length

      for( var j = 0; j < tmpGroupObj.length; ++j )
      {
         var tmpNodeObj = tmpGroupObj[j];
         tmpArray[index][j + 1] = new Object();
         tmpArray[index][j + 1].HostName = tmpNodeObj.HostName;           // HostName
         tmpArray[index][j + 1].dbpath = tmpNodeObj.dbpath;               // dbpath
         tmpArray[index][j + 1].svcname = tmpNodeObj.Service[0].Name;     // svcname
         tmpArray[index][j + 1].NodeID = tmpNodeObj.NodeID;

         if( tmpNodeObj.NodeID == tmpObj.PrimaryNode )
         {
            tmpArray[index][0].PrimaryPos = j + 1;                        // PrimaryPos
         }
      }
      ++index;
   }

   return tmpArray;
}

/******************************************************************************
@description  获取 group 个数，默认只获取数据组
@author Jianhua Li
@return  {number}  group 个数
***************************************************************************** */
function commGetGroupsNum ( db )
{
   return commGetGroups( db ).length;
}

/******************************************************************************
@description  获取所有数据组名
@author  luweikang
@return  {array}  数据组
   e.g:
      ["group1","group2","group3"]
***************************************************************************** */
function commGetDataGroupNames ( db )
{
   var groups = commGetGroups( db );
   var groupNames = [];
   for( var i = 0; i < groups.length; i++ )
   {
      groupNames.push( groups[i][0].GroupName );
   }
   return groupNames;
}

/*****************************************************************************
@description  获取指定 group 的所有节点
@author  luweikang
@parameter
   groupName     {string}   :  group name
@return {array}   存放节点 HostName 和 ServiceName 的数组；
   e.g:
     array[0] {"HostName": "sdbserver1", "svcname": "11820","NodeID":"1002"}
     array[1] {"HostName": "sdbserver2", "svcname": "11820","NodeID":"1003"}
     ...
*****************************************************************************/
function commGetGroupNodes ( db, groupName )
{
   if( typeof ( groupName ) != "string" || groupName.length == 0 )
   {
      throw new Error( "commGetGroupNodes: Invalid groupName parameter or groupName is empty" );
   }

   var tmpArray = [];
   var snapshotCur = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
   if( snapshotCur.next() )
   {
      var groupObj = snapshotCur.current().toObj();
   }
   else
   {
      throw new Error( "commGetGroupNodes: failed to get group info, group: " + groupName );
   }

   var nodes = groupObj.Group;
   for( var i = 0; i < nodes.length; i++ )
   {
      var nodeObj = nodes[i];
      tmpArray[i] = {};
      tmpArray[i].HostName = nodeObj.HostName;
      tmpArray[i].svcname = nodeObj.Service[0].Name;
      tmpArray[i].NodeID = nodeObj.NodeID;
   }
   return tmpArray;
}

/******************************************************************************
@description    获取backup
@author  Jianhui Xu
@parameter
   filter         {string}   :  模糊查询backup名
   path           {string}   :  默认为 ""，备份路径
   isSubDir       {string}   :  默认为 false，path 参数配置路径不是配置参数指定的备份路径的子目录
   cond           {string}   :  已废弃，随便传
   grpNameArray   {array}    :  默认为 []，指定备份的 group
@return {array}   返回满足条件的 backup 名
   e.g:
     ["bk1","bk2"]
***************************************************************************** */
function commGetBackups ( db, filter, path, isSubDir, cond, grpNameArray )
{
   if( path == undefined ) { path = ""; }
   if( isSubDir == undefined ) { isSubDir = false; }
   if( grpNameArray == undefined ) { grpNameArray = new Array(); }
   commCheckType( path, "string" );
   commCheckType( isSubDir, "boolean" );
   commCheckType( grpNameArray, "array" );

   var tmpBackup = [];
   var tmpInfoCur;
   try
   {
      if( path.length == 0 )
      {
         tmpInfoCur = db.listBackup( { GroupName: grpNameArray } );
      }
      else
      {
         tmpInfoCur = db.listBackup( { Path: path, IsSubDir: isSubDir, GroupName: grpNameArray } );
      }
   }
   catch( e )
   {
      println( "commGetBackups failed: " + e );
      return tmpBackup;
   }
   var tmpInfo = commCursor2Array( tmpInfoCur, "Name", filter );

   var exists = [];
   for( var i = 0; i < tmpInfo.length; ++i )
   {
      var tmpBackupObj = tmpInfo[i];
      if( exists[tmpBackupObj.Name] == undefined )
      {
         tmpBackup.push( tmpBackupObj.Name );
         exists[tmpBackupObj.Name] = "exist";
      }

   }
   return tmpBackup;
}

/******************************************************************************
@description  获取 procedure
@author  Jianhui Xu
@parameter
   filter    {string}   :   模糊查询 procedure 名
@return   {array}   返回满足条件的 procedure 名
   e.g:
      ["local_test_createCSAndCL","local_test_insertRecord"]
***************************************************************************** */
function commGetProcedures ( db, filter )
{
   var tmpProcedure = [];
   var tmpInfoCur;
   try
   {
      tmpInfoCur = db.listProcedures();
   }
   catch( e )
   {
      println( "commGetProcedures failed: " + e );
      return tmpProcedure;
   }
   var tmpInfo = commCursor2Array( tmpInfoCur, "name", filter );

   for( var i = 0; i < tmpInfo.length; ++i )
   {
      var tmpProcedureObj = tmpInfo[i];
      tmpProcedure.push( tmpProcedureObj.name );
   }
   return tmpProcedure;
}

/******************************************************************************
@description  多次检查 data group 和 catalog group 状态（包括检测连接、是否有主、服务是否可用，检测 LSN 可选），超时检测不过报错
         此处检测 LSN 的逻辑是：假设主 LSN 为 A，备 LSN 为 B，多次更新 A、B，检测 A、B 是否相等
@author  Jianhui Xu
@parameter
   timeout           {number}   :   默认为 120s，超时时间
   checkLSN          {boolean}  :   默认为 true，检测 LSN
***************************************************************************** */
function commCheckBusinessStatus ( db, timeout, checkLSN )
{
   if( checkLSN == undefined ) { checkLSN = true; }
   if( timeout == undefined ) { timeout = 120 }
   commCheckType( checkLSN, "boolean" );
   commCheckType( timeout, "number" );

   var time = 0;
   for( var i = 1; i <= timeout; i++ )
   {
      try
      {
         rg = db.getRG( "SYSCatalogGroup" );
         break;
      } catch( e )
      {
         if( !commCompareErrorCode( e, SDB_CLS_NOT_PRIMARY ) )
         {
            throw e;
         }
      }
      sleep( 1000 );
      time++;
   }

   if( time == timeout )
   {
      throw new Error( "checkout the cluster state timeout,check failed reason: " +
         "exec db.getRG( \"SYSCatalogGroup\" ) failed" );
   }

   for( var i = time; i <= timeout; i++ )
   {
      // data and cata group
      var groups = commGetGroups( db, false, "", false );
      var tmpArr = commCheckBusiness( groups, checkLSN );
      if( tmpArr.length == 0 )
      {
         break;
      }
      else if( i < timeout )
      {
         time++;
         sleep( 1000 );
      }
      else
      {
         throw new Error( "check the cluster state timeout, check failed nodes: "
            + JSON.stringify( tmpArr, "", 1 ) );
      }
   }

   // LSN 已校验通过，校验DiffLSNWithPrimary不为-1，超时不报错，连续校验通过5次退出
   var passNum = 0;
   for( var i = time; i <= timeout; i++ )
   {
      var allDiffLSNWithPrimary = [];
      var cursor = db.snapshot( SDB_SNAP_HEALTH, { Role: "data" }, { DiffLSNWithPrimary: "" } );
      while( cursor.next() )
      {
         var obj = cursor.current().toObj();
         allDiffLSNWithPrimary.push( obj.DiffLSNWithPrimary );
      }
      cursor.close();
      if( allDiffLSNWithPrimary.indexOf( -1 ) == -1 )
      {
         passNum++;
         sleep( 1000 );
         time++;
      }
      else
      {
         passNum = 0;
         sleep( 1000 );
         time++;
      }
      if( passNum > 5 )
      {
         break;
      }
   }
}

/******************************************************************************
@description  检查集群状态（包括检测连接、是否有主、服务是否可用，检测 LSN 可选）
              只检测 1 次集群状态，并返回故障的节点信息
              如果需要循环多次检查集群环境，请使用 commCheckBusinessStatus
@author  Jianhui Xu
@parameter
   groups            {array}    :   从 commGetGroups 方法中取到的 group 信息
   checkLSN          {boolean}  :   默认为 false，不检测 LSN
@return  {array}
   e.g:
      array[0]
         array[0][0]  {"GroupName":"group1","GroupID":1000,"PrimaryNode":1001,"ConnCheck":true,"PrimaryCheck":true,"LSNCheck":false,"ServiceCheck":true}
         array[0][1]  {"HostName":"lyysdbserver1","svcname":"11820","NodeID":1000,"Connect":true,"IsPrimay":false,"LSN":9484,"ServiceStatus":true,"FreeSpace":-1}
         array[0][2]  {"HostName":"lyysdbserver2","svcname":"11820","NodeID":1001,"Connect":true,"IsPrimay":true,"LSN":9484,"ServiceStatus":true,"FreeSpace":-1}
         array[0][3]  {"HostName":"lyysdbserver3","svcname":"11820","NodeID":1002,"Connect":true,"IsPrimay":false,"LSN":9300,"ServiceStatus":true,"FreeSpace":-1}
      array[1]
         array[1][0]  {"GroupName":"group2","GroupID":1001,"PrimaryNode":1005,"ConnCheck":false,"PrimaryCheck":true,"LSNCheck":true,"ServiceCheck":true}
         array[1][1]  {"HostName":"lyysdbserver1","svcname":"11830","NodeID":1003,"Connect":true,"IsPrimay":false,"LSN":9256,"ServiceStatus":true,"FreeSpace":-1}
         array[1][2]  {"HostName":"lyysdbserver2","svcname":"11830","NodeID":1004,"Connect":false,"IsPrimay":false,"LSN":-1,"ServiceStatus":true,"FreeSpace":-1}
         array[1][3]  {"HostName":"lyysdbserver3","svcname":"11830","NodeID":1005,"Connect":true,"IsPrimay":true,"LSN":9256,"ServiceStatus":true,"FreeSpace":-1}
      ...
***************************************************************************** */
function commCheckBusiness ( groups, checkLSN )
{
   if( checkLSN == undefined ) { checkLSN = false; }
   commCheckType( checkLSN, "boolean" );

   var tmpCheckGrp = new Array();
   var grpindex = 0;
   var ndindex = 1;
   var error = false;

   for( var i = 0; i < groups.length; ++i )
   {
      error = false;

      tmpCheckGrp[grpindex] = new Array();
      tmpCheckGrp[grpindex][0] = new Object();
      tmpCheckGrp[grpindex][0].GroupName = groups[i][0].GroupName;
      tmpCheckGrp[grpindex][0].GroupID = groups[i][0].GroupID;
      tmpCheckGrp[grpindex][0].PrimaryNode = groups[i][0].PrimaryNode;
      tmpCheckGrp[grpindex][0].ConnCheck = true;
      tmpCheckGrp[grpindex][0].PrimaryCheck = true;
      tmpCheckGrp[grpindex][0].LSNCheck = true;
      tmpCheckGrp[grpindex][0].ServiceCheck = true;

      // if no primary
      if( groups[i][0].PrimaryNode == -1 )
      {
         error = true;
         tmpCheckGrp[grpindex][0].PrimaryCheck = false;
      }

      for( var j = 1; j < groups[i].length; ++j )
      {
         ndindex = j;
         tmpCheckGrp[grpindex][ndindex] = new Object();
         tmpCheckGrp[grpindex][ndindex].HostName = groups[i][j].HostName;
         tmpCheckGrp[grpindex][ndindex].svcname = groups[i][j].svcname;
         tmpCheckGrp[grpindex][ndindex].NodeID = groups[i][j].NodeID;
         tmpCheckGrp[grpindex][ndindex].Connect = true;
         tmpCheckGrp[grpindex][ndindex].IsPrimay = false;
         tmpCheckGrp[grpindex][ndindex].LSN = -1;
         tmpCheckGrp[grpindex][ndindex].ServiceStatus = true;
         tmpCheckGrp[grpindex][ndindex].FreeSpace = -1;

         var tmpDB;
         // check connection
         try
         {
            tmpDB = new Sdb( groups[i][j].HostName, groups[i][j].svcname );
         }
         catch( e )
         {
            error = true;
            tmpCheckGrp[grpindex][0].ConnCheck = false;
            tmpCheckGrp[grpindex][ndindex].Connect = false;
            continue;
         }
         // check primary and lsn
         var tmpSysInfoCur;
         try
         {
            tmpSysInfoCur = tmpDB.snapshot( SDB_SNAP_DATABASE );
         }
         catch( e )
         {
            error = true;
            tmpCheckGrp[grpindex][0].ConnCheck = false;
            tmpCheckGrp[grpindex][ndindex].Connect = false;
            tmpDB.close();
            continue;
         }
         var tmpSysInfo = commCursor2Array( tmpSysInfoCur );
         //delete tmpCheckGrp[grpindex][ndindex].Connect ;
         // check primary
         var tmpSysInfoObj = tmpSysInfo[0];
         if( tmpSysInfoObj.IsPrimary == true )
         {
            tmpCheckGrp[grpindex][ndindex].IsPrimay = true;
            if( groups[i][0].PrimaryNode != groups[i][j].NodeID )
            {
               error = true;
               tmpCheckGrp[grpindex][0].PrimaryCheck = false;
            }
         }
         else
         {
            if( groups[i][0].PrimaryNode == groups[i][j].NodeID )
            {
               error = true;
               tmpCheckGrp[grpindex][0].PrimaryCheck = false;
            }
            //delete tmpCheckGrp[grpindex][ndindex].IsPrimay ;
         }
         // check ServiceStatus
         if( tmpSysInfoObj.ServiceStatus == false )
         {
            error = true;
            tmpCheckGrp[grpindex][ndindex].ServiceStatus = false;
            tmpCheckGrp[grpindex][0].ServiceCheck = false;
         }
         // check LSN
         tmpCheckGrp[grpindex][ndindex].LSN = tmpSysInfoObj.CurrentLSN.Offset;
         if( checkLSN && j > 1 && tmpCheckGrp[grpindex][ndindex].LSN != tmpCheckGrp[grpindex][1].LSN )
         {
            error = true;
            tmpCheckGrp[grpindex][0].LSNCheck = false;
         }

         tmpDB.close();
      }

      if( error == true )
      {
         ++grpindex;
      }
   }
   // no error
   if( error == false )
   {
      if( grpindex == 0 )
      {
         tmpCheckGrp = new Array();
      }
      else
      {
         delete tmpCheckGrp[grpindex];
      }
   }
   return tmpCheckGrp;
}

/******************************************************************************
@description  检测主备 LSN 是否一致
              假设主节点 LSN 为 A，备节点 LSN 为 B，多次更新 B，当 B 大于等于 A，检测通过
@author  luweikang
@parameter 
   groupNames      {array}   :   默认为所有 data group 和 catalog group 名，group 名
   timeout         {number}  :   默认为 60s，超时时间
***************************************************************************** */
function commCheckLSN ( db, groupNames, timeout )
{
   if( groupNames == undefined ) 
   {
      groupNames = commGetDataGroupNames( db );
      groupNames.push( "SYSCatalogGroup" );
   }
   if( timeout == undefined ) { timeout = 60; }
   if( typeof ( groupNames ) == "string" ) { groupNames = [groupNames]; }
   commCheckType( timeout, "number" );
   commCheckType( groupNames, "array" );


   for( var i = 0; i < groupNames.length; i++ )
   {
      var groupName = groupNames[i];
      var masterSnapshot = commGetSnapshot( db, SDB_SNAP_SYSTEM, { GroupName: groupName, RawData: true, "IsPrimary": true } );
      if( masterSnapshot.length == 0 )
      {
         println( db.snapshot( SDB_SNAP_SYSTEM, { GroupName: groupName, RawData: true } ) );
         throw new Error( "check group failed: group can't found primary node, the console view detailed snapshot info" );
      }

      var masterObj = masterSnapshot[0];
      var masterCompleteLSN = masterObj.CompleteLSN;

      var time = 0;
      while( true )
      {
         var success = true;
         var slaveSnapshot = commGetSnapshot( db, SDB_SNAP_SYSTEM, { GroupName: groupName, RawData: true, "IsPrimary": false } );
         //no need to detect consistency without slave node
         if( slaveSnapshot.length == 0 )
         {
            break;
         }

         for( var j = 0; j < slaveSnapshot.length; j++ )
         {
            var slaveObj = slaveSnapshot[j];
            if( slaveObj.CompleteLSN < masterCompleteLSN )
            {
               success = false;
               break;
            }
         }

         if( success )
         {
            break;
         }
         else if( time === timeout )
         {
            println( "master snapshot: " + JSON.stringify( masterSnapshot, "", 1 ) );
            println( "slave snapshot: " + JSON.stringify( slaveSnapshot, "", 1 ) );
            throw new Error( "check catalog failed: the standby node is not consistency after " + timeout + "consistency, see console snapshot detailed" )
         }
         else
         {
            time++;
            sleep( 1000 );
         }
      }
   }
}

/******************************************************************************
@description  获取 sdb 安装路径
@author  xiaojun Hu
@return  {string}  sdb 安装路径
   e.g:
       "/opt/sequoiadb"
***************************************************************************** */
function commGetInstallPath ()
{
   try
   {
      var cmd = new Cmd();
      try
      {
         var installFile = cmd.run( "grep INSTALL_DIR /etc/default/sequoiadb" ).split( "=" );
         var installPath = installFile[1].split( "\n" );
         var InstallPath = installPath[0];
      }
      catch( e )
      {
         if( commCompareErrorCode( e, 2 ) )
         {
            var local = cmd.run( "pwd" ).split( "\n" );
            var LocalPath = local[0];
            var folder = cmd.run( 'ls ' + LocalPath ).split( '\n' );
            var fcnt = 0;
            for( var i = 0; i < folder.length; ++i )
            {
               if( "bin" == folder[i] || "SequoiaDB" == folder[i] ||
                  "testcase" == folder[i] )
               {
                  fcnt++;
               }
            }
            if( 2 <= fcnt )
            {
               InstallPath = LocalPath;
            } else
            {
               throw new Error( "Don'tGetLocalPath" );
            }
         }
         else
         {
            commThrowError( e, "Don'tGetLocalPath" );
         }
      }
   }
   catch( e )
   {
      commThrowError( e, "failed to get install path[common]: " + e );
   }
   return InstallPath;
}

/******************************************************************************
@description  远程获取 sdb 安装路径
@author  liuli
@return  {string}  sdb 安装路径
   e.g:
       "/opt/sequoiadb"
***************************************************************************** */
function commGetRemoteInstallPath ( cmHostName, cmSvcName )
{
   try
   {
      var remoteObj = new Remote( cmHostName, cmSvcName );
      var cmd = remoteObj.getCmd();
      try
      {
         var installFile = cmd.run( "grep INSTALL_DIR /etc/default/sequoiadb" ).split( "=" );
         var installPath = installFile[1].split( "\n" );
         var InstallPath = installPath[0];
      }
      catch( e )
      {
         if( commCompareErrorCode( e, 2 ) )
         {
            var local = cmd.run( "pwd" ).split( "\n" );
            var LocalPath = local[0].replace( "/bin", "" );
            var folder = cmd.run( 'ls ' + LocalPath ).split( '\n' );
            var fcnt = 0;
            for( var i = 0; i < folder.length; ++i )
            {
               if( "bin" == folder[i] || "SequoiaDB" == folder[i] ||
                  "testcase" == folder[i] || "conf" == folder[i] )
               {
                  fcnt++;
               }
            }
            if( 2 <= fcnt )
            {
               InstallPath = LocalPath;
            } else
            {
               throw new Error( "Don'tGetLocalPath" );
            }
         }
         else
         {
            commThrowError( e, "Don'tGetLocalPath" );
         }
      }
   }
   catch( e )
   {
      commThrowError( e, "failed to get install path[common]: " + e );
   }
   finally
   {
      remoteObj.close();
   }
   return InstallPath;
}

/******************************************************************************
@description  构建异常信息（逐步废弃，新用例请勿使用）
@author  wenjing wang
@parameter 
   funname    {string}  :   函数名称
   e          {Error}   :   错误信息
   operate    {string}  :   操作信息
   expectval  {string}  :   期望值
   realval    {}        :   实际值
@return  {string}  构建出的错误信息
***************************************************************************** */
function buildException ( funname, e, operate, expectval, realval )
{
   if( undefined != operate &&
      undefined != expectval &&
      undefined != realval )
   {
      var message = "Exec " + operate + " \nExpect result: " + expectval + " \nReal result: " + realval;
      return funname + " throw: " + message;
   }
   else
   {
      return funname + " unknown error expect: " + e;
   }
}

/*********************************************************************
@description  比较结果集，不一致抛错
@author luweikang
@parameter
   cursor      {object}   :   必填项，游标对象
   expRecs     {array}    :   期望结果集  
   exceptId    {boolean}  :   默认为 true，不考虑 _id
********************************************************************* */
function commCompareResults ( cursor, expRecs, exceptId )
{
   if( exceptId == undefined ) { exceptId = true; }
   commCheckType( exceptId, "boolean" );

   var actRecs = [];
   var pos = 0;
   var isSuccess = true;
   var posOfFailure;
   var isLong = false;

   try
   {
      while( cursor.next() )
      {
         var expRecord = null;
         if( pos < expRecs.length )
         {
            expRecord = expRecs[pos++];
         }

         var actRecord = cursor.current().toObj();
         if( actRecord._id != undefined && exceptId )
         {
            delete actRecord._id;
         }

         if( isSuccess && !commCompareObject( expRecord, actRecord ) )
         {
            if( isSuccess ) posOfFailure = pos - 1;
            isSuccess = false;
            if( JSON.stringify( actRecord ).length > 1024 )
            {
               isLong = true;
            }
         }
         actRecs.push( actRecord );
      }
      if( actRecs.length !== expRecs.length )
      {
         isSuccess = false;
         pos = actRecs.length > expRecs.length ? expRecs.length : actRecs.length;
         posOfFailure = pos;
         if( actRecs.length != 0 && JSON.stringify( actRecs[posOfFailure] ).length > 1024 )
         {
            isLong = true;
         }
      }
   }
   catch( e )
   {
      commThrowError( e );
   }
   finally
   {
      if( cursor !== undefined )
      {
         cursor.close();
      }
   }

   var recordLocation = posOfFailure + 1;
   if( !isSuccess )
   {
      if( isLong )
      {
         var expStr = posOfFailure < expRecs.length ? JSON.stringify( expRecs[posOfFailure] ) : "";
         var actStr = posOfFailure < actRecs.length ? JSON.stringify( actRecs[posOfFailure] ) : "";
         throw new Error( "compare the " + recordLocation + "th record failed, "
            + "\nexp record count: " + expRecs.length
            + "\nact record count: " + actRecs.length
            + "\nexp record: " + expStr
            + "\nact record: " + actStr );
      }
      else
      {
         var bpos = posOfFailure - 10 > 0 ? posOfFailure - 10 : 0;
         var epos = posOfFailure + 10;
         var expRecords = expRecs.slice( bpos, epos );
         var actRecords = actRecs.slice( bpos, epos );
         throw new Error( "compare the " + recordLocation + "th record failed, "
            + "\nexp record count: " + expRecs.length
            + "\nact record count: " + actRecs.length
            + "\nexp: " + JSON.stringify( expRecords )
            + "\nact: " + JSON.stringify( actRecords ) );
      }
   }
}

/********************************************************************
@description   判断两个对象是否相等
@author  luweikang
@return  {boolean}
   e.g:
      true   expObj 与 actObj 相等
      false  expObj 与 actObj 不相等
******************************************************************* */
function commCompareObject ( expObj, actObj )
{
   function isDirectCompare ( value )
   {
      if( typeof ( value ) !== "object" )
      {
         return true;
      }
      else if( value == null )  // null and undefined
      {
         return true;
      }
      else if( value.constructor === Date )
      {
         return true;
      }
      else
      {
         return false;
      }
   }

   if( typeof ( expObj ) != typeof ( actObj ) || isDirectCompare( expObj ) != isDirectCompare( actObj ) )
   {
      return expObj == actObj;
   }
   if( isDirectCompare( actObj ) )
   {
      if( typeof ( actObj ) === "number" && isNaN( actObj ) )
      {
         if( typeof ( actObj ) === "number" )
         {
            return isNaN( expObj );
         }
         return false;
      }
      return expObj === actObj;
   }
   else
   {
      if( Object.keys( expObj ).length != Object.keys( actObj ).length )
      {
         return false;
      }
      for( var key in expObj )
      {
         if( !commCompareObject( expObj[key], actObj[key] ) )
         {
            return false;
         }
      }
   }
   return true;
}

/*********************************************************************
@description  获取指定快照信息
@author  luweikang
@parameter
   snapshotType     {enum}    :   必填项，快照类型
   condObj          {object}  :   默认为 {}，匹配条件
   selObj           {object}  :   默认为 {}，返回字段
   sortObj          {object}  :   默认为 {}，排序字段
   skipNum          {number}  :   默认为 0，开始返回记录
   limitNum         {number}  :   默认为 -1，记录返回条数
   optionsObj       {object}  :   默认为 {}，快照参数
@return  {array}   快照信息
   e.g:
      array[0]  {"xxxx": "xxxx", "xxxx": "xxxx"}
      array[1]  {"xxxx": "xxxx", "xxxx": "xxxx"}
      ...
********************************************************************* */
function commGetSnapshot ( db, snapshotType, condObj, selObj, sortObj, skipNum, limitNum, optionsObj )
{
   if( condObj == undefined ) { condObj = {}; }
   if( selObj == undefined ) { selObj = {}; }
   if( sortObj == undefined ) { sortObj = {}; }
   if( skipNum == undefined ) { skipNum = 0; }
   if( limitNum == undefined ) { limitNum = -1; }
   if( optionsObj == undefined ) { optionsObj = {}; }
   commCheckType( condObj, "object" );
   commCheckType( selObj, "object" );
   commCheckType( sortObj, "object" );
   commCheckType( skipNum, "number" );
   commCheckType( limitNum, "number" );
   commCheckType( optionsObj, "object" );

   var snapshotOption = new SdbSnapshotOption();
   snapshotOption.cond( condObj ).sel( selObj ).options( optionsObj );
   snapshotOption.sort( sortObj ).skip( skipNum ).limit( limitNum );

   var cursor = db.snapshot( snapshotType, snapshotOption );
   var tmpArr = commCursor2Array( cursor );

   return tmpArr;
}

/*********************************************************************
@description  遍历游标，将查询结果或模糊查询匹配项存入数组
@author  luweikang
@parameter
   cursor      {object}   :   必填项，游标对象
   fieldName   {string}   :   可选项，模糊查询的属性
   filter      {string}   :   可选项，模糊查询匹配的值
@return  {array}   将游标对象遍历（且过滤）的值存入数组
   e.g:
      array[0]  [object Object]
      array[1]  [object Object]
      ...
********************************************************************* */
function commCursor2Array ( cursor, fieldName, filter )
{
   commCheckType( cursor, "object" );

   var tmpArray = [];
   try
   {
      while( cursor.next() )
      {
         var obj = cursor.current().toObj();
         if( fieldName !== undefined && filter !== undefined )
         {
            if( obj[fieldName] !== undefined && obj[fieldName].indexOf( filter ) !== -1 )
            {
               tmpArray.push( obj );
            }
         }
         else
         {
            tmpArray.push( obj );
         }
      }
   }
   catch( e )
   {
      commThrowError( e );
   }
   finally
   {
      if( cursor != null )
      {
         cursor.close();
      }
   }

   return tmpArray;
}

/********************************************************************
@description  创建并启动 group，group 中创建指定个数 node
            创建的 node 端口号为 26000 26100 ..
@author  luweikang
@parameter
   rgName        {string}    :    必填项，分区组名
   nodeNum       {number}    :    必填项，group 中 node 个数
   hostname      {string}    :    默认为 coord 节点，主机名
   nodeOption    {object}    :    默认开启 debug 日志，节点配置信息
@return    {array}   节点信息
   e.g:
      array[0] {"hostname":"lyysdbserver1","svcname":26000,"logpath":"lyysdbserver1:11790@/opt/sequoiadb/database/data/26000/diaglog/sdbdiag.log"}
      ...
******************************************************************* */
function commCreateRG ( db, rgName, nodeNum, hostname, nodeOption )
{
   if( hostname === undefined )
   {
      var nodeList = commGetSnapshot( db, SDB_SNAP_SYSTEM, { Role: "coord", RawData: true } );
      hostname = nodeList[0].HostName;
   }
   if( nodeOption === undefined )
   {
      nodeOption = { diaglevel: 5 };
   }
   commCheckType( hostname, "string" );
   commCheckType( nodeOption, "object" );

   try
   {
      var rg = db.createRG( rgName );
   }
   catch( e )
   {
      throw new Error( e );
   }

   var maxRetryTimes = 100;
   var nodeInfos = [];
   for( var i = 0; i < nodeNum; i++ )
   {
      var failedCount = 0;
      var svc = parseInt( RSRVPORTBEGIN ) + 10 * ( i + failedCount );
      var dbPath = RSRVNODEDIR + "data/" + svc;
      do
      {
         try
         {
            new Remote( hostname ).getCmd().run( "lsof -i:" + svc );
            svc = svc + 10;
            dbPath = RSRVNODEDIR + "data/" + svc;
            failedCount++;
            continue;
         }
         catch( e )
         {
            if( !commCompareErrorCode( e, 1 ) )
            {
               throw new Error( "lsof check port error: " + e );
            }
         }
         try
         {
            rg.createNode( hostname, svc, dbPath, nodeOption );
            println( "create node: " + hostname + ":" + svc + " dbpath: " + dbPath );
            var nodeInfo = { "hostname": hostname, "svcname": svc, "logpath": hostname + ":" + CMSVCNAME + "@" + dbPath + "/diaglog/sdbdiag.log" };
            nodeInfos.push( nodeInfo );
            break;
         }
         catch( e )
         {
            //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
            if( commCompareErrorCode( e, SDBCM_NODE_EXISTED ) || commCompareErrorCode( e, SDB_DIR_NOT_EMPTY ) )
            {
               svc = svc + 10;
               dbPath = RSRVNODEDIR + "data/" + svc;
               failedCount++;
            }
            else
            {
               throw new Error( "create node failed!  port = " + svc + " dataPath = " + dbPath + " errorCode: " + e );
            }
         }
      } while( failedCount < maxRetryTimes );
   }
   rg.start();
   return nodeInfos;
}

/**********************************************************************
@description  随机生成指定类型的数据，包括 int、long、float、string、OID、bool、date、timestamp、binary、regex、object、array、null
@author  Ting YU
@function getRecords
   @parameter 
      recNum         {number}           :    必填项，指定随机生成的数据数
      dataTypes      {string | array}   :    必填项，指定随机生成数据的类型，如果传入类型为数组，每条数据随机选择数组中数据类型生成
      fieldNames     {array}            :    默认值为随机长度数组，该数组中每个值由'a'~'z'随机长度的字符串的组成，指定随机生成数据的属性名
   @returns    {array}   随机生成的数据
      e.g:
         当执行： 
            var rd = new commDataGenerator();
            rd.getRecords( 30, "int", ['a','b'] );
         返回值为：
            array[0]  {"a":-353903809,"b":1620652070}
            array[1]  {"a":1708939799,"b":1730495500}
            ...
            array[29] {"a":-324168767,"b":481875392}
@function getValue
   @parameter
      dataType       {string}     :     必填项，指定随机生成数据的类型
   @returns     {any}   根据指定类型生成的数据
      e.g：
         当执行：
            var rd = new commDataGenerator();
            rd.getValue("string");
         返回值为：
            "`\PjHqMu:`Z^fV"
***********************************************************************/
function commDataGenerator ()
{
   this.getRecords =
      function( recNum, dataTypes, fieldNames )
      {
         return getRandomRecords( recNum, dataTypes, fieldNames );
      }

   this.getValue =
      function( dataType )
      {
         return getRandomValue( dataType );
      }

   function getRandomRecords ( recNum, dataTypes, fieldNames )
   {
      if( fieldNames === undefined ) { fieldNames = getRandomFieldNames(); }
      if( !Array.isArray( dataTypes ) ) { dataTypes = [dataTypes]; }
      commCheckType( dataTypes, "array" );
      commCheckType( fieldNames, "array" );

      var recs = [];
      for( var i = 0; i < recNum; i++ )
      {
         // generate 1 record
         var rec = {};
         for( var j in fieldNames )
         {
            // generate 1 filed
            var filedName = fieldNames[j];

            var dataType = dataTypes[parseInt( Math.random() * dataTypes.length )];  //randomly get 1 data type
            var filedVal = getRandomValue( dataType );

            if( filedVal !== undefined ) rec[filedName] = filedVal;
         }

         recs.push( rec );
      }
      return recs;
   }

   function getRandomValue ( dataType )
   {
      var value = undefined;

      switch( dataType )
      {
         case "int":
            value = getRandomInt( -2147483648, 2147483647 );
            break;
         case "long":
            value = getRandomLong( -922337203685477600, 922337203685477600 );
            break;
         case "float":
            value = getRandomFloat( -999999, 999999 );
            break;
         case "string":
            value = getRandomString( 0, 20 );
            break;
         case "OID":
            value = ObjectId();
            break;
         case "bool":
            value = getRandomBool();
            break;
         case "date":
            value = getRandomDate();
            break;
         case "timestamp":
            value = getRandomTimestamp();
            break;
         case "binary":
            value = getRandomBinary();
            break;
         case "regex":
            value = getRandomRegex();
            break;
         case "object":
            value = getRandomObject();
            break;
         case "array":
            value = getRandomArray();
            break;
         case "null":
            value = null;
            break;
         case "non-existed":
            break;
      }

      return value;
   }

   function getRandomFieldNames ( minNum, maxNum )
   {
      if( minNum == undefined ) { minNum = 0; }
      if( maxNum == undefined ) { maxNum = 16; }

      var fieldNames = [];
      var fieldNum = getRandomInt( minNum, maxNum );

      for( var i = 0; i < fieldNum; i++ )
      {
         //get 1 field name
         var fieldName = "";
         var fieldNameLen = getRandomInt( 1, 9 );
         for( var j = 0; j < fieldNameLen; j++ )
         {
            //get 1 char
            var ascii = getRandomInt( 97, 123 ); // 'a'~'z'
            var c = String.fromCharCode( ascii );
            fieldName += c;
         }
         fieldNames.push( fieldName );
      }

      return fieldNames;
   }

   function getRandomInt ( min, max ) // [min, max)
   {
      var range = max - min;
      var value = min + parseInt( Math.random() * range );
      return value;
   }

   function getRandomLong ( min, max )
   {
      var longValue = getRandomInt( min, max );
      var value = { "$numberLong": longValue.toString() };
      return value;
   }

   function getRandomFloat ( min, max )
   {
      var range = max - min;
      var value = min + Math.random() * range;
      return value;
   }

   function getRandomString ( minLen, maxLen ) //string length value locate in [minLen, maxLen)
   {
      var strLen = getRandomInt( minLen, maxLen );
      var str = "";

      for( var i = 0; i < strLen; i++ )
      {
         var ascii = getRandomInt( 48, 127 ); // '0' -- '~'
         var c = String.fromCharCode( ascii );
         str += c;
      }
      return str;
   }

   function getRandomBool ()
   {
      var Bools = [true, false];
      var index = parseInt( Math.random() * Bools.length );
      var value = Bools[index];

      return value;
   }

   function getRandomDate ()
   {
      var sec = getRandomInt( -2208902400, 253402128000 ); //1900-01-02 ~ 9999-12-30
      var dateVal = new Date( sec * 1000 ).Format( "yyyy-MM-dd" );
      var value = { "$date": dateVal };
      return value;
   }

   function getRandomTimestamp ()
   {
      var sec = getRandomInt( -2147397248, 2147397247 ); //1901-12-14-20.45.52 ~ 2038-01-18-03.14.07
      var d = new Date( sec * 1000 ).Format( "yyyy-MM-dd-hh.mm.ss" );

      var ns = getRandomInt( 0, 1000000 ).toString();
      if( ns.length < 6 )
      {
         var addZero = 6 - ns.length;
         for( var i = 0; i < addZero; i++ ) { ns = '0' + ns; }
      }

      var timeVal = d + '.' + ns;

      var value = { "$timestamp": timeVal };
      return value;
   }

   function getRandomBinary ()
   {
      var str = getRandomString( 1, 50 );

      var cmd = new Cmd();
      var binaryVal = cmd.run( "echo -n '" + str + "' | base64 -w 0" );

      var typeVal = getRandomInt( 0, 256 );
      typeVal = typeVal.toString();

      var value = { "$binary": binaryVal, "$type": typeVal };
      return value;
   }

   function getRandomRegex ()
   {
      var opts = ["i", "m", "x", "s"];
      var index = parseInt( Math.random() * opts.length );
      var optVal = opts[index];

      var regexVal = getRandomString( 1, 10 );

      var value = { "$regex": regexVal, "$options": optVal };
      return value;
   }

   function getRandomObject ( n )
   {
      var obj = {};
      var dataTypes = ["int", "long", "float", "string", "OID", "bool", "date",
         "timestamp", "binary", "regex", "object", "array", "null"];

      var fieldNames = getRandomFieldNames( 1, 5 );

      for( var i in fieldNames )
      {
         var dataType = dataTypes[parseInt( Math.random() * dataTypes.length )];  //randomly get 1 data type 

         var filedName = fieldNames[i]
         obj[filedName] = getRandomValue( dataType );
      }

      return obj;
   }

   function getRandomArray ()
   {
      var arr = [];
      var dataTypes = ["int", "long", "float", "string", "OID", "bool", "date",
         "timestamp", "binary", "regex", "object", "array", "null"];

      var arrLen = getRandomInt( 1, 5 );
      for( var i = 0; i < arrLen; i++ )
      {
         var dataType = dataTypes[parseInt( Math.random() * dataTypes.length )];  //randomly get 1 data type 

         var elem = getRandomValue( dataType );
         arr.push( elem );
      }

      return arr;
   }

   // 重写 Date 的 Format 方法
   Date.prototype.Format = function( fmt )   
   { //author: meizz   
      var o = {
         "M+": this.getMonth() + 1,                 //month   
         "d+": this.getDate(),                    //date  
         "h+": this.getHours(),                   //hour  
         "m+": this.getMinutes(),                 //minute 
         "s+": this.getSeconds(),                 //second   
         "q+": Math.floor( ( this.getMonth() + 3 ) / 3 ), //quarter   
         "S": this.getMilliseconds()             //millisecond   
      };
      if( /(y+)/.test( fmt ) )
         fmt = fmt.replace( RegExp.$1, ( this.getFullYear() + "" ).substr( 4 - RegExp.$1.length ) );
      for( var k in o )
         if( new RegExp( "(" + k + ")" ).test( fmt ) )
            fmt = fmt.replace( RegExp.$1, ( RegExp.$1.length == 1 ) ? ( o[k] ) : ( ( "00" + o[k] ).substr( ( "" + o[k] ).length ) ) );
      return fmt;
   }
}

/**********************************************************************
@description   在指定主机创建目录
@author        Liangxw
@parameter
   host        {string}     :     主机名
   dir         {string}     :     创建目录名
@usage         1. commMakeDir( COORDHOSTNAME, WORKDIR )
               2. commMakeDir( "localhost", WORKDIR )
***********************************************************************/
function commMakeDir ( hostName, dir )
{
   try
   {
      var remote = new Remote( hostName, CMSVCNAME );
      var file = remote.getFile();
      if( file.exist( dir ) )
      {
         return;
      }
      file.mkdir( dir );
   }
   catch( e )
   {
      commThrowError( e, "commMakeDir make dir " + dir + " in " + hostName + " error: " + e )
   }
}

/******************************************************************************
@description   创建 domain，先删除已有 domain，再创建
@author  zhaoyu
@parameter
   domainName       {string}    :    必填项，domain 名
   groupNames       {array}     :    默认为 []，domain 所在的 group
   options          {object}    :    默认为 {}，创建 domain 设置的属性
@return    {object}    新建 domain 的引用
***************************************************************************** */
function commCreateDomain ( db, domainName, groupNames, options )
{
   if( groupNames == undefined ) { groupNames = []; }
   if( options == undefined ) { options = {}; }
   commCheckType( groupNames, "object" );
   commCheckType( options, "object" );

   // drop old domain before create new domain
   commDropDomain( db, domainName );

   try
   {
      return db.createDomain( domainName, groupNames, options );
   }
   catch( e )
   {
      commThrowError( e, "commCreateDomain, create domain: " + domainName + " failed: " + e );
   }
}

/******************************************************************************
@description  删除 domain
@author  zhaoyu
@parameter
   domainName        {string}     :     必填项，要删除的 domain 名
   ignoreNotExist    {boolean}    :     默认为 true，忽略 domain 不存在的错误
******************************************************************************/
function commDropDomain ( db, domainName, ignoreNotExist )
{
   if( ignoreNotExist == undefined ) { ignoreNotExist = true; }
   commCheckType( ignoreNotExist, "boolean" );

   try
   {
      var domain = db.getDomain( domainName );
      var cursor = domain.listCollectionSpaces();
      while( cursor.next() )
      {
         var csName = cursor.current().toObj().Name;
         db.dropCS( csName );
      }
      db.dropDomain( domainName );
   } catch( e )
   {
      if( commCompareErrorCode( e, SDB_CAT_DOMAIN_NOT_EXIST ) && ignoreNotExist )
      {
         // think right
      } else
      {
         commThrowError( e, "commDropDomain, drop domain: " + domainName + " failed: " + e )
      }
   }
}

/******************************************************************************
@description  对比错误码
@author  luweikang
@parameter
   e         {Error}     :     必填项，catch 到的错误信息
   code      {number}    :     必填项，期望对比的错误码
******************************************************************************/
function commCompareErrorCode ( e, code )
{
   var errValue = e.message || e;
   return errValue == code;
}

/******************************************************************************
@description   封装错误信息
@author  luweikang
@parameter
   e         {Error}     :     必填项，catch 到的错误信息
   msg       {string}    :     错误信息
******************************************************************************/
function commThrowError ( e, msg )
{
   if( e instanceof Error )
   {
      throw e;
   } else
   {
      if( msg === undefined )
      {
         throw new Error( e );
      } else
      {
         throw new Error( msg );
      }
   }
}

/******************************************************************************
@description   类型检测（func自用）
@author  lyy
@parameter
   variable         {any}     :     检测变量
   expType          {"string",""}    :     期望类型
******************************************************************************/
function commCheckType ( variable, expType )
{
   if( variable == null )
   {
      throw new Error( variable + " == null " );
   }
   if( expType == "string" || expType == "number" || expType == "boolean" || "object" == expType || "function" == expType )
   {
      if( typeof ( variable ) != expType )
      {
         throw new Error( variable + " isn't " + expType );
      }
   } else if( expType == "array" )
   {
      if( !Array.isArray( variable ) )
      {
         throw new Error( variable + " isn't " + expType );
      }
   } else
   {
      throw new Error( expType + "not exists" );
   }
}

/******************************************************************************
@description  校验索引一致性，包括主备是否都存在索引，是否都不存索引
@author  liuli
@parameter
   idxname        {string}     :     必填项，要检测的 idxname 名
   isExist        {boolean}    :     默认为 true，检测索引存在且主备一致
******************************************************************************/
function commCheckIndexConsistent ( db, csname, clname, idxname, isExist )
{
   if( isExist == undefined ) { isExist = true; }
   var nodes = commGetCLNodes( db, csname + "." + clname );
   var timeOut = 300000;
   var doTime = 0;
   if( isExist )
   {
      var expIndex = null;
      do
      {
         var sucNodes = 0;
         for( var i = 0; i < nodes.length; i++ )
         {
            var seqdb = new Sdb( nodes[i].HostName + ":" + nodes[i].svcname );
            try
            {
               try
               {
                  var dbcl = seqdb.getCS( csname ).getCL( clname );
               } catch( e )
               {
                  if( e != SDB_DMS_NOTEXIST && e != SDB_DMS_CS_NOTEXIST )
                  {
                     throw new Error( e );
                  }
                  break;
               }
               try
               {
                  var actIndex = dbcl.getIndex( idxname );
                  sucNodes++;
               } catch( e )
               {
                  if( e != SDB_IXM_NOTEXIST )
                  {
                     throw new Error( e );
                  }
                  break;
               }
               if( actIndex.toObj().IndexFlag != "Normal" )
               {
                  break;
               }
               if( expIndex == null )
               {
                  expIndex = actIndex;
               }
               else
               {
                  var expDef = expIndex.toObj().IndexDef;
                  var actDef = actIndex.toObj().IndexDef;
                  delete expDef.CreateTime;
                  delete expDef.RebuildTime;
                  delete actDef.CreateTime;
                  delete actDef.RebuildTime;
                  assert.equal( expDef, actDef );
               }
            } finally
            {
               seqdb.close();
            }
         }
         sleep( 200 );
         doTime += 200;
      } while( doTime < timeOut && sucNodes < nodes.length );

      if( doTime >= timeOut )
      {
         throw new Error( "check timeout index not synchronized ! index exist:" + expIndex );
      }
   }
   else
   {
      var indexDef = "";
      do
      {
         var sucNodes = 0;
         for( var i = 0; i < nodes.length; i++ )
         {
            var seqdb = new Sdb( nodes[i].HostName + ":" + nodes[i].svcname );
            try
            {
               try
               {
                  var dbcl = seqdb.getCS( csname ).getCL( clname );
               }
               catch( e )
               {
                  if( e != SDB_DMS_NOTEXIST && e != SDB_DMS_CS_NOTEXIST )
                  {
                     throw new Error( e );
                  }
                  break;
               }

               try
               {
                  indexDef = dbcl.getIndex( idxname );
                  break;
               }
               catch( e )
               {
                  if( e != SDB_IXM_NOTEXIST )
                  {
                     throw new Error( e );
                  }
                  sucNodes++;
               }
            } finally
            {
               seqdb.close();
            }
         }
         sleep( 200 );
         doTime += 200;
      } while( doTime < timeOut && sucNodes < nodes.length )
      if( doTime >= timeOut )
      {
         throw new Error( "check timeout index not synchronized ! index exist:" + indexDef + ", nodename : " + seqdb );
      }
   }
}

// common database connection
try
{
   var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var assert = new Assert();
}
catch( e )
{
   commThrowError( e, "connect sdb " + COORDHOSTNAME + ":" + COORDSVCNAME );
}
