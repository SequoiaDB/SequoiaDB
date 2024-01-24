/*******************************************************************************
@Description : Create Index common functions
@Modify list :
               2014-5-20  xiaojun Hu  Init
*******************************************************************************/
import( "../lib/main.js" );

var csName = COMMCSNAME;
var clName = COMMCLNAME;

// common functions
function createIndex ( cl, idxName, idxKeygen, unique, enforced, errno )
{
   if( undefined == unique ) { unique = false; }
   if( undefined == enforced ) { enforced = false; }
   if( undefined == errno ) { errno = ""; }
   try
   {
      if( undefined == cl || undefined == idxName || undefined == idxKeygen )
      {
         throw new Error( "ErrArg" );
      }
      cl.createIndex( idxName, idxKeygen, unique, enforced );
      // inspect the index we created
   }
   catch( e )
   {
      if( errno != e.message )
      {
         throw e;
      }
   }
}

//inspect the index is created success or not.
function inspecIndex ( cl, indexName, indexKey, keyValue, idxUnique, idxEnforced )
{
   if( undefined == idxUnique ) { idxUnique = false; }
   if( undefined == idxEnforced ) { idxEnforced = false; }
   if( undefined == cl || undefined == indexName || undefined == indexKey || undefined == keyValue )
   {
      throw new Error( "ErrArg" );
   }
   var getIndex = new Boolean( true );
   try
   {
      getIndex = cl.getIndex( indexName );
   }
   catch( e )
   {
      getIndex = undefined;
   }
   var cnt = 0;
   while( cnt < 20 )
   {
      try
      {
         getIndex = cl.getIndex( indexName );
      }
      catch( e )
      {
         getIndex = undefined;
      }
      if( undefined != getIndex )
      {
         break;
      }
      ++cnt;
   }
   if( undefined == getIndex )
   {
      throw new Error( "ErrIdxName" );
   }
   var indexDef = getIndex.toString();
   indexDef = eval( '(' + indexDef + ')' );
   var index = indexDef["IndexDef"];

   assert.equal( keyValue, index["key"][indexKey] );
   assert.equal( idxUnique, index["unique"] );
   assert.equal( idxEnforced, index["enforced"] );
}

/************************************
*@Description: bulk insert data
*@author:      wuyan
*@createDate:  2021.04.02
**************************************/
function insertBulkData ( dbcl, recordNum, recordStart, recordEnd )
{
   if( undefined == recordStart ) { recordStart = 0; }
   if( undefined == recordEnd ) { recordEnd = recordNum; }
   try
   {
      var doc = [];
      for( var i = 0; i < recordNum; i++ )
      {
         var bValue = recordStart + parseInt( Math.random() * ( recordEnd - recordStart ) );
         doc.push( { a: i, b: bValue, c: i, no: i } );
      }
      dbcl.insert( doc );
      println( "---bulk insert data success" );
   }
   catch( e )
   {
      throw buildException( "insertBulkData()", e, "insert", "insert data :" + JSON.stringify( doc ), "insert fail" );
   }
   return doc;
}

/****************************************************
@description: check the scanType of the explain
@modify list:
              2016-3-3 yan WU init
****************************************************/
function checkExplain ( CL, keyValue, expectType, expIndexName )
{
   if( undefined == expectType ) { var expectType = "tbscan"; }
   if( undefined == expIndexName ) { var expIndexName = ""; }

   var listIndex = CL.find( keyValue ).explain().current().toObj();
   var scanType = listIndex.ScanType;
   var indexName = listIndex.IndexName;
   assert.equal( scanType, expectType );
   assert.equal( indexName, expIndexName );
}

/****************************************************
@description: check the scanType of the explain for maincl
@modify list:
              2021-4-2 yan WU init
****************************************************/
function checkExplainByMaincl ( maincl, cond, expectType, expIndexName )
{
   if( undefined == expectType ) { var expectType = "tbscan"; }
   if( undefined == expIndexName ) { var expIndexName = ""; }

   listIndex = maincl.find( cond ).explain().current().toObj();
   var scanType = listIndex["SubCollections"][0].ScanType;
   var indexName = listIndex["SubCollections"][0].IndexName;
   assert.equal( scanType, expectType );
   assert.equal( indexName, expIndexName );
}

