/******************************************************************************
 * @Description   : seqDB-24378:指定其它cl所在节点创建本地索引    
 * @Author        : wu yan
 * @CreateTime    : 2021.09.23
 * @LastEditTime  : 2022.01.29
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clName = COMMCLNAME + "_standaloneIndex_24378";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ()
{
   var indexName = "Index_24378";
   var dbcl = testPara.testCL;
   var mainCLName = COMMCLNAME + "_maincl_index24378";
   var groupName2 = testPara.dstGroupNames[0];
   var nodeName = getOneNodeName( db, groupName2 );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Fail to drop CL in the beginning" );
   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );

   //普通表指定其它cl所在节点创建索引
   assert.tryThrow( SDB_CL_NOT_EXIST_ON_NODE, function()
   {
      dbcl.createIndex( indexName, { a: 1, b: 1 }, { Standalone: true }, { NodeName: nodeName } );
   } );
   checkIndexTask( nodeName, indexName );

   //主表指定其它cl所在节点创建索引
   maincl.attachCL( COMMCSNAME + "." + testConf.clName, { LowBound: { a: 0 }, UpBound: { a: 20000 } } );
   assert.tryThrow( SDB_CL_NOT_EXIST_ON_NODE, function()
   {
      maincl.createIndex( indexName, { a: 1, b: 1 }, { Standalone: true }, { NodeName: nodeName } );
   } );
   checkIndexTask( nodeName, indexName );
}

function checkIndexTask ( nodeName, indexName )
{
   //检查索引信息  
   var cursor = db.snapshot( SDB_SNAP_TASKS, { "NodeName": nodeName, "IndexName": indexName } );
   while( cursor.next() )
   {
      var taskInfo = cursor.current().toObj();
      throw new Error( "the index task should be no exist! taskInfo=" + JSON.stringify( taskInfo ) );
   }
   cursor.close();
}


