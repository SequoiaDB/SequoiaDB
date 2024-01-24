/****************************************************
@description:	commlib of FullText		
@Date : 2018-09-18
@Author: liuxiaoxuan
@call：fullText、fulltext_rlb、fullText_sync                
****************************************************/
import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

var cmd = new Cmd();
var HEADER = "'Content-Type: application/json'";
var HTTP = "'http://" + ESHOSTNAME + ":" + ESSVCNAME;
var esOpr = new ESOperator();
var dbOpr = new DBOperator();
// create WORKDIR in local host
// CI测试机和执行机分离，调整为在COORDHOSTNAME机器上创建WORKDIR，后面的checkInspectResult应需要同步调整
commMakeDir( COORDHOSTNAME, WORKDIR );

/******************************************************************************
*@Description : do some operations related to ES, such as:
                do queries by rest
                check index sync to ES
@usage:         var es = new ESOperator();
                var esRecofindFromES( "esIndexName", querycond );
                es.checkFullSyncToES("esIndexName", expectCount, cappedCL)
******************************************************************************/
function ESOperator ()
{
   /*****************************************************************
   * run CURL command, to get return records from elasticsearch by rest
   *****************************************************************/
   this.findFromES = function( esIndexName, queryCond )
   {
      var records = new Array();
      // get curl command
      var str = "curl -H " + HEADER + " -XGET " + HTTP + "/" + esIndexName + "/_search' -d '" + queryCond + "' 2>/dev/null";
      // to get records from ES
      var info = cmd.run( str );
      //get json
      var json = eval( "(" + info + ")" );
      var array = json["hits"]["hits"];
      for( var i = 0; i < array.length; i++ )
      {
         var _id = array[i]["_id"];
         if( _id == "SDBCOMMIT" ) continue;
         var obj = array[i]["_source"];
         records.push( obj );
      }
      return records;
   }

   /*****************************************************************
   * run CURL command, to get count from elasticsearch by rest
   *****************************************************************/
   this.countFromES = function( esIndexName )
   {
      var count = 0;
      // get curl command
      var str = "curl -H " + HEADER + " -XGET " + HTTP + "/" + esIndexName + "/_count" + "' 2>/dev/null";

      // get count from ES
      var info = cmd.run( str );
      //get json
      var json = eval( "(" + info + ")" );
      count = json["count"];
      return count;
   }

   /*****************************************************************
   * run CURL command, to get SDBCOMMITID from elasticsearch by rest  
   *****************************************************************/
   this.getCommitIDFromES = function( esIndexName )
   {
      var commitID = -1;

      var querySdbCommitID = "{\"query\" : {\"match\" : {\"_id\": \"SDBCOMMIT\"}}}";
      // get curl command
      var str = "curl -H " + HEADER + " -XGET " + HTTP + "/" + esIndexName + "/_search' -d '" + querySdbCommitID + "' 2>/dev/null";

      // to get SDBCOMMITID from ES
      var info = cmd.run( str );
      //get json
      var json = eval( "(" + info + ")" );
      var array = json["hits"]["hits"];
      if( array.length == 1 )
      {
         commitID = array[0]["_source"]["_lid"];
      }
      return commitID;
   }

   /*****************************************************************
   * run CURL command, to refresh ES after check sync 
   *****************************************************************/
   this.refreshFromES = function( esIndexName )
   {
      // get curl command
      var str = "curl -H " + HEADER + " -XPOST " + HTTP + "/" + esIndexName + "/_refresh' 2>/dev/null";

      // to refresh shards from ES
      cmd.run( str );
   }

   /*****************************************************************
   * check if index is create in elasticsearch      
   *****************************************************************/
   this.isCreateIndexInES = function( esIndexName )
   {
      // get curl command
      var str = "curl -H " + HEADER + " -XGET " + HTTP + "/" + esIndexName + "' 2>/dev/null";

      //the longest waiting time is 300s
      var isExist = false;
      var timeout = 300;
      var doTimes = 0;
      while( doTimes < timeout )
      {
         var info = cmd.run( str );
         //get json
         var json = eval( "(" + info + ")" );
         var error = json["error"];
         if( typeof ( error ) == "undefined" )
         {
            isExist = true;
            break;
         }
         else
         {
            sleep( 1000 );
            doTimes++;
         }
      }
      return isExist;
   }

   /*****************************************************************
   * check if index is drop in elasticsearch      
   *****************************************************************/
   this.isDropIndexInES = function( esIndexName )
   {
      // get curl command
      var str = "curl -H " + HEADER + " -XGET " + HTTP + "/" + esIndexName + "' 2>/dev/null";

      //the longest waiting time is 300s
      var isExist = true;
      var timeout = 300;
      var doTimes = 0;
      while( doTimes < timeout )
      {
         var info = cmd.run( str );
         //get json
         var json = eval( "(" + info + ")" );
         var error = json["error"];
         if( typeof ( error ) != "undefined" )
         {
            isExist = false;
            break;
         }
         else
         {
            sleep( 1000 );
            doTimes++;
         }
      }
      return isExist;
   }
}