function sortBy ( field )
{
   return function( a, b )
   {
      return a[field] > b[field];
   }
}

/****************************************************
@description: check the result of query
@modify list:
              2016-3-3 yan WU init
****************************************************/
function checkResult ( idxCL, keyValue )
{
   var rc = idxCL.find( keyValue );
   var expRecs = [];
   expRecs.push( keyValue );
   checkRec( rc, expRecs );
}

function checkRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   //check count
   assert.equal( actRecs.length, expRecs.length );

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         assert.equal( actRec[f], expRec[f] );
      }
   }
}

/******************************************************************************
 * @description: 检查集合中索引是否打开NotArray
 * @param {*}
 * @return {*}
 ******************************************************************************/
function checkNotArray ( cl, idxname, expResult )
{
   var index = cl.getIndex( idxname );
   var notArray = index.toObj().IndexDef.NotArray;
   assert.equal( expResult, notArray );
}

/******************************************************************************
 * @description: 检查分析结果是否有索引覆盖
 * @param {*} explain
 * @param {*} expResult
 * @return {*}
 ******************************************************************************/
function checkIndexCover ( explain, expResult )
{
   while( explain.next() )
   {
      var result = explain.current().toObj();
      var actResult = result.IndexCover;
      assert.equal( expResult, actResult );
   }
}

function getConfig ( rgName, fieldName )
{
   var master = db.getRG( rgName ).getMaster();
   var mstHostName = master.getHostName();
   var mstSvcName = master.getServiceName();
   var nodeName = mstHostName + ":" + mstSvcName;
   var fieldValue = "";
   if( fieldName === "indexcoveron" )
   {
      var cursor = db.snapshot( SDB_SNAP_CONFIGS, { NodeName: nodeName }, { indexcoveron: "" } );
      fieldValue = cursor.next().toObj().indexcoveron;
   }
   else if( fieldName === "mvccon" )
   {
      var cursor = db.snapshot( SDB_SNAP_CONFIGS, { NodeName: nodeName }, { mvccon: "" } );
      fieldValue = cursor.next().toObj().mvccon;
   }
   else if( fieldName === "transisolation" )
   {
      var cursor = db.snapshot( SDB_SNAP_CONFIGS, { NodeName: nodeName }, { transisolation: "" } );
      fieldValue = cursor.next().toObj().transisolation;
   }
   return fieldValue;
}

/******************************************************************************
 * @description: 等待任务执行完成
 * @param {string} csName  
 * @param {string} clName
 * @param {string} taskTypeDesc  //任务类型 
 * @param {int} num           //需要检测的任务数量，不填默认为1
 * @return {*}
 ******************************************************************************/
function waitTaskFinish ( csName, clName, taskTypeDesc, taskNum )
{
   if( taskNum == undefined ) { taskNum = 1 };

   //status取值 0:Ready	9:Finish
   var status = 9;
   var times = 0;
   var sleepTime = 100;
   var maxWaitTimes = 20000;

   while( true )
   {
      var cursor = db.listTasks( { "Name": csName + '.' + clName, TaskTypeDesc: taskTypeDesc } );
      var finshTaskNum = 0;
      var taskInfo = "";
      while( cursor.next() )
      {
         taskInfo = cursor.current().toObj();
         if( taskInfo.Status != status )
         {
            break;
         }
         finshTaskNum++;
      }
      cursor.close();

      if( finshTaskNum == taskNum )
      {
         break;
      }
      else if( times * sleepTime >= maxWaitTimes )
      {
         throw new Error( "waiting task time out! waitTimes=" + times * sleepTime + "\nfinshTaskNum=" + finshTaskNum + ",exp taskNum=" + taskNum
            + "\ntask=" + JSON.stringify( taskInfo ) );
      }

      sleep( sleepTime );
      times++;
   }
}

