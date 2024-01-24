import( "../lib/main.js" );
import( "../lib/lobSubCL_commlib.js" );
import( "../lib/basic_operation/commlib.js" );
/******************************************************************************
 用例IP配置说明：datasrcIp 是用 runtest.sh 传入
 如需运行全部用例需要进行如下配置并放开用例中main()的注释：datasrcIp 和 datasrcIp1 是同一集群的不同IP
 other_datasrcIp1 和 other_datasrcIp2 是不同集群的不同IP
 一共需要3个数据源集群，datasrcIp 和 datasrcIp1 属于第一个集群
 other_datasrcIp1 属于第二个集群，other_datasrcIp2 属于第三个集群
 datasrcPort 统一使用相同的用 runtest.sh 传入的端口号，如果对应的端口号不相同请自行修改
 ******************************************************************************/

var srcCSName = "comm_srcCS";
var srcCLName = "comm_srcCL";
var srcDataName = "srcData";

var datasrcIp = DSHOSTNAME;
// CI暂时只提供一台主机
// var datasrcIp1 = "192.168.31.39";
// 部分用例数据源需要多个集群，CI不运行，本地运行时自行配置
// var other_datasrcIp1 = "192.168.31.41";
// var other_datasrcIp2 = "192.168.20.44";
var datasrcPort = DSSVCNAME;
var userName = "sdbadmin";
var passwd = "sdbadmin";
var datasrcUrl = datasrcIp + ":" + datasrcPort;
// var datasrcUrl1 = datasrcIp1 + ":" + datasrcPort;
// var otherDSUrl1 = other_datasrcIp1 + ":" + datasrcPort;
// var otherDSUrl2 = other_datasrcIp2 + ":" + datasrcPort;


// var datasrcDB = new Sdb( datasrcIp, datasrcPort, userName, passwd );  CI-1687 数据源用例下线
// var datasrcDB1 = new Sdb( datasrcIp1, datasrcPort, userName, passwd );

function clearDataSource ( csName, dataSrcName )
{
   if( typeof ( csName ) === "string" )
   {
      try
      {
         db.dropCS( csName );
      }
      catch( e )
      {
         if( e != SDB_DMS_CS_NOTEXIST )
         {
            throw new Error( e );
         }
      }
   }
   else
   {
      for( var i in csName )
      {
         try
         {
            db.dropCS( csName[i] );
         }
         catch( e )
         {
            if( e != SDB_DMS_CS_NOTEXIST )
            {
               throw new Error( e );
            }
         }
      }
   }

   try
   {
      db.dropDataSource( dataSrcName )
   }
   catch( e )
   {
      if( e != SDB_CAT_DATASOURCE_NOTEXIST )
      {
         throw new Error( e );
      }
   }
}

function sortBy ( field )
{
   return function( a, b )
   {
      return a[field] > b[field];
   }
}



/************************************
*@Description: bulk insert data
*@input: dbcl        db.getCS(cs).getCL(cl)
         recordNum   插入记录数
         recordStart 记录数起始值
         recordEnd   记录结束值
**************************************/
function insertBulkData ( dbcl, recordNum, recordStart, recordEnd )
{
   if( undefined == recordNum ) { recordNum = 5000; }
   if( undefined == recordStart ) { recordStart = 0; }
   if( undefined == recordEnd ) { recordEnd = 5000; }
   try
   {
      var doc = [];
      for( var i = 0; i < recordNum; i++ )
      {
         var bValue = recordStart + parseInt( Math.random() * ( recordEnd - recordStart ) );
         var cValue = recordStart + parseInt( Math.random() * ( recordEnd - recordStart ) );
         doc.push( { a: i, b: bValue, c: cValue } );
      }
      dbcl.insert( doc );
      println( "--bulk insert data success" );
   }
   catch( e )
   {
      throw buildException( "insertBulkData()", e, "insert", "insert data :" + JSON.stringify( doc ), "insert fail" );
   }
   return doc;
}

function getDSMajorVersion ( dataSrcName )
{
   var DSVersion = db.listDataSources( { Name: dataSrcName } ).current().toObj().DSVersion;
   var majorVersion = DSVersion.slice( 0, 1 );
   return majorVersion;
}

function getDSVersion ( dataSrcName )
{
   var DSVersion = db.listDataSources( { Name: dataSrcName } ).current().toObj().DSVersion;
   var version = DSVersion.split( "." );
   return version;
}

function getCoordUrl ( sdb )
{
   var coordUrls = [];
   var rgInfo = sdb.getRG( 'SYSCoord' ).getDetail().current().toObj().Group;
   for( var i = 0; i < rgInfo.length; i++ )
   {
      var hostname = rgInfo[i].HostName;
      var svcname = rgInfo[i].Service[0].Name;
      coordUrls.push( hostname + ":" + svcname );
   }
   return coordUrls;
}

function updateConf ( db, configs, options, errno )
{
   try
   {
      db.updateConf( configs, options );
   }
   catch( e )
   {
      if( errno.indexOf( Number( e ) ) == -1 )
      {
         throw e;
      }
   }
}