/******************************************************************************
*@Description : get db datas, such as:
                get clname
                get query results
@usage:         var db = new DBOperator();
                var cappedCLName = db.getCappedCLName( dbcl, "textIndexName" );
******************************************************************************/
function DBOperator ()
{
   /*****************************************************************
   * get cappedcl name 
   *****************************************************************/
   this.getCappedCLName = function( dbcl, textIndexName )
   {
      var cappedCLName = "";
      var idx = dbcl.getIndex( textIndexName );
      cappedCLName = idx.toObj().ExtDataName;
      return cappedCLName;
   }


   /*****************************************************************
   * get cappedcl  
   *****************************************************************/
   this.getCappedCLs = function( csName, clName, textIndexName )
   {
      var clFullName = csName + "." + clName;
      var dbcl = db.getCS( csName ).getCL( clName );
      var cappedCLName = this.getCappedCLName( dbcl, textIndexName );
      var clGroups = commGetCLGroups( db, clFullName );
      // sort groupname, in order to mapping esIndexNames <-> cappedCLs
      clGroups = removeDuplicateItems( clGroups );
      clGroups.sort();
      // get each cappedCL from each group
      var cappedCLs = new Array();
      for( var i in clGroups )
      {
         var cappedCL = db.getRG( clGroups[i] ).getMaster().connect().getCS( cappedCLName ).getCL( cappedCLName );
         cappedCLs.push( cappedCL );
      }
      return cappedCLs;
   }

   /*****************************************************************
   * get es index name, rule: 
   * cappedCLName: SYS_uniqueId_textIndexName  
   * esIndexName:  indexPrefix + sys_uniqueId_textIndexName_clGroupName							  
   *****************************************************************/
   this.getESIndexNames = function( csName, clName, textIndexName )
   {
      // check cappedcl name is valid
      var dbcl = db.getCS( csName ).getCL( clName );
      var cappedCLName = this.getCappedCLName( dbcl, textIndexName );

      // get es index names
      var esIndexNames = new Array();
      var clGroupNames = commGetCLGroups( db, csName + "." + clName );
      clGroupNames = removeDuplicateItems( clGroupNames );
      // sort groupname, in order to mapping esIndexNames <-> cappedCLs
      clGroupNames.sort();
      for( var i in clGroupNames )
      {
         esIndexNames.push( FULLTEXTPREFIX.toLowerCase() + cappedCLName.toLowerCase() + "_" + clGroupNames[i] );
      }

      // if sharding cl, return all indices
      return esIndexNames;
   }

   /*****************************************************************
   * get last _id from cappedCL, in order to compare with ES's SDBCOMMITID 
   * and ensure that records are all sync to ES
   *****************************************************************/
   this.getLastLID = function( cappedCL )
   {
      var lastLogicalID = -1;
      var sortCond = { "_id": 1 };
      var records = this.findFromCL( cappedCL, null, null, sortCond, null );
      if( records.length > 0 )
      {
         lastLogicalID = records[records.length - 1]["_id"];
      }

      return lastLogicalID;
   }

   /*****************************************************************
   * find records by options
   *****************************************************************/
   this.findFromCL = function( dbcl, findCond, selectorCond, sortCond, hintCond, limitCond, skipCond )
   {
      if( typeof ( selectorCond ) == "undefined" ) { selectorCond = null; }
      if( typeof ( findCond ) == "undefined" ) { findCond = null; }
      if( typeof ( sortCond ) == "undefined" ) { sortCond = null; }
      if( typeof ( hintCond ) == "undefined" ) { hintCond = null; }
      if( typeof ( limitCond ) == "undefined" ) { limitCond = null; }
      if( typeof ( skipCond ) == "undefined" ) { skipCond = null; }

      //find({"":{"$Text":{"query":{"match":{"a" : "test"}}}}}) 
      var rc = dbcl.find( findCond, selectorCond ).sort( sortCond ).hint( hintCond ).limit( limitCond ).skip( skipCond );

      var records = new Array();
      //get all records
      while( rc.next() )
      {
         var record = rc.current().toObj();
         records.push( record );
      }

      return records;
   }
}