/******************************************************************************
 * @description: 检查copy任务信息（选择copy子表名、索引名、组名、结果状态码比较）
 * @param {string} csName  //主表所在cs
 * @param {string} mainclName  //主表名
 * @param {string/array} subclNames  //子表全名
 * @param {array} indexNames    //检验测索引名,复制索引操作需要传入索引名数组
 * @param {int} resultCode       //任务错误码 成功:0 失败:错误码 
 * @return {*}
 ******************************************************************************/
function checkCopyTask ( csName, mainclName, indexNames, subclNames, resultCode )
{
   if( undefined == resultCode ) { resultCode = 0; }
   if( typeof ( indexNames ) == "string" ) { indexNames = [indexNames]; }
   if( typeof ( subclNames ) == "string" ) { subclNames = [subclNames]; }

   var cursor = db.listTasks( { "Name": csName + '.' + mainclName, TaskTypeDesc: "Copy index" } );
   var taskInfos = [];
   var tastNum = 0;
   while( cursor.next() )
   {
      var taskInfo = cursor.current().toObj();
      taskInfos.push( taskInfo );
      tastNum++;
   }
   cursor.close();

   //校验索引名
   if( !commCompareObject( taskInfo.IndexNames.sort(), indexNames.sort() ) ) 
   {
      throw new Error( "check indexNames error! " + JSON.stringify( taskInfos ) );
   }

   //校验结果状态码
   if( taskInfo.ResultCode !== resultCode )
   {
      throw new Error( "check resultCode error! " + JSON.stringify( taskInfos ) );
   }

   //校验copy子表信息
   if( !commCompareObject( taskInfo.CopyTo.sort(), subclNames.sort() ) )
   {
      throw new Error( "check copyto subclNames error! " + JSON.stringify( taskInfos ) );
   }

   //校验任务数量为1
   if( tastNum !== 1 )
   {
      throw new Error( "check tasknum error! taskNum =  " + tastNum + ", taskinfo=" + JSON.stringify( taskInfos ) );
   }

   //校验子表组信息   
   var actGroupNames = [];
   var groups = taskInfo.Groups;
   for( var i = 0; i < groups.length; i++ )
   {
      actGroupNames.push( groups[i].GroupName );
   }

   var clGroupNames = [];
   for( var i = 0; i < subclNames.length; i++ )
   {
      var subclName = subclNames[i];
      var groupNames = commGetCLGroups( db, subclName );
      clGroupNames = clGroupNames.concat( groupNames );
   }

   //去掉重复的组名     
   clGroupNames.sort();
   var expGroupNames = [clGroupNames[0]];
   for( var i = 1; i < clGroupNames.length; i++ )
   {
      if( clGroupNames[i] !== clGroupNames[i - 1] )
      {
         expGroupNames.push( clGroupNames[i] )
      }
   }
   if( !commCompareObject( actGroupNames.sort(), expGroupNames ) )
   {
      throw new Error( "check groupNames error! expGroupNames=" + JSON.stringify( expGroupNames ) + "\ntask=" + JSON.stringify( taskInfo ) );
   }
}

/******************************************************************************
 * @description: 检查create / drop index任务信息（选择copy子表名、索引名、组名、结果状态码比较）
 * @param {string} taskTypeDesc  //任务类型 
 * @param {string} csName  
 * @param {string} clName
 * @param {array} idxNameObj    //检验测索引名，多个索引名或复制索引操作需要传入索引名数组
 * @param {int} resultCode       //任务错误码 成功:0 失败:错误码 
 * @return {*}
 ******************************************************************************/