function deleteConf ( db, configs, options, errno )
{
   try
   {
      db.deleteConf( configs, options );
   }
   catch( e )
   {
      if( errno.indexOf( Number( e ) ) == -1 )
      {
         throw e;
      }
   }
}

/************************************************************************
*@Description: 设置会话访问属性，查看访问计划检查访问节点信息，符合条件的多个节点需要访问均衡
*@input: cl            db.getCS(cs).getCL(cl)
         expAccessNodes  预期访问节点
         options         会话访问属性信息
**************************************************************************/
function checkAccessNodes ( cl, expAccessNodes, options )
{
   var doTimes = 0;
   var timeOut = 300000;
   var actAccessNodes = [];
   while( doTimes < timeOut )//设置instanceid后，获取访问的节点，当访问节点数组的长度等于期望结果时结束循环
   {
      db.setSessionAttr( options );
      sleep( 100 );
      var cursor = cl.find().explain();
      while( cursor.next() )
      {
         var actAccessNode = cursor.current().toObj().NodeName;
         if( actAccessNodes.indexOf( actAccessNode ) === -1 )
         {
            actAccessNodes.push( actAccessNode );
         }
      }

      if( actAccessNodes.length === expAccessNodes.length )
      {
         break;
      }
      else
      {
         doTimes += 100;
      }
   }

   if( doTimes >= timeOut )
   {
      throw new Error( "actAccessNodes: " + actAccessNodes + ", expAccessNodes: " + expAccessNodes );
   }

   //实际结果与预期结果比较
   for( var i in expAccessNodes )
   {
      if( actAccessNodes.indexOf( expAccessNodes[i] ) === -1 )
      {
         println( "actAccessNodes: " + actAccessNodes + "\nexpAccessNodes: " + expAccessNodes );
         throw new Error( "The actAccessNodes do not include the node: " + expAccessNodes[i] );
      }
   }
}

/************************************************************************
*@Description: 设置会话访问属性，查看访问计划检查访问节点信息
*@input: dbcl            db.getCS(cs).getCL(cl)
         expAccessNodes  预期访问节点
         options         会话访问属性信息
**************************************************************************/
function setSessionAndcheckAccessNodes ( cl, expAccessNodes, options )
{
   db.setSessionAttr( options );
   var cursor = cl.find().explain();
   var actAccessNodes = [];
   while( cursor.next() )
   {
      var actAccessNode = cursor.current().toObj().NodeName;
      if( actAccessNodes.indexOf( actAccessNode ) === -1 )
      {
         actAccessNodes.push( actAccessNode );
      }
   }
   cursor.close();


   //实际结果与预期结果比较
   for( var i in actAccessNodes )
   {
      if( expAccessNodes.indexOf( actAccessNodes[i] ) === -1 )
      {
         println( "actAccessNodes: " + actAccessNodes + "\nexpAccessNodes: " + expAccessNodes );
         throw new Error( "The actAccessNodes do not include the node: " + expAccessNodes[i] );
      }
   }
}



/************************************************************************
*@Description: find().explain()查看访问计划中访问节点信息，检查访问节点正确性
*@input: dbcl            db.getCS(cs).getCL(cl)
         expAccessNodes  预期访问节点
**************************************************************************/
function findAndCheckAccessNodes ( dbcl, expAccessNodes )
{
   var actAccessNodes = [];
   var cursor = dbcl.find().explain();
   while( cursor.next() )
   {
      var actAccessNode = cursor.current().toObj().NodeName;
      actAccessNodes.push( actAccessNode );
   }
   cursor.close();
   //实际结果与预期结果比较
   for( var i in actAccessNodes )
   {
      if( expAccessNodes.indexOf( actAccessNodes[i] ) === -1 )
      {
         println( "actAccessNodes: " + actAccessNodes + "\nexpAccessNodes: " + expAccessNodes );
         throw new Error( "The actAccessNodes do not include the node: " + expAccessNodes[i] );
      }
   }
}

/************************************************************************
*@Description: get svcname of the data group
*@author:      wuyan
*@createDate:  2018.1.22
**************************************************************************/
function getGroupNodes ( db, groupName )
{
   var groupInfo = db.getRG( groupName ).getDetail().current().toObj().Group;
   var groupNodes = [];
   for( var i in groupInfo )
   {
      var nodeInfo = groupInfo[i].HostName + ":" + groupInfo[i].Service[0].Name;
      groupNodes.push( nodeInfo );
   }
   return groupNodes;
}

/*****************************************************************
*@Description: 检查数据主节点是否存在
*@input: db       
         groupName
******************************************************************/
function checkMasterNodeExist ( db, groupName )
{
   var doTimes = 1;
   var curMaster;
   // 最长等待10min
   while( doTimes <= 600 )
   {
      try
      {
         curMaster = db.getRG( groupName ).getMaster();
         break;
      }
      catch( e )
      {
         if( SDB_RTN_NO_PRIMARY_FOUND != e.message && SDB_CLS_NOT_PRIMARY != e.message )
         {
            throw e;
         }
         doTimes++;
         sleep( 1000 );
      }
   }
   if( doTimes > 600 )
   {
      throw new Error( "Check group has master node timeout" );
   }
}