/*****************************************************************
@description:   check records all sync to elasticsearch by comparing count and lid       
@input:         csName
                clName
                expectCount
******************************************************************/
function checkFullSyncToES ( csName, clName, textIndexName, expectCount )
{
   var esIndexNames = dbOpr.getESIndexNames( csName, clName, textIndexName );
   var cappedCLs = dbOpr.getCappedCLs( csName, clName, textIndexName );

   // check indexnames sync to ES
   for( var i in esIndexNames )
   {
      if( !esOpr.isCreateIndexInES( esIndexNames[i] ) )
      {
         throw new Error( "checkFullSyncToES() index name:" + esIndexNames[i] + " not exsit" );
      }
   }

   // check all indices sync to ES
   checkCountInES( esIndexNames, expectCount );
   checkLidInES( esIndexNames, cappedCLs );
   // refresh ES after check sync
   for( var i in esIndexNames )
   {
      esOpr.refreshFromES( esIndexNames[i] );
   }
}

/*****************************************************************
@description:   check records of MainCL all sync to elasticsearch by comparing count and lid       
@input:         csName
                clName
                expectCount
******************************************************************/
function checkMainCLFullSyncToES ( csName, mainCLName, textIndexName, expectCount )
{
   var subCLNames = new Array();
   var cursor = db.snapshot( 8, { Name: csName + "." + mainCLName } );
   while( cursor.next() )
   {
      var cateInfos = cursor.current().toObj().CataInfo;
      for( var i = 0; i < cateInfos.length; i++ )
      {
         subCLNames.push( cateInfos[i]['SubCLName'] );
      }
   }

   // get all indexs from maincl 
   var esIndexNames = new Array();
   var cappedCLs = new Array();
   for( var i in subCLNames )
   {
      var subCSName = subCLNames[i].split( '.' )[0];
      var subCLName = subCLNames[i].split( '.' )[1];

      esIndexNames = esIndexNames.concat( dbOpr.getESIndexNames( subCSName, subCLName, textIndexName ) );
      cappedCLs = cappedCLs.concat( dbOpr.getCappedCLs( subCSName, subCLName, textIndexName ) );
   }

   // check full sync to ES for each subcl
   for( var i in esIndexNames )
   {
      if( !esOpr.isCreateIndexInES( esIndexNames[i] ) )
      {
         throw new Error( "checkMainCLFullSyncToES() index name " + esIndexNames[i] + " is not exsit" );
      }
   }

   checkCountInES( esIndexNames, expectCount );
   checkLidInES( esIndexNames, cappedCLs );
   // refresh ES after check sync
   for( var i in esIndexNames )
   {
      esOpr.refreshFromES( esIndexNames[i] );
   }
}


/*****************************************************************
@description:   check records is full sync to elasticsearch by comparing count         
@input:         csName
                clName
                expectCount    
******************************************************************/
function checkCountInES ( esIndexNames, expectCount )
{
   //the longest waiting time is 600S
   var isSync = false;
   var timeout = 600;
   var doTimes = 0;

   while( doTimes < timeout )
   {
      // clear count every time
      var actCount = 0;
      // Add counts of all indices 
      for( var i in esIndexNames )
      {
         actCount += ( esOpr.countFromES( esIndexNames[i] ) - 1 );
      }
      // if expect count < act count, exit
      if( actCount == expectCount )
      {
         isSync = true;
         break;
      } else
      {
         sleep( 1000 );
         doTimes += 1;
      }

   }

   if( !isSync )
   {
      throw new Error( "checkCountInES() check ES sync record failed, expect record num:" + expectCount + ",actual record num:" + actCount );
   }

   return isSync;
}