function checkIndexTask ( taskTypeDesc, csName, clName, idxNameObj, resultCode )
{
   if( undefined == resultCode ) { resultCode = 0; }
   if( typeof ( idxNameObj ) == "string" ) { idxNameObj = [idxNameObj]; }

   var expGroupNames = commGetCLGroups( db, csName + "." + clName );
   var cursor = db.listTasks( { "Name": csName + '.' + clName, TaskTypeDesc: taskTypeDesc } );
   var actTasks = [];
   var actIndexNames = [];

   while( cursor.next() )
   {
      var taskInfo = cursor.current().toObj();
      actIndexNames.push( taskInfo.IndexName );
      actTasks.push( taskInfo );
   }
   cursor.close();

   //校验索引名
   actIndexNames.sort();
   idxNameObj.sort();
   if( !commCompareObject( actIndexNames, idxNameObj ) ) 
   {
      throw new Error( "check indexNames error! actIndexes=" + JSON.stringify( actIndexNames ) + "\ntask=" + JSON.stringify( actTasks ) );
   }

   for( var i = 0; i < actTasks.length; i++ )
   {
      var task = actTasks[i];

      //校验结果状态码
      if( task.ResultCode != resultCode )
      {
         throw new Error( "check resultCode error! code = " + task.ResultCode + "\ntasks=" + JSON.stringify( actTasks ) );
      }

      var isMainTask = taskInfo.hasOwnProperty( "IsMainTask" );
      if( !isMainTask )
      {
         //校验组信息   
         var actGroupNames = [];
         var groups = task.Groups;
         for( var j = 0; j < groups.length; j++ )
         {
            actGroupNames.push( groups[j].GroupName );
         }
         if( !commCompareObject( actGroupNames.sort(), expGroupNames.sort() ) )
         {
            throw new Error( "check groupNames error! groupNames = " + JSON.stringify( actGroupNames ) + ",expGroupNames="
               + JSON.stringify( expGroupNames ) + "\ntasks=" + JSON.stringify( actTasks ) );
         }
      }
   }
}

/******************************************************************************
 * @description: 检查create / drop index任务信息（通过匹配cl名、任务类型、错误码查找任务进行比较）
 * @param {string} taskTypeDesc  //任务类型 
 * @param {string} csName  
 * @param {string} clName
 * @param {array} idxNameObj    //检验测索引名，多个索引名或复制索引操作需要传入索引名数组
 * @param {int} resultCode       //任务错误码 成功:0 失败:错误码 
 * @return {*}
 ******************************************************************************/
function checkIndexTaskResult ( taskTypeDesc, csName, clName, idxNameObj, resultCode )
{
   if( undefined == resultCode ) { resultCode = 0; }
   if( typeof ( idxNameObj ) == "string" ) { idxNameObj = [idxNameObj]; }

   var expGroupNames = commGetCLGroups( db, csName + "." + clName );
   var cursor = db.listTasks( { "Name": csName + '.' + clName, "TaskTypeDesc": taskTypeDesc, "ResultCode": resultCode } );
   var actTasks = [];
   var actIndexNames = [];

   while( cursor.next() )
   {
      var taskInfo = cursor.current().toObj();
      actIndexNames.push( taskInfo.IndexName );
      actTasks.push( taskInfo );
   }
   cursor.close();

   //校验索引名
   if( !commCompareObject( actIndexNames, idxNameObj ) ) 
   {
      throw new Error( "check indexNames error! actIndexes=" + JSON.stringify( actIndexNames ) + "\ntask=" + JSON.stringify( taskInfo ) );
   }

   var status = 9;
   for( var i = 0; i < actTasks.length; i++ )
   {
      var task = actTasks[i];

      //校验任务状态
      if( task.Status != status )
      {
         throw new Error( "check status error! status = " + task.Status + "\ntasks=" + JSON.stringify( actTasks ) );
      }

      var isMainTask = taskInfo.hasOwnProperty( "IsMainTask" );
      if( !isMainTask )
      {
         //校验组信息   
         var actGroupNames = [];
         var groups = task.Groups;
         for( var j = 0; j < groups.length; j++ )
         {
            actGroupNames.push( groups[j].GroupName );
         }
         if( !commCompareObject( actGroupNames.sort(), expGroupNames.sort() ) )
         {
            throw new Error( "check groupNames error! groupNames = " + JSON.stringify( actGroupNames ) + ",expGroupNames="
               + JSON.stringify( expGroupNames ) + "\ntasks=" + JSON.stringify( actTasks ) );
         }
      }
   }
}

