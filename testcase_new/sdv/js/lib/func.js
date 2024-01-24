/*******************************************************************************
*@Description: JavaScript common function library
*@Modify list:
*   2014-2-24 Jianhui Xu  Init
*******************************************************************************/
import( "../lib/assert.js" );
// begin global variable configuration
// CSPREFIX, COORDSVCNAME, COORDHOSTNAME  is input parameter
// UUID, UUNAME is input parameter
//if ( typeof(CSPREFIX) == "undefined" ) { CSPREFIX = "local_test"; }
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
if( typeof ( WORKDIR ) == "undefined" ) { WORKDIR = "/tmp/jstest"; }
//ES服务端主机名，CI默认传入192.168.28.143
if( typeof ( ESHOSTNAME ) == "undefined" ) { ESHOSTNAME = 'localhost'; }
//ES服务端端口号，CI默认传入9200
if( typeof ( ESSVCNAME ) == "undefined" ) { ESSVCNAME = '9200'; }
//ES全文索引前缀，与工程名相关
if( typeof ( FULLTEXTPREFIX ) == "undefined" ) { FULLTEXTPREFIX = ''; }
if( typeof ( CLEANFORFAIL ) == "undefined" ) { var CLEANFORFAIL = false; }

var COMMCSNAME = CHANGEDPREFIX + "_cs";
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