/*****************************************************************
@description:   check records is sync to elasticsearch by comparing lastLogicalID and SDBCOMMITID          
@input:         csName
                clName 
******************************************************************/
function checkLidInES ( esIndexNames, cappedCLs )
{

   // if esIndexNames not mapping to cappedCLs, fail
   if( esIndexNames.length !== cappedCLs.length )
   {
      throw new Error( "checkLidInES() index not sync to es:" + esIndexNames.length + ", the number of index name in db: " + cappedCLs.length );
   }

   //the longest waiting time is 600S
   var isSync = false;
   var timeout = 600;
   var doTimes = 0;

   // get all lids from all groups
   var lastLogicalIDs = new Array();
   for( var i in cappedCLs )
   {
      lastLogicalIDs.push( dbOpr.getLastLID( cappedCLs[i] ) );
   }
   while( doTimes < timeout )
   {
      // get all commitids from all esIndexNames 
      var commitIDs = new Array();
      for( var i in esIndexNames )
      {
         commitIDs.push( esOpr.getCommitIDFromES( esIndexNames[i] ) );
      }

      // check if all indices finish sync
      for( var i in esIndexNames )  
      {
         isSync = false;
         if( commitIDs[i] === lastLogicalIDs[i] )
         {
            isSync = true;
         } else
         {
            sleep( 1000 );
            doTimes++;
            break;
         }
      }

      if( isSync )
      {
         break;
      }
   }
   if( !isSync )
   {
      throw new Error( "checkLidInES() expect lid: " + commitIDs + ",actual lid: " + lastLogicalIDs );

   }
}

/*****************************************************************
@description:   check index in elasticsearch not exist       
@input:         csName
                clName
                textIndexName
******************************************************************/
function checkIndexNotExistInES ( esIndexNames )
{
   // check indexnames in ES not exist
   for( var i in esIndexNames )
   {
      if( esOpr.isDropIndexInES( esIndexNames[i] ) )
      {
         throw new Error( "checkIndexNotExistInES() index name: " + esIndexNames[i] + "is not exists." );
      }
   }
}

/*****************************************************************
@description:   check result        
@input:         expectResult
                actResult
******************************************************************/
function checkResult ( expectResult, actResult )
{
   if( expectResult.length !== actResult.length )
   {
      throw new Error( "checkResult() check recordNum failed, expectNum: " + expectResult.length + ",actualNum: " + actResult.length );
   }

   // compare array  
   for( var i in expectResult )
   {
      var actRec = actResult[i];
      var expRec = expectResult[i];

      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) ) 
         {
            throw new Error( "checkResult() check record failed, expect record: " + JSON.stringify( expRec ) + ",actual record: " + JSON.stringify( actRec ) );
         }
      }
   }

   for( var j in actResult )
   {
      var actRec = actResult[j];
      var expRec = expectResult[j];

      for( var f in actRec )
      {
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            throw new Error( "checkResult() check record failed, expect record: " + JSON.stringify( expRec ) + ",actual record: " + JSON.stringify( actRec ) );
         }
      }
   }

}

/*****************************************************************
@description:   sort object in array, rg:
                array.sort(compare("key1", compare("key2", compare("key3"))));       
@input:         key
                o: object1's key
                p: object2's key					 
******************************************************************/
function compare ( name, minor )
{
   return function( o, p )
   {
      var a, b;
      if( o && p && typeof o === 'object' && typeof p === 'object' )
      {
         a = o[name];
         b = p[name];
         if( a === b )
         {
            return typeof minor === 'function' ? minor( o, p ) : 0;
         }
         if( typeof a === typeof b )
         {
            return a < b ? -1 : 1;
         }
         return typeof a < typeof b ? -1 : 1;
      } else
      {
         throw new Error( "compare() other error" );
      }
   }
}

/*****************************************************************
*@Description : check consistency: LSNs consistency of all nodes, ES consistency
@input:         csName
                clName
******************************************************************/
function checkConsistency ( csName, clName )
{
   // check LSN consistency
   if( csName == null ) { var csName = "UNDEFINED"; }
   if( clName == null ) { var clName = "UNDEFINED"; }

   var groups = commGetCLGroups( db, csName + "." + clName );

   //the longest waiting time is 600S
   var lsnFlag = false;
   var timeout = 600;
   var doTimes = 0;

   //get primary nodes
   var primaryNodeLSNs = getPrimaryNodeLSNs( groups );

   while( doTimes < timeout )
   {
      lsnFlag = checkLSN( groups, primaryNodeLSNs );
      if( lsnFlag )
      {
         break;
      } else
      {
         sleep( 1000 );
         doTimes++;
      }
   }

   if( !lsnFlag )
   {
      throw new Error( "checkConsistency() check lsn failed on groups: " + groups );
   }

}