/******************************************************************************
 * @description: 检查create / drop index任务信息（通过匹配cl名、任务类型、索引名查找任务进行比较）
 * @param {string} taskTypeDesc  //任务类型 
 * @param {string} csName  
 * @param {string} clName
 * @param {string} indexName    //检验测索引名
 * @param {int} resultCode       //任务错误码 成功:0 失败:错误码 
 * @return {*}
 ******************************************************************************/
function checkOneIndexTaskResult ( taskTypeDesc, csName, clName, indexName, resultCode )
{
   if( undefined == resultCode ) { resultCode = 0; }

   var expGroupNames = commGetCLGroups( db, csName + "." + clName );
   var cursor = db.listTasks( { "Name": csName + '.' + clName, "TaskTypeDesc": taskTypeDesc, "IndexName": indexName, "ResultCode": resultCode } );
   var actTasks = [];
   var actIndexNames = [];

   while( cursor.next() )
   {
      var taskInfo = cursor.current().toObj();
      actIndexNames.push( taskInfo.IndexName );
      actTasks.push( taskInfo );
   }
   cursor.close();

   var status = 9;
   for( var i = 0; i < actTasks.length; i++ )
   {
      var task = actTasks[i];

      //校验任务状态
      if( task.Status != status )
      {
         throw new Error( "check status error! status = " + task.Status + "\ntasks=" + JSON.stringify( actTasks ) );
      }

      var isMainTask = taskInfo.hasOwnProperty( "IsMainTask" );
      if( !isMainTask )
      {
         //校验组信息   
         var actGroupNames = [];
         var groups = task.Groups;
         for( var j = 0; j < groups.length; j++ )
         {
            actGroupNames.push( groups[j].GroupName );
         }
         if( !commCompareObject( actGroupNames.sort(), expGroupNames.sort() ) )
         {
            throw new Error( "check groupNames error! groupNames = " + JSON.stringify( actGroupNames ) + ",expGroupNames="
               + JSON.stringify( expGroupNames ) + "\ntasks=" + JSON.stringify( actTasks ) );
         }
      }
   }
}

/******************************************************************************
 * @description: 检查未创建任务
 * @param {string} csName  
 * @param {string} clName  
 * @param {string} taskTypeDesc  //任务类型 
 * @return {*}
 ******************************************************************************/
function checkNoTask ( csName, clName, taskTypeDesc )
{
   var cursor = db.listTasks( { "Name": csName + '.' + clName, TaskTypeDesc: taskTypeDesc } );
   while( cursor.next() )
   {
      var taskInfo = cursor.current().toObj();
      throw new Error( "check task should be no exist! act task= " + JSON.stringify( taskInfo ) );
   }
   cursor.close();
}

/******************************************************************************
 * @description: 校验索引是否存在
 * @param {Sequoiadb} db  
 * @param {String} csName  
 * @param {String} clName
 * @param {String} idxName   //检验测索引名
 * @param {Boolean} isExist   //getIndex获取索引在或不存在
 * @return {*}
 ******************************************************************************/
function checkIndexExist ( db, csname, clname, idxname, isExist )
{
   var dbcl = db.getCS( csname ).getCL( clname );
   if( isExist )
   {
      dbcl.getIndex( idxname );
   }
   else
   {
      assert.tryThrow( SDB_IXM_NOTEXIST, function()
      {
         dbcl.getIndex( idxname );
      } );
   }
}

/******************************************************************************
 * @description: 检查create / drop index任务信息使用resultCode进行匹配
 * @param {string} taskTypeDesc  //任务类型 
 * @param {string} csName  
 * @param {string} clName
 * @param {array} idxNameObj    //检验测索引名，多个索引名或复制索引操作需要传入索引名数组
 * @param {int} resultCode       //任务错误码 成功:0 失败:错误码 
 * @return {*}
 ******************************************************************************/