/* *****************************************************************************
@discription: check database mode is standalone
              判断集群是否是独立模式
@author: Jianhui Xu
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

      if( commCompareErrorCode( e, -159 ) )
      {
         return true;
      }
      else
      {
         throw e;
      }
   }
}

/* *****************************************************************************
@discription: create collection space
              创建并返回cs对象
@author: Jianhui Xu
@parameter
   ignoreExisted: default = false, value: true/false, cs已存在则直接返回getCS
   message: user define message, default:""
   options: create CS specify options, default:"";[by  xiaojun Hu ]
           exp : {"Domain":"domName"}
           
***************************************************************************** */
function commCreateCS ( db, csName, ignoreExisted, message, options )
{
   ++funcCommCreateCSTimes;
   if( ignoreExisted == undefined ) { ignoreExisted = false; }
   if( message == undefined ) { message = ""; }
   if( options == undefined ) { options = ""; }
   try
   {
      if( "" == options )
         return db.createCS( csName );
      else
         return db.createCS( csName, options );
   }
   catch( e )
   {

      if( !commCompareErrorCode( e, -33 ) || !ignoreExisted )
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

/* *****************************************************************************
@discription: create collection by user option
              创建并返回cl对象
@author: Jianhui Xu
@parameter
   optionObj: option object, default {}
   compressed: default = true, value: true/false
   autoCreateCS: default = true, value: true/false, 自动创建cs
   ignoreExisted: default = false, value: true/false, cl已存在则直接返回getCL
   message: default = "", value: user defined message string
***************************************************************************** */
function commCreateCL ( db, csName, clName, optionObj, autoCreateCS, ignoreExisted, message )
{
   ++funcCommCreateCLOptTimes;
   if( optionObj == undefined ) { optionObj = {}; }
   if( autoCreateCS == undefined ) { autoCreateCS = true; }
   if( ignoreExisted == undefined ) { ignoreExisted = false; }
   if( message == undefined ) { message = ""; }

   if( typeof ( optionObj ) != "object" )
   {
      throw new Error( "commCreateCL: optionObj is not object" );
   }
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
      if( !commCompareErrorCode( e, -22 ) || !ignoreExisted )
      {
         commThrowError( e, "commCreateCL[" + funcCommCreateCLOptTimes + "] create collection[" + csName + "." + clName + "] failed: " + e + ",message: " + message );
      }
   }

   //get collection
   try
   {
      return db.getCS( csName ).getCL( clName );
   }
   catch( e )
   {
      commThrowError( e, "commCreateCL[" + funcCommCreateCLOptTimes + "] get collection[" + csName + "." + clName + "] failed: " + e + ",message: " + message );
   }
}

/* *****************************************************************************
@discription: drop collection space
              删除集合空间
@author: Jianhui Xu
@parameter
   ignoreNotExist: default = true, value: true/false, 忽略不存在错误
   message: default = ""
***************************************************************************** */
function commDropCS ( db, csName, ignoreNotExist, message )
{
   ++funcCommDropCSTimes;
   if( ignoreNotExist == undefined ) { ignoreNotExist = true; }
   if( message == undefined ) { message = ""; }

   try
   {
      db.dropCS( csName );
   }
   catch( e )
   {
      if( commCompareErrorCode( e, -34 ) && ignoreNotExist )
      {
         // think right
      }
      else
      {
         commThrowError( e, "commDropCS[" + funcCommDropCSTimes + "] Drop collection space[" + csName + "] failed: " + e + ",message: " + message )
      }
   }
}

/* *****************************************************************************
@discription: drop collection
              删除集合
@author: Jianhui Xu
@parameter
   ignoreCSNotExist: default = true, value: true/false, 忽略集合空间不存在错误
   ignoreCLNotExist: default = true, value: true/false, 忽略集合不存在错误
   message: default = ""
***************************************************************************** */
function commDropCL ( db, csName, clName, ignoreCSNotExist, ignoreCLNotExist, message )
{
   ++funcCommDropCLTimes;
   if( message == undefined ) { message = ""; }
   if( ignoreCSNotExist == undefined ) { ignoreCSNotExist = true; }
   if( ignoreCLNotExist == undefined ) { ignoreCLNotExist = true; }

   try
   {
      db.getCS( csName ).dropCL( clName );
   }
   catch( e )
   {
      if( ( commCompareErrorCode( e, -34 ) && ignoreCSNotExist ) || ( commCompareErrorCode( e, -23 ) && ignoreCLNotExist ) )
      {
         // think right
      }
      else
      {
         commThrowError( e, "commDropCL[" + funcCommDropCLTimes + "] Drop collection[" + csName + "." + clName + "] failed: " + e + ",message: " + message )
      }
   }
}

/* *****************************************************************************
@discription: create index
              创建索引
@author: Jianhui Xu
@parameter
   indexDef: index define object
   options: index options
   ignoreExist: default is false, 忽略索引已存在错误
***************************************************************************** */
function commCreateIndex ( cl, name, indexDef, options, ignoreExist )
{
   if( options == undefined ) { options = {}; }
   if( ignoreExist == undefined ) { ignoreExist = false; }

   if( typeof ( indexDef ) != "object" )
   {
      throw new Error( "commCreateIndex: indexDef is not object" );
   }
   if( typeof ( options ) != "object" )
   {
      throw new Error( "commCreateIndex: optionsk is not object" );
   }

   try
   {
      cl.createIndex( name, indexDef, options );
   }
   catch( e )
   {
      println( "commCreateIndex: create index[" + name + "] failed: " + e );
      if( ignoreExist && ( commCompareErrorCode( e, -46 ) || commCompareErrorCode( e, -247 ) ) )
      {
         // ok
      }
      else
      {
         commThrowError( e, "commCreateIndex: create index[" + name + "] failed: " + e );
      }
   }
}

function commDropIndex ( cl, name, ignoreNotExist )
{
   if( ignoreNotExist == undefined ) { ignoreNotExist = false; }

   try
   {
      cl.dropIndex( name );
   }
   catch( e )
   {
      if( ignoreNotExist && commCompareErrorCode( e, -47 ) )
      {
         // ok
      }
      else
      {
         commThrowError( e, "commDropIndex: drop index[" + name + "] failed: " + e );
      }
   }
}

/* *****************************************************************************
@discription: check index consistency
@author: Jianhui Xu
@parameter
   exist: true/false, if true check index exist, else check index not exist, default is true
   timeout: default 30 secs
***************************************************************************** */
function commCheckIndexConsistency ( cl, name, exist, timeout )
{
   if( exist == undefined ) { exist = true; }
   if( timeout == undefined ) { timeout = 30; }

   //cl.toString = hostname:svc.csName.clName
   var arr1 = cl.toString().split( ":" );
   var arr2 = arr1[1].split( "." );
   var csName = arr2[1];
   var clName = arr2[2];
   var nodes = commGetCLNodes( db, csName, clName );

   var timecount = 0;
   while( true )
   {
      for( var j = 0; j < nodes.length; j++ )
      {
         try
         {
            var nodeConn = new Sdb( nodes[j].HostName, nodes[j].svcname );
            var tmpInfo = nodeConn.getCS( csName ).getCL( clName ).getIndex( name );
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
            if( tmpObj.IndexDef.name != name )
            {
               println( "commCheckIndexConsistency: get index name[" + tmpObj.IndexDef.name + "] is not the same with name[" + name + "]" );
               println( tmpInfo );
               throw new Error( "check name error" );
            }
         }
      }
      //var tmpInfo = cl.getIndex( name ) ;
      if( ( exist && tmpInfo == undefined ) ||
         ( !exist && tmpInfo != undefined ) )
      {
         if( timecount < timeout )
         {
            ++timecount;
            sleep( 1000 );
            continue;
         }
         throw new Error( "commCheckIndexConsistency: check index[" + name + "] time out" );
      }
      break;
   }
}

/* *****************************************************************************
@discription: print object function
@author: Jianhui Xu
***************************************************************************** */
function commPrintIndent ( str, deep, withRN )
{
   if( undefined == deep ) { deep = 0; }
   if( undefined == withRN ) { withRN = true; }
   for( var i = 0; i < 3 * deep; ++i )
   {
      print( " " );
   }
   if( withRN )
   {
      println( str );
   }
   else
   {
      print( str );
   }
}

function commPrint ( obj, deep )
{
   var isArray = false;
   if( typeof ( obj ) != "object" )
   {
      println( obj );
      return;
   }
   if( undefined == deep ) { deep = 0; }
   if( typeof ( obj.length ) == "number" ) { isArray = true; }

   if( 0 == deep )
   {
      if( !isArray )
      {
         commPrintIndent( "{", deep, false );
      }
      else
      {
         commPrintIndent( "[", deep, false );
      }
   }

   var i = 0;
   for( var p in obj )
   {
      if( i == 0 ) { commPrintIndent( "", 0, true ); }
      else if( i > 0 ) { commPrintIndent( ",", 0, true ); }
      ++i;

      if( typeof ( obj[p] ) == "object" )
      {
         if( typeof ( obj[p].length ) == "undefined" )
         {
            if( !isArray )
            {
               commPrintIndent( p + ": {", deep + 1, false );
            }
            else
            {
               commPrintIndent( "{", deep + 1, false );
            }
         }
         else if( !isArray )
         {
            commPrintIndent( p + ": [", deep + 1, false );
         }
         else
         {
            commPrintIndent( "[", deep + 1, false );
         }
         commPrint( obj[p], deep + 1 );
      }
      else if( typeof ( obj[p] ) == "function" )
      {
         // not print
      }
      else if( typeof ( obj[p] ) == "string" )
      {
         if( !isArray )
         {
            commPrintIndent( p + ': "' + obj[p] + '"', deep + 1, false );
         }
         else
         {
            commPrintIndent( '"' + obj[p] + '"', deep + 1, false );
         }
      }
      else
      {
         if( !isArray )
         {
            commPrintIndent( p + ': ' + obj[p], deep + 1, false );
         }
         else
         {
            commPrintIndent( obj[p], deep + 1, false );
         }
      }
   }
   if( i == 0 )
   {
      if( !isArray )
      {
         commPrintIndent( "}", 0, false );
      }
      else
      {
         commPrintIndent( "]", 0, false );
      }
   }
   else
   {
      commPrintIndent( "", 0, true );
      if( !isArray )
      {
         commPrintIndent( "}", deep, false );
      }
      else
      {
         commPrintIndent( "]", deep, false );
      }
   }
   if( deep == 0 ) { commPrintIndent( "", deep, true ) };
}

/* ******************************************************************************
@description : get collection groups
               获取集合所属的group名，返回已去重的groupName数组
@author : xiaojun Hu
@parameter:
   clname: collection name, such as : "foo.bar"
@return array[] ex:
   array[0] group1
   ...
***************************************************************************** */
function commGetCLGroups ( db, clName )
{
   if( typeof ( clName ) != "string" || clName.length == 0 )
   {
      throw new Error( "commGetCLGroups: Invalid clName parameter or clName is empty" );
   }
   if( commIsStandalone( db ) )
   {
      return new Array();
   }

   var tmpArray = new Array();
   var groups = {};
   var cursor;
   try
   {
      cursor = db.snapshot( 8, { Name: clName } );
   }
   catch( e )
   {
      commThrowError( e, "commGetCLGroups: snapshot collection space failed: " + e );
   }

   while( cursor.next() )
   {
      var cataInfo = cursor.current().toObj()['CataInfo'];
      for( var i = 0; i < cataInfo.length; ++i )
      {
         if( groups[cataInfo[i].GroupName] === undefined )
         {
            tmpArray.push( cataInfo[i].GroupName );
            groups[cataInfo[i].GroupName] = 1;
         }
      }
   }

   return tmpArray;
}

/******************************************************************************
@description : get collection nodes
               获取集合所在的节点
@author : luweikang
@parameter:
   csName: cs name
   clName: cl name 
@return array[] ex:
   array[0] {"HostName": "XXXX", "svcname": "XXXX"}
   array[1] {"HostName": "XXXX", "svcname": "XXXX"}
   ...
******************************************************************************/
function commGetCLNodes ( db, csName, clName )
{
   var clGroups = commGetCLGroups( db, csName + "." + clName );
   var nodes = [];
   for( var i = 0; i < clGroups.length; i++ )
   {
      nodes = nodes.concat( commGetGroupNodes( db, clGroups[i] ) );
   }
   return nodes;
}

/* *****************************************************************************
@discription: get collection space groups
              获取集合空间所属的group名，返回groupName数组
@author: Jianhui Xu
@parameter:
   csname: collection space name
@return array[] ex:
   array[0] group1
   ...
***************************************************************************** */
function commGetCSGroups ( db, csname )
{
   if( typeof ( csname ) != "string" || csname.length == 0 )
   {
      throw new Error( "commGetCSGroups: Invalid csname parameter or csname is empty" );
   }
   if( commIsStandalone( db ) )
   {
      return new Array();
   }

   var tmpArray = [];
   var tmpInfo = commGetSnapshot( db, SDB_SNAP_COLLECTIONSPACES );

   for( var i = 0; i < tmpInfo.length; ++i )
   {
      var tmpObj = tmpInfo[i];
      if( tmpObj.Name != csname )
      {
         continue;
      }
      for( var j = 0; j < tmpObj.Group.length; ++j )
      {
         tmpArray.push( tmpObj.Group[j] );
      }
   }
   return tmpArray;
}

/* *****************************************************************************
@discription: get all groups
              获取所有group的详细信息，默认只获取数据组
@author: Jianhui Xu
@parameter:
   filter: group name filter
   exceptCata : default true
   exceptCoord: default true
@return array[][] ex:
        [0]
           [0] {"GroupName":"XXXX", "GroupID":XXXX, "Status":XX, "Version":XX, "Role":XX, "PrimaryNode":XXXX, "Length":XXXX, "PrimaryPos":XXXX}
           [1] {"HostName":"XXXX", "dbpath":"XXXX", "svcname":"XXXX", "NodeID":XXXX}
           [N] ...
        [N]
           ...
***************************************************************************** */
function commGetGroups ( db, print, filter, exceptCata, exceptCoord, exceptSpare )
{
   if( undefined == print ) { print = false; }
   if( undefined == exceptCata ) { exceptCata = true; }
   if( undefined == exceptCoord ) { exceptCoord = true; }
   if( undefined == exceptSpare ) { exceptSpare = true; }

   var tmpArray = [];
   var tmpInfoCur;
   try
   {
      tmpInfoCur = db.listReplicaGroups();
   }
   catch( e )
   {
      if( commCompareErrorCode( e, -159 ) )
      {
         return tmpArray;
      }
      else
      {
         if( true == print )
            println( "commGetGroups failed: " + e );
         commThrowError( e, "commGetGroups failed: " + e );
      }
   }
   var tmpInfo = commCursor2Array( tmpInfoCur, "GroupName", filter );

   var index = 0;
   for( var i = 0; i < tmpInfo.length; ++i )
   {
      var tmpObj = tmpInfo[i];
      if( true == exceptCata && tmpObj.GroupID == CATALOG_GROUPID )
      {
         continue;
      }
      if( true == exceptCoord && tmpObj.GroupID == COORD_GROUPID )
      {
         continue;
      }
      if( true == exceptSpare && tmpObj.GroupID == SPARE_GROUPID )
      {
         continue;
      }
      if ( tmpObj.Group.length == 0 ) continue ;
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


/* *****************************************************************************
@discription: get the number of groups
               获取集群所有group个数，默认只获取数据组
@author: Jianhua Li
@parameter:
   filter: group name filter
   exceptCata : default true
   exceptCoord: default true
@return integer
***************************************************************************** */
function commGetGroupsNum ( db, print, filter, exceptCata, exceptCoord, exceptSpare )
{
   if( undefined == print ) { print = false; }
   if( undefined == exceptCata ) { exceptCata = true; }
   if( undefined == exceptCoord ) { exceptCoord = true; }
   if( undefined == exceptSpare ) { exceptSpare = true; }

   var tmpInfoCur;
   var num = 0;
   try
   {
      tmpInfoCur = db.listReplicaGroups();
   }
   catch( e )
   {
      if( commCompareErrorCode( e, -159 ) )
      {
         return num;
      }
      else
      {
         if( true == print )
            println( "commGetGroups failed: " + e );
         commThrowError( e, "commGetGroups failed: " + e );
      }
   }
   var tmpInfo = commCursor2Array( tmpInfoCur, "GroupName", filter );

   for( var i = 0; i < tmpInfo.length; ++i )
   {
      var tmpObj = tmpInfo[i];
      if( true == exceptCata && tmpObj.GroupID == CATALOG_GROUPID )
      {
         continue;
      }
      if( true == exceptCoord && tmpObj.GroupID == COORD_GROUPID )
      {
         continue;
      }
      if( true == exceptSpare && tmpObj.GroupID == SPARE_GROUPID )
      {
         continue;
      }
      ++num;
   }

   return num;
}

/* *****************************************************************************
@discription: get data groups name
              获取所有数据组名
@author: luweikang
@return integer
***************************************************************************** */
function commGetDataGroupNames ( db )
{
   var groups = commGetGroups( db, false, "", true, true, true );
   var groupNames = [];
   for( var i = 0; i < groups.length; i++ )
   {
      groupNames.push( groups[i][0].GroupName );
   }
   return groupNames;
}

/* ****************************************************************************
@discription: get all node from specified group
              获取指定数据组的所有节点
@author: luweikang
@parameter: 
   groupName: group name
@return: array[] ex:
   array[0] {"HostName": "XXXX", "svcname": "XXXX"}
   array[1] {"HostName": "XXXX", "svcname": "XXXX"}
   ...
**************************************************************************** */
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
   }
   return tmpArray;
}

/* *****************************************************************************
@discription: get cs and cl
@author: Jianhui Xu
@parameter:
   csfilter : collection space filter
   clfilter : collection filter
@return array[] ex:
        [0] {"cs":"XXXX", "cl":["XXXX", "XXXX",...] }
        [N] ...
***************************************************************************** */
function commGetCSCL ( db, csfilter, clfilter )
{
   if( csfilter == undefined ) { csfilter = ""; }
   if( clfilter == undefined ) { clfilter = ""; }

   var tmpCSCL = new Array();
   var tmpInfo = commGetSnapshot( db, SDB_SNAP_COLLECTIONSPACES );

   var m = 0;
   for( var i = 0; i < tmpInfo.length; ++i )
   {
      var tmpObj = tmpInfo[i];
      if( csfilter.length != 0 && tmpObj.Name.indexOf( csfilter, 0 ) == -1 )
      {
         continue;
      }
      tmpCSCL[m] = new Object();
      tmpCSCL[m].cs = tmpObj.Name;
      tmpCSCL[m].cl = new Array();

      var tmpCollection = tmpObj.Collection;
      for( var j = 0; j < tmpCollection.length; ++j )
      {
         if( clfilter.length != 0 && tmpCollection[j].Name.indexOf( clfilter, 0 ) == -1 )
         {
            continue;
         }
         tmpCSCL[m].cl.push( tmpCollection[j].Name );
      }
      ++m;
   }
   return tmpCSCL;
}

/* *****************************************************************************
@discription: get backup
@author: Jianhui Xu
@parameter:
   filter : backup name filter
   path : backup path
   isSubDir: true/false, default is false
   condObj : condition object
   grpNameArray : group name array
@return array[] ex:
        [0] "XXXX"
        [N] ...
***************************************************************************** */
function commGetBackups ( db, filter, path, isSubDir, cond, grpNameArray )
{
   if( path == undefined ) { path = ""; }
   if( isSubDir == undefined ) { isSubDir = false; }
   if( cond == undefined ) { cond = {}; }
   if( grpNameArray == undefined ) { grpNameArray = new Array(); }

   if( typeof ( cond ) != "object" )
   {
      throw new Error( "commGetBackups: cond is not a object" );
   }
   if( typeof ( grpNameArray ) != "object" )
   {
      throw new Error( "commGetBackups: grpNameArray is not a array" );
   }

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

   for( var i = 0; i < tmpInfo.length; ++i )
   {
      var tmpBackupObj = tmpInfo[i];
      if( typeof ( tmpBackupObj.Name ) == "undefined" )
      {
         continue;
      }
      var exist = false;
      for( var j = 0; j < tmpBackup.length; ++j )
      {
         if( tmpBackupObj.Name == tmpBackup[j] )
         {
            exist = true;
            break;
         }
      }
      if( exist )
      {
         continue;
      }
      tmpBackup.push( tmpBackupObj.Name );
   }
   return tmpBackup;
}

/* *****************************************************************************
@discription: get procedure
@author: Jianhui Xu
@parameter:
   filter : procedure name filter
@return array[] ex:
        [0] "XXXX"
        [N] ...
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
      if( typeof ( tmpProcedureObj.name ) == "undefined" )
      {
         continue;
      }
      tmpProcedure.push( tmpProcedureObj.name );
   }
   return tmpProcedure;
}

/* *****************************************************************************
@discription: get domains
@author: Jianhui Xu
@parameter:
   filter : domain name filter
@return array[] ex:
        [0] "XXXX"
        [N] ...
***************************************************************************** */
function commGetDomains ( db, filter )
{
   var tmpDomain = new Array();
   var tmpInfoCur;
   try
   {
      tmpInfoCur = db.listDomains();
   }
   catch( e )
   {
      println( "commGetDomains failed: " + e );
      return tmpDomain;
   }
   var tmpInfo = commCursor2Array( tmpInfoCur, "Name", filter );

   for( var i = 0; i < tmpInfo.length; ++i )
   {
      var tmpDomainObj = tmpInfo[i];
      if( typeof ( tmpDomainObj.Name ) == "undefined" )
      {
         continue;
      }
      tmpDomain.push( tmpDomainObj.Name );
   }
   return tmpDomain;
}

/* *****************************************************************************
@discription: check nodes function
              检查节点是否可以连接，返回连接失败的节点
@author: Jianhui Xu
@return array[] ex:
        [0] {"HostName":"XXXX", "dbpath":"XXXX", "svcname":"XXXX", "NodeID":XXXX}
        [N] ...
***************************************************************************** */
function commCheckNodes ( groups )
{
   var tmpFailedGrp = new Array();
   for( var i = 0; i < groups.length; ++i )
   {
      for( var j = 1; j < groups[i].length; ++j )
      {
         var tmpDB;
         try
         {
            tmpDB = new Sdb( groups[i][j].HostName, groups[i][j].svcname );
            tmpDB.close();
         }
         catch( e )
         {
            tmpFailedGrp.push( groups[i][j] );
         }
      }
   }
   return tmpFailedGrp;
}

/* *****************************************************************************
@discription: check business right function
              检查集群状态，返回故障的节点
@author: Jianhui Xu
@return array[][] ex:
        [0]
           [0] {"GroupName":"XXXX", "GroupID":XXXX, "PrimaryNode":XXXX, "ConnCheck":t/f, "PrimaryCheck":t/f, "LSNCheck":t/f, "ServiceCheck":t/f, "DiskCheck":t/f }
           [1] {"HostName":"XXXX", "svcname":"XXXX", "NodeID":XXXX, "Connect":t/f, "IsPrimay":t/f, "LSN":XXXX, "ServiceStatus":t/f, "FreeSpace":XXXX }
           [N] ...
        [N]
           ...
***************************************************************************** */
function commCheckBusinessStatus ( db, timeout, checkLSN, diskThreshold )
{
   if( checkLSN == undefined ) { checkLSN = true; }
   if( timeout == undefined ) { timeout = 120 }

   for( var i = 0; i < timeout; i++ )
   {
      var groups = commGetGroups( db, false, "", false );
      var tmpArr = commCheckBusiness( groups, checkLSN, diskThreshold );
      if( tmpArr.length == 0 )
      {
         break;
      }
      else if( i < ( timeout - 1 ) )
      {
         sleep( 1000 );
      }
      else
      {
         throw new Error( "check the cluster state timeout, check failed nodes: "
            + JSON.stringify( tmpArr ) );
      }
   }
}

/* *****************************************************************************
@discription: check business right function
              检查集群状态，返回故障的节点
@author: Jianhui Xu
@return array[][] ex:
        [0]
           [0] {"GroupName":"XXXX", "GroupID":XXXX, "PrimaryNode":XXXX, "ConnCheck":t/f, "PrimaryCheck":t/f, "LSNCheck":t/f, "ServiceCheck":t/f, "DiskCheck":t/f }
           [1] {"HostName":"XXXX", "svcname":"XXXX", "NodeID":XXXX, "Connect":t/f, "IsPrimay":t/f, "LSN":XXXX, "ServiceStatus":t/f, "FreeSpace":XXXX }
           [N] ...
        [N]
           ...
***************************************************************************** */
function commCheckBusiness ( groups, checkLSN, diskThreshold )
{
   if( checkLSN == undefined ) { checkLSN = false; }
   if( diskThreshold == undefined ) { diskThreshold = 134217728; }
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
      tmpCheckGrp[grpindex][0].DiskCheck = true;

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
            tmpSysInfoCur = tmpDB.snapshot( 6 );
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
         // check disk
         tmpCheckGrp[grpindex][ndindex].FreeSpace = tmpSysInfoObj.Disk.FreeSpace;
         if( tmpCheckGrp[grpindex][ndindex].FreeSpace < diskThreshold )
         {
            error = true;
            tmpCheckGrp[grpindex][0].DiskCheck = false;
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

/* *****************************************************************************
@discription: check whether the lsn of the group is consistent
@author: luweikang
@parameter:
   groupNames: groups name, default is all data group and catalog group name.
   timeout: check the maximum amount of time each group take, default is 60s.
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
            println( "master snapshot: " + JSON.stringify( masterSnapshot ) );
            println( "slave snapshot: " + JSON.stringify( slaveSnapshot ) );
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

/* *****************************************************************************
@discription: check business right function
@author: xiaojun Hu
@return array[][] ex:
        [0]
           [0] {"GroupName":"XXXX", "GroupID":XXXX, "PrimaryNode":XXXX, "ConnCheck":t/f, "PrimaryCheck":t/f, "LSNCheck":t/f, "ServiceCheck":t/f, "DiskCheck":t/f }
           [1] {"HostName":"XXXX", "svcname":"XXXX", "NodeID":XXXX, "Connect":t/f, "IsPrimay":t/f, "LSN":XXXX, "ServiceStatus":t/f, "FreeSpace":XXXX }
           [N] ...
        [N]
           ...
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
               InstallPath = LocalPath;
            else
               throw new Error( "Don'tGetLocalPath" );
         }
         else
            commThrowError( e, "Don'tGetLocalPath" );
      }
   }
   catch( e )
   {
      commThrowError( e, "failed to get install path[common]: " + e );
   }
   return InstallPath;
}

/* *****************************************************************************
@Description: build exception
@author: wenjing wang
@parameter:
  funname  : funnction name
  e        : js exception
  operate  : current operation
  expectval: expect value
  realval  : real value
@return string:exception msg
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

/* *****************************************************************************
@Description: The number or string is whether in Array
@author: xujianhui
@parameter:
   needle: the compared value, number or string
   hayStack : the array
@return string:true/false
***************************************************************************** */
function commInArray ( needle, hayStack )
{
   type = typeof needle;
   if( type == 'string' || type == 'number' )
   {
      for( var i in hayStack )
      {
         if( hayStack[i] == needle ) return true;
      }
      return false;
   }
}

/* ********************************************************************
@Description: comparison result set
@parameter:
         cursor:  DBCursor
         expRecs: array
         exceptId: noncomparison of _id, default true
@author: luweikang
********************************************************************* */
function commCompareResults ( cursor, expRecs, exceptId )
{
   if( exceptId == undefined ) { exceptId = true; }
   var actRecs = [];
   var pos = 0;
   var isSuccess = true;
   var posOfFailure;
   var isLong = false;

   try
   {
      while( cursor.next() )
      {
         var expRecord = expRecs[pos++];
         var actRecord = cursor.current().toObj();
         if( actRecord._id != undefined && exceptId )
         {
            delete actRecord._id;
         }

         if( isSuccess && !commCompareObject( expRecord, actRecord ) )
         {
            isSuccess = false;
            posOfFailure = pos - 1;
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
         posOfFailure = pos - 1;
         if( JSON.stringify( expRecs[posOfFailure] ).length > 1024 )
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
         throw new Error( "compare the " + recordLocation + "th record failed, "
            + "\nexp record count: " + expRecs.length
            + "\nact record count: " + actRecs.length
            + "\nexp record: " + JSON.stringify( expRecs[posOfFailure] )
            + "\nact record: " + JSON.stringify( actRecs[posOfFailure] ) );
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



/* ********************************************************************
@Description: get snapshot
@author: luweikang
@return array[][] ex:
        [0]{"xxxx": "xxxx", "xxxx": "xxxx"}
        [1]{"xxxx": "xxxx", "xxxx": "xxxx"}
********************************************************************* */
function commGetSnapshot ( db, snapshotType, condObj, selObj, sortObj, skipNum, limitNum, optionsObj )
{
   if( condObj == undefined ) { condObj = {}; }
   if( selObj == undefined ) { selObj = {}; }
   if( sortObj == undefined ) { sortObj = {}; }
   if( skipNum == undefined ) { skipNum = 0; }
   if( limitNum == undefined ) { limitNum = -1; }
   if( optionsObj == undefined ) { optionsObj = {}; }

   if( typeof ( condObj ) != "object" || condObj == null ) { throw new Error( "cond must be obj, can't be null" ); }
   if( typeof ( selObj ) != "object" || selObj == null ) { throw new Error( "sel must be obj, can't be null" ); }
   if( typeof ( sortObj ) != "object" || sortObj == null ) { throw new Error( "sort must be obj, can't be null" ); }
   if( typeof ( optionsObj ) != "object" || optionsObj == null ) { throw new Error( "options must be obj, can't be null" ); }

   var snapshotOption = new SdbSnapshotOption();
   snapshotOption.cond( condObj ).sel( selObj ).options( optionsObj );
   snapshotOption.sort( sortObj ).skip( skipNum ).limit( limitNum );

   var tmpArr = [];
   var cursor = null;
   try
   {
      cursor = db.snapshot( snapshotType, snapshotOption );
      while( cursor.next() )
      {
         tmpArr.push( cursor.current().toObj() );
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
   return tmpArr;
}

/* ********************************************************************
@Description: traversal cursor
@author: luweikang
@return array[]
********************************************************************* */
function commCursor2Array ( cursor, fieldName, filter )
{
   var tmpArray = [];
   if( cursor === undefined || cursor === null )
   {
      throw new Error( "cursor can't be undefined or null." );
   }
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
      cursor.close();
   }

   return tmpArray;
}

/* *******************************************************************
@Description: check node data consistency
@author: luweikang
******************************************************************* */
function commInspectData ( db, group, csName, clName, loop )
{
   var coord = " -d " + db.toString();
   var installDir = commGetInstallPath();
   var romdom = Math.floor( ( Math.random() * 10000 ) );
   var reportPath = WORKDIR + "/inspect_" + romdom;
   var output = " -o " + reportPath;
   ( group === undefined || group === "" ) ? group = "" : group = " -g " + group;
   ( csName === undefined || csName === "" ) ? csName = "" : csName = " -c " + csName;
   ( clName === undefined || clName === "" ) ? clName = "" : clName = " -l " + clName;
   ( loop === undefined ) ? loop = "" : loop = " -t " + loop;

   var inspect = installDir + "/bin/sdbinspect" + coord + group + csName + clName + output + loop;
   println( inspect );

   try
   {
      var cmd = new Cmd();
      var result = cmd.run( inspect );
   }
   catch( e )
   {
      throw new Error( e );
   }

   var tmpArr = result.split( "\n" );
   if( tmpArr[tmpArr.length - 3] !== "Reason for exit : exit with no records different" )
   {
      throw new Error( "report path: " + reportPath + "\n" + result );
   }
   cmd.run( "rm -rf " + reportPath + "*" );
}

/* *******************************************************************
@Description: create group and start
              db: connection handle, can't be standalone                
              rgName: group name
              nodesNum: node num, node svc like 26000 26010 ....
@return       nodeInfos : hostname, svcname, log paths to be backed up
               array[]:
                  [{hostname: "xxx", svcname: "xxx", logpath: "xxx"}]
@author: luweikang
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
            if( e !== 1 )
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
            if( commCompareErrorCode( e, -145 ) || commCompareErrorCode( e, -290 ) )
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
      }
      while( failedCount < maxRetryTimes );
   }
   rg.start();
   return nodeInfos;
}

/**********************************************************************
@Description:  generate all kinds of types data randomly
@author:       Ting YU
@usage:        var rd = new commDataGenerator();
               1.var recs = rd.getRecords( 300, "int", ['a','b'] );
               2.var recs = rd.getRecords( 3, ["int", "string"] );
               3.rd.getValue("string");
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
      if( dataTypes.constructor !== Array ) { dataTypes = [dataTypes]; }

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
}

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

/**********************************************************************
@Description:  make dir in host( can be used to make WORKDIR )
@author:       Liangxw
@usage:        1. commMakeDir( COORDHOSTNAME, WORKDIR )
               2. commMakeDir( "localhost", WORKDIR )
***********************************************************************/
function commMakeDir ( host, dir )
{
   try
   {
      var remote = new Remote( host, CMSVCNAME );
      var file = remote.getFile();
      if( file.exist( dir ) )
      {
         return;
      }
      file.mkdir( dir );
   }
   catch( e )
   {
      commThrowError( e, "commMakeDir make dir " + dir + " in " + host + " error: " + e )
   }
}

/* *****************************************************************************
@discription: create domain
@author: zhaoyu
@parameter
***************************************************************************** */
function commCreateDomain ( db, domainName, groupNames, options, ignoreExisted, message )
{
   var outmessage = "";
   if( options == undefined ) { options = {}; }
   if( message !== undefined && message !== "" ) { var outmessage = ",message:" + message; }
   if( ignoreExisted == undefined ) { ignoreExisted = false; }
   try
   {
      return db.createDomain( domainName, groupNames, options );
   }
   catch( e )
   {
      if( !commCompareErrorCode( e, -215 ) || !ignoreExisted )
      {
         commThrowError( e, "commCreateDomain, create domain: " + domainName + " failed: " + e + outmessage );
      }
   }

   try
   {
      return db.getDomain( domainName );
   }
   catch( e )
   {
      commThrowError( e, "commCreateDomain, get domain: " + domainName + " failed: " + e + outmessage )
   }
}

/* *****************************************************************************
@discription: drop domain
@author: zhaoyu
@parameter
***************************************************************************** */
function commDropDomain ( db, domainName, ignoreNotExist, message )
{
   var outmessage = "";
   if( message !== undefined && message !== "" ) { var outmessage = ",message:" + message; }
   if( ignoreNotExist == undefined ) { ignoreNotExist = true; }
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
      if( !commCompareErrorCode( e, -214 ) || !ignoreNotExist )
      {
         commThrowError( e, "commDropDomain, drop domain: " + domainName + " failed: " + e + outmessage )
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

/* *****************************************************************************
@discription: create procedure
@author: zhaoyu
@parameter
***************************************************************************** */
function commCreateProcedure ( db, code, ignoreExisted, message )
{
   var outmessage = "";
   if( message !== undefined && message !== "" ) { var outmessage = ",message:" + message; }
   if( ignoreExisted == undefined ) { ignoreExisted = false; }
   try
   {
      db.createProcedure( code );
   } catch( e )
   {
      if( !commCompareErrorCode( e, -38 ) || !ignoreExisted )
      {
         commThrowError( e, "commCreateProcedure, create procedure: " + code + " failed: " + e + outmessage );
      }
   }
}

/* *****************************************************************************
@discription: remove procedure
@author: zhaoyu
@parameter
***************************************************************************** */
function commRemoveProcedure ( db, functionName, ignoreNotExist, message )
{
   var outmessage = "";
   if( message !== undefined && message !== "" ) { var outmessage = ",message:" + message; }
   if( ignoreNotExist == undefined ) { ignoreNotExist = true; }
   try
   {
      db.removeProcedure( functionName );
   } catch( e )
   {
      if( !commCompareErrorCode( e, -233 ) || !ignoreNotExist )
      {
         commThrowError( e, "commRemoveProcedure, remove procedure: " + functionName + " failed: " + e + outmessage );
      }
   }
}

/* *****************************************************************************
@discription: create userName
@author: zhaoyu
@parameter
***************************************************************************** */
function commCreateUsr ( db, userName, password, options, ignoreExisted, message )
{
   var outmessage = "";
   if( options == undefined ) { options = {}; }
   if( message !== undefined && message !== "" ) { var outmessage = ",message:" + message; }
   if( ignoreExisted == undefined ) { ignoreExisted = false; }
   try
   {
      db.createUsr( userName, password, options );
   } catch( e )
   {
      if( !commCompareErrorCode( e, -295 ) || !ignoreExisted )
      {
         commThrowError( e, "commCreateUsr, create userName: " + userName + " failed: " + e + outmessage );
      }
   }
}

/* *****************************************************************************
@discription: drop userName
@author: zhaoyu
@parameter
***************************************************************************** */
function commDropUsr ( db, userName, password, ignoreNotExist, message )
{
   var outmessage = "";
   if( message !== undefined && message !== "" ) { var outmessage = ",message:" + message; }
   if( ignoreNotExist == undefined ) { ignoreNotExist = true; }
   try
   {
      db.dropUsr( userName, password );
   } catch( e )
   {
      if( !commCompareErrorCode( e, -300 ) || !ignoreNotExist )
      {
         commThrowError( e, "commDropUsr, drop userName: " + userName + " failed: " + e + outmessage );
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

function commCompareErrorCode ( e, code )
{
   if( e.constructor === Error )
   {
      var errorCode = e.message;
      return errorCode == code;
   }
   else
   {
      return e == code;
   }
}

function commThrowError ( e, msg )
{
   if( e.constructor === Error )
   {
      throw e;
   }
   else
   {
      if( msg === undefined )
      {
         throw new Error( e );
      }
      else
      {
         throw new Error( msg );
      }
   }
}

commCreateCL( db, COMMCSNAME, COMMDUMMYCLNAME, { ShardingType: 'hash', ShardingKey: { _id: 1 }, AutoSplit: true }, true, true );