/*****************************************************************
*@Description: check lsn consistency
@input:         groups
                primaryNodeLSNs
******************************************************************/
function checkLSN ( groups, primaryNodeLSNs )
{
   var slaveNodeLSNs = getSlaveNodeLSNs( groups );

   //比较主备节点lsn
   for( var i = 0; i < slaveNodeLSNs.length; ++i )
   {
      // 如果主节点不存在，则直接返回false
      if( primaryNodeLSNs[i].length == 0 )
      {
         return false;
      }

      for( var j = 0; j < slaveNodeLSNs[i].length; ++j )
      {
         if( primaryNodeLSNs[i][0] > slaveNodeLSNs[i][j] )
         {
            return false;
         }
      }
   }

   return true;
}

/*****************************************************************
*@Description: get lsn of primary node
@input:        groups
******************************************************************/
function getPrimaryNodeLSNs ( groups )
{

   var datas = getNodesInGroups( groups );

   var LSNs = new Array();
   for( var i = 0; i < datas.length; ++i )
   {
      var nodesInGroup = datas[i];
      LSNs[i] = Array();
      for( var j = 0; j < nodesInGroup.length; ++j )
      {
         var getSnapshot6 = eval( "(" + nodesInGroup[j].snapshot( 6 ).toArray()[0] + ")" );

         var completeLSN = getSnapshot6.CompleteLSN;
         var isPrimary = getSnapshot6.IsPrimary;
         if( isPrimary )
         {
            LSNs[i][0] = completeLSN;
            break;
         }
      }
   }

   return LSNs;
}

/*****************************************************************
*@Description: get lsn of slave node
@input:        groups
******************************************************************/
function getSlaveNodeLSNs ( groups )
{
   var datas = getNodesInGroups( groups );

   var LSNs = new Array();
   for( var i = 0; i < datas.length; ++i )
   {
      var nodesInGroup = datas[i];
      LSNs[i] = Array();
      var f = 0;
      for( var j = 0; j < nodesInGroup.length; ++j )
      {
         var getSnapshot6 = eval( "(" + nodesInGroup[j].snapshot( 6 ).toArray()[0] + ")" );

         var completeLSN = getSnapshot6.CompleteLSN;
         var isPrimary = getSnapshot6.IsPrimary;
         if( !isPrimary )
         {
            LSNs[i][f++] = completeLSN;
         }
      }
   }

   return LSNs;
}

/*****************************************************************
*@Description: get all nodes of all groups
@input:        groups
******************************************************************/
function getNodesInGroups ( groups )
{
   var datas = new Array();
   var hostName;
   var serviceName;
   //standalone
   if( true === commIsStandalone( db ) )
   {
      datas[0] = Array();
      datas[0][0] = db;
   }
   else
   {
      for( var i = 0; i < groups.length; ++i )
      {
         datas[i] = Array();

         var rg = db.getRG( groups[i] );
         var rgDetail = eval( "(" + rg.getDetail().toArray()[0] + ")" );
         var nodesInGroup = rgDetail.Group;
         for( var j = 0; j < nodesInGroup.length; ++j )
         {
            hostName = nodesInGroup[j].HostName;
            serviceName = nodesInGroup[j].Service[0].Name;
            datas[i][j] = new Sdb( hostName, serviceName );
         }
      }
   }
   return datas;
}

/******************************************************************************
*@Description : check inpect result
@input:         csName
                clName
                checkTimes
******************************************************************************/
function checkInspectResult ( csName, clName, checkTimes )
{
   if( typeof ( checkTimes ) == "undefined" ) { checkTimes = 5; }

   var inspectBinFile = WORKDIR + "/" + "inspect_" + csName + "_" + clName + ".bin";
   var inspectReportFile = WORKDIR + "/" + "inspect_" + csName + "_" + clName + ".bin.report";
   var installPath = commGetInstallPath();
   var inspectCommand = installPath + "/bin/sdbinspect" + " -d " + COORDHOSTNAME + ":" + COORDSVCNAME + " -c " + csName + " -l " + clName + " -o " + inspectBinFile + " -t " + checkTimes;
   // exec sdbinspect 
   cmd.run( inspectCommand );
   var info = cmd.run( "tail -n 1 " + inspectReportFile );
   var actResult = info.split( "\n" )[0].split( "\:" )[1].trim();
   var expectRusult = "exit with no records different";
   // compare result
   if( actResult != expectRusult )
   {
      throw new Error( "check consistency with csName:" + csName + ",clName:" + clName + "failed" );
   }
   // remove report files
   cmd.run( "rm -f " + inspectBinFile );
   cmd.run( "rm -f " + inspectReportFile );
}