function checkIndexTaskResult ( taskTypeDesc, csName, clName, idxNameObj, resultCode )
{
   if( typeof ( idxNameObj ) == "string" ) { idxNameObj = [idxNameObj]; }

   var expGroupNames = commGetCLGroups( db, csName + "." + clName );
   var cursor = db.listTasks( { "Name": csName + '.' + clName, TaskTypeDesc: taskTypeDesc, ResultCode: resultCode } );
   var actIndexNames = [];

   var taskInfos = [];
   while( cursor.next() )
   {
      var taskInfo = cursor.current().toObj();
      actIndexNames.push( taskInfo.IndexName );
      taskInfos.push( taskInfo );

      // 校验组信息
      var isMainTask = taskInfo.hasOwnProperty( "IsMainTask" );
      if( !isMainTask )
      {
         var actGroupNames = [];
         var groups = taskInfo.Groups;
         for( var i = 0; i < groups.length; i++ )
         {
            actGroupNames.push( groups[i].GroupName );
         }

         if( !commCompareObject( actGroupNames.sort(), expGroupNames.sort() ) )
         {
            throw new Error( "check groupNames error! groupNames = " + JSON.stringify( actGroupNames ) + ",expGroupNames="
               + JSON.stringify( expGroupNames ) + "\ntasks=" + JSON.stringify( taskInfo ) );
         }
      }
   }
   cursor.close();

   // 校验索引名
   if( !commCompareObject( actIndexNames.sort(), idxNameObj.sort() ) ) 
   {
      throw new Error( "check indexNames error! actIndexes=" + JSON.stringify( actIndexNames ) + "\ntask=" + JSON.stringify( taskInfos ) );
   }
}

/******************************************************************************
 * @description: 获取cl所在节点中任意一个节点名
 * @param {Sequoiadb} db  
 * @param {string} fullclName  //cl全名:cs名+cl名  *
 * @return {String} nodeName //返回需要创建索引的节点名
 ******************************************************************************/
function getCLOneNodeName ( db, fullclName )
{
   var nodes = commGetCLNodes( db, fullclName );
   var serialNum = Math.floor( Math.random() * nodes.length );
   var nodeName = nodes[serialNum].HostName + ":" + nodes[serialNum].svcname;
   return nodeName;
}

/******************************************************************************
 * @description: 检查访问索引所在节点上的访问计划信息（只检查查询扫描类型和索引名）
 * @param {string} csName  
 * @param {string} clName
 * @param {string} indexNodeName  //需要访问节点的节点名
 * @param {object} keyValue       //查询匹配记录值 
 * @param {string} expectType    //预期查询扫描类型：tbscan、ixscan
 * @param {string} expIndexName  //预期查询使用索引名
 * @return {*}
 ******************************************************************************/
function checkExplainUseStandAloneIndex ( csName, clName, indexNodeName, keyValue, expectType, expIndexName )
{
   try
   {
      var dataDB = new Sdb( indexNodeName );
      var dbcl = dataDB.getCS( csName ).getCL( clName );
      checkExplain( dbcl, keyValue, expectType, expIndexName );
   }
   finally
   {
      dataDB.close();
   }

}

/******************************************************************************
 * @description: 检查节点上是否存在本地索引信息
 * @param {Sequoiadb} db
 * @param {string} groupName         //cl所在组名 
 * @param {string} csName      
 * @param {string} clName
 * @param {string} indexName   //使用索引名      
 * @param {string/array} indexNodeNames //预期索引所在节点名
 * @param {boolean} isExist //预期判断是否存在索引，预期存在则为true，不存在则为false
 * @return {*}
 ******************************************************************************/
function checkStandaloneIndexOnNode ( db, csName, clName, indexName, indexNodeNames, isExist )
{
   var nodes = commGetCLNodes( db, csName + "." + clName );

   for( var i = 0; i < nodes.length; i++ )
   {
      var dataDB = new Sdb( nodes[i].HostName + ":" + nodes[i].svcname );
      var dbcl = getCL( dataDB, csName, clName );
      var nodeExist = false;
      if( typeof ( indexNodeNames ) == "string" )
      {
         nodeExist = ( nodes[i].HostName + ":" + nodes[i].svcname == indexNodeNames );
      }
      else
      {
         nodeExist = ( indexNodeNames.indexOf( nodes[i].HostName + ":" + nodes[i].svcname ) != -1 );
      }

      if( isExist )
      {
         if( nodeExist )
         {
            dbcl.getIndex( indexName );
         }
         else
         {
            try
            {
               dbcl.getIndex( indexName );
               throw "standalone index should be not exist! node=" + nodes[i].HostName + ":" + nodes[i].svcname;
            }
            catch( e )
            {
               if( e != SDB_IXM_NOTEXIST )
               {
                  throw new Error( e );
               }
            }
         }
      }
      //验证节点不存在索引，只需要比较指定索引节点上不存在索引
      else
      {
         if( nodeExist )
         {
            try
            {
               dbcl.getIndex( indexName );
               throw "standalone index should be not exist! node=" + nodes[i].HostName + ":" + nodes[i].svcname;
            }
            catch( e )
            {
               if( e != SDB_IXM_NOTEXIST )
               {
                  throw new Error( e );
               }
            }
         }
      }
      dataDB.close();
   }
}

/******************************************************************************
 * @description: 检查节点上是否存在本地索引信息
 * @param {Sequoiadb} db
 * @param {string} csName      
 * @param {string} clName
 * @param {string} indexName   //使用索引名      
 * @param {array} indexNodeNames //所有包含该索引的节点名
 * @param {boolean} isExist //预期判断是否存在索引，预期存在则为true，不存在则为false
 * @return {*}
 ******************************************************************************/
function checkStandaloneIndexOnNodes ( db, csName, clName, indexName, indexNodeNames, isExist )
{
   var nodes = commGetCLNodes( db, csName + "." + clName );

   for( var i = 0; i < nodes.length; i++ )
   {
      var dataDB = new Sdb( nodes[i].HostName + ":" + nodes[i].svcname );
      var dbcl = dataDB.getCS( csName ).getCL( clName );
      var nodeName = nodes[i].HostName + ":" + nodes[i].svcname;
      if( isExist )
      {
         if( indexNodeNames.indexOf( nodeName ) == -1 )
         {
            try
            {
               dbcl.getIndex( indexName );
               throw "standalone index should be not exist! node=" + nodes[i].HostName + ":" + nodes[i].svcname;
            }
            catch( e )
            {
               if( e != SDB_IXM_NOTEXIST )
               {
                  throw new Error( e );
               }
            }
         }
         else
         {
            dbcl.getIndex( indexName );
         }
      }
      //验证节点不存在索引，只需要比较指定索引节点上不存在索引
      else
      {
         if( indexNodeNames.indexOf( nodeName ) == -1 )
         {
            try
            {
               dbcl.getIndex( indexName );
               throw "standalone index should be not exist! node=" + nodes[i].HostName + ":" + nodes[i].svcname;
            }
            catch( e )
            {
               if( e != SDB_IXM_NOTEXIST )
               {
                  throw new Error( e );
               }
            }
            break;
         }
      }
      dataDB.close();
   }
}

/******************************************************************************
 * @description: 检查create / drop index任务信息使用resultCode进行匹配
 * @param {string} taskTypeDesc  //任务类型 
 * @param {string} fullclName    //cl全名 
 * @param {string} nodeName      //独立索引所在节点的节点名
 * @param {array} idxNameObj    //检验测索引名，多个索引名或复制索引操作需要传入索引名数组
 * @param {int} resultCode       //任务错误码 成功:0 失败:错误码 
 * @return {*}
 ******************************************************************************/