/******************************************************************************
*@Description : remove duplicate items in array
@input:         array
******************************************************************************/
function removeDuplicateItems ( array )
{
   var uniqueArray = new Array();
   for( var i in array )
   {
      if( uniqueArray.indexOf( array[i] ) == -1 )
      {
         uniqueArray.push( array[i] );
      }
   }
   return uniqueArray;
}

/*****************************************************************
*@Description: 检查数据组的所有节点是否正常，且主备LSN是否一致
@input:        timeoutSecond
               csName
               clName
******************************************************************/
function checkGroupBusiness ( timeoutSecond, csName, clName )
{
   var groupNames = commGetCLGroups( db, csName + "." + clName );
   for( var i in groupNames )
   {
      var doTimes = 1;
      while( doTimes <= timeoutSecond )
      {
         if( !isSuccesscreateTestCollection( groupNames[i] ) || !isNodesNormal( groupNames[i] ) )
         {
            doTimes++;
            sleep( 1000 );
         }
         else 
         {
            break;
         }
      }

      if( doTimes > timeoutSecond )
      {
         throw new Error( "check group bussiness timeout." );
      }
   }

   //  校验主备节点LSN
   doTimes = 1;
   while( doTimes <= timeoutSecond )
   {
      var primaryNodeLSNs = getPrimaryNodeLSNs( groupNames );
      if( !checkLSN( groupNames, primaryNodeLSNs ) )
      {
         doTimes++;
         sleep( 1000 );
      }
      else 
      {
         break;
      }
   }

   if( doTimes > timeoutSecond )
   {
      throw new Error( "check group business timeout." );
   }
}

/*****************************************************************
*@Description: 指定数据组、强一致性创建集合
@input:        groupName
******************************************************************/
function isSuccesscreateTestCollection ( groupName )
{
   var clName = "clForTestBusiness_reliability_js";
   try
   {
      commDropCL( db, COMMCSNAME, clName, true, true );
      var dbcl = commCreateCL( db, COMMCSNAME, clName, { "ReplSize": 0, "Group": groupName }, false, false );
      return true;
   }
   catch( e )
   {
      return false;
   }
   finally
   {
      commDropCL( db, COMMCSNAME, clName, true, true );
   }
}


/*****************************************************************
*@Description: 检查catalog的主备LSN是否一致
@input:        csName
               clName
******************************************************************/
function checkCatalogBusiness ( timeoutSecond )
{
   // 1. 首先检查所有节点是否能正常连接
   var doTimes = 1;
   while( doTimes <= timeoutSecond )
   {
      if( !isNodesNormal( "SYSCatalogGroup" ) )
      {
         doTimes++;
         sleep( 1000 );
      }
      else 
      {
         break;
      }
   }

   if( doTimes > timeoutSecond )
   {
      throw new Error( "check catalog nodes normal timeout." );
   }

   // 2. 校验主备节点LSN
   doTimes = 1;
   while( doTimes <= timeoutSecond )
   {
      var primaryNodeLSNs = getPrimaryNodeLSNs( ["SYSCatalogGroup"] );
      if( !checkLSN( ["SYSCatalogGroup"], primaryNodeLSNs ) )
      {
         doTimes++;
         sleep( 1000 );
      }
      else 
      {
         break;
      }
   }

   if( doTimes > timeoutSecond )
   {
      throw new Error( "check catalog business timeout." );
   }
}

/*****************************************************************
*@Description: 检查连接节点是否正常
@input:        groupName
******************************************************************/
function isNodesNormal ( groupName )
{
   try
   {
      var rg = db.getRG( groupName );
      var rgDetail = eval( "(" + rg.getDetail().toArray()[0] + ")" );
      var nodesInGroup = rgDetail.Group;
      for( var i = 0; i < nodesInGroup.length; ++i )
      {
         var hostName = nodesInGroup[i].HostName;
         var serviceName = nodesInGroup[i].Service[0].Name;
         new Sdb( hostName, serviceName );
      }
      return true;
   }
   catch( e )
   {
      if( SDB_CLS_NOT_PRIMARY != e.message && SDB_NET_CANNOT_CONNECT != e.message && SDB_COORD_REMOTE_DISC != e.message )
      {
         throw e;
      }
      return false;
   }
}

/*****************************************************************
*@Description: 检查数据主节点是否存在
@input:        groupName
******************************************************************/
function isMasterNodeExist ( groupName )
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
   return curMaster;
}

/* *****************************************************************************
@description: drop collection
              删除集合
@author: Zhao Xiaoni 
@parameter
   ignoreCSNotExist: default = true, value: true/false, 忽略集合空间不存在错误
   ignoreCLNotExist: default = true, value: true/false, 忽略集合不存在错误
   message: default = ""
规避-147问题，当适配器向es同步时间时删除集合会报错-147
***************************************************************************** */
function dropCL ( db, csName, clName, ignoreCSNotExist, ignoreCLNotExist, message )
{
   if( message == undefined ) { message = ""; }
   if( ignoreCSNotExist == undefined ) { ignoreCSNotExist = true; }
   if( ignoreCLNotExist == undefined ) { ignoreCLNotExist = true; }

   var timeout = 120;
   var doTimes = 0;
   while( doTimes < timeout )
   {
      try
      {
         db.getCS( csName ).dropCL( clName );
         break;
      }
      catch( e )
      {
         doTimes++;
         if( ( commCompareErrorCode( e, SDB_DMS_CS_NOTEXIST ) && ignoreCSNotExist ) || ( commCompareErrorCode( e, SDB_DMS_NOTEXIST ) && ignoreCLNotExist ) )
         {
            break;
         }
         else if( e.message == SDB_LOCK_FAILED )
         {
            sleep( 1000 );
         }
         else
         {
            commThrowError( e, "commDropCL[" + funcCommDropCLTimes + "] Drop collection[" + csName + "." + clName + "] failed: " + e + ",message: " + message )
         }
      }
   }
}


/* *****************************************************************************
@description: drop collection space
              删除集合空间
@author: Zhao Xiaoni 
@parameter
   ignoreNotExist: default = true, value: true/false, 忽略不存在错误
   message: default = ""
重写此方法，规避-147问题
***************************************************************************** */
function dropCS ( db, csName, ignoreNotExist, message )
{
   if( ignoreNotExist == undefined ) { ignoreNotExist = true; }
   if( message == undefined ) { message = ""; }

   var timeout = 120;
   var doTimes = 0;
   while( doTimes < timeout )
   {
      try
      {
         db.dropCS( csName );
         break;
      }
      catch( e )
      {
         doTimes++;
         if( commCompareErrorCode( e, SDB_DMS_CS_NOTEXIST ) && ignoreNotExist )
         {
            break;
         }
         else if( e.message == SDB_LOCK_FAILED )
         {
            sleep( 1000 );
         }
         else
         {
            commThrowError( e, "commDropCS[" + funcCommDropCSTimes + "] Drop collection space[" + csName + "] failed: " + e + ",message: " + message )
         }
      }
   }
}

/* *****************************************************************************
@description: drop index
              删除索引
@author: Zhao Xiaoni
@parameter
   cl: 集合
   name: 索引名 
   ignoreNotExist: default is false, 忽略索引不存在错误
***************************************************************************** */
function dropIndex ( cl, name, ignoreNotExist )
{
   if( ignoreNotExist == undefined ) { ignoreNotExist = true; }

   var timeout = 120;
   var doTimes = 0;
   while( doTimes < timeout )
   {
      try
      {
         cl.dropIndex( name );
         break;
      }
      catch( e )
      {
         doTimes++;
         if( ignoreNotExist && commCompareErrorCode( e, SDB_IXM_NOTEXIST ) )
         {
            break;
         }
         else if( e.message == SDB_LOCK_FAILED )
         {
            sleep( 1000 );
         }
         else
         {
            commThrowError( e, "commDropIndex: drop index[" + name + "] failed: " + e );
         }
      }
   }
}