function checkStandaloneIndexTask ( taskTypeDesc, fullclName, nodeName, idxNameObj, resultCode )
{
   if( undefined == resultCode ) { resultCode = 0; }
   if( typeof ( idxNameObj ) == "string" ) { idxNameObj = [idxNameObj]; }

   var cursor = db.snapshot( SDB_SNAP_TASKS, { "NodeName": nodeName, "TaskTypeDesc": taskTypeDesc, "Name": fullclName } );
   var actTasks = [];
   //status 9=finsh
   var status = 9;
   var actIndexNames = [];

   while( cursor.next() )
   {
      var taskInfo = cursor.current().toObj();
      actIndexNames.push( taskInfo.IndexName );
      actTasks.push( taskInfo );
   }
   cursor.close();

   //校验索引名
   if( !commCompareObject( actIndexNames, idxNameObj ) ) 
   {
      throw new Error( "check indexNames error! actIndexes=" + JSON.stringify( actIndexNames ) + "\ntask=" + JSON.stringify( actTasks ) );
   }

   for( var i = 0; i < actTasks.length; i++ )
   {
      var task = actTasks[i];

      //校验结果状态码
      if( task.ResultCode != resultCode )
      {
         throw new Error( "check resultCode error! code = " + task.ResultCode + "\ntasks=" + JSON.stringify( actTasks ) );
      }

      //检查任务状态
      if( task.Status != status )
      {
         throw new Error( "check resultStatus error! status = " + task.Status + "\ntasks=" + JSON.stringify( actTasks ) );
      }

      //检查节点信息
      if( task.NodeName != nodeName )
      {
         throw new Error( "check nodeName error! nodeName = " + task.NodeName + "\ntasks=" + JSON.stringify( actTasks ) );
      }

      //检查独立索引标志
      if( task.IndexDef.Standalone != true )
      {
         throw new Error( "check standalone error! standalone = " + task.IndexDef.Standalone + "\ntasks=" + JSON.stringify( actTasks ) );
      }
   }
}

function getOneNodeName ( db, groupName )
{
   var nodes = commGetGroupNodes( db, groupName )
   var serialNum = Math.floor( Math.random() * nodes.length );
   var nodeName = nodes[serialNum].HostName + ":" + nodes[serialNum].svcname;
   return nodeName;
}

function getOneNodeName ( db, groupName )
{
   var nodes = commGetGroupNodes( db, groupName );
   var serialNum = Math.floor( Math.random() * nodes.length );
   var nodeName = nodes[serialNum].HostName + ":" + nodes[serialNum].svcname;
   return nodeName;
}

function getCL ( db, csName, clName )
{
   var doTime = 0;
   var timeOut = 30;
   while( doTime < timeOut )
   {
      try
      {
         var dbcl = db.getCS( csName ).getCL( clName );
         break;
      }
      catch( e )
      {
         if( e != SDB_DMS_NOTEXIST && e != SDB_DMS_CS_NOTEXIST )
         {
            throw new Error( e );
         }
      }
      sleep( 1000 );
      doTime++;
   }
   if( doTime >= timeOut )
   {
      throw new Error( "get collection time out" );
   }
   return dbcl;
}

/******************************************************************************
 * @description: 创建索引后，校验主备节点上索引一致性（不校验CreateTime、RebuildTime、_id索引属性）
 * @param {string} csname  
 * @param {string} clName     
 * @param {string} idxname      //索引名*
 ******************************************************************************/
function checkExistIndexConsistent ( db, csname, clname, idxname )
{
   var expIndex = null;
   var doTime = 0;
   var timeOut = 300000;
   var nodes = commGetCLNodes( db, csname + "." + clname );
   do
   {
      var sucNodes = 0;
      for( var i = 0; i < nodes.length; i++ )
      {
         var seqdb = new Sdb( nodes[i].HostName + ":" + nodes[i].svcname );
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
            delete expDef._id;
            delete actDef.CreateTime;
            delete actDef.RebuildTime;
            delete actDef._id;
            assert.equal( expDef, actDef, "---checkout nodename =" + nodes[i].HostName + ":" + nodes[i].svcname );
         }
         seqdb.close();
      }
      sleep( 200 );
      doTime += 200;
   } while( doTime < timeOut && sucNodes < nodes.length );

   if( doTime >= timeOut )
   {
      throw new Error( "check timeout index not synchronized !" );
   }
}