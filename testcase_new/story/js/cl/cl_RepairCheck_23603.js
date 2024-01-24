/******************************************************************************
 * @Description   : seqDB-23603:cl.alter，配置RepairCheck:true，做数据操作
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2021.03.05
 * @LastEditTime  : 2021.03.08
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = CHANGEDPREFIX + "_cl_23603";

main( test );
function test ( testPara )
{
   var lobPath1 = WORKDIR + "/lob_23603";
   var lobPath2 = lobPath1 + "_2";
   var cl = testPara.testCL;
   var cmd = new Cmd();

   // 清理环境
   cmd.run( "rm -rf " + lobPath1 );
   cmd.run( "rm -rf " + lobPath2 );

   // 准备数据
   cl.insert( { "a": 1 }, { "a": 2 } );
   cl.alter( { "RepairCheck": true } );

   // 被禁用的操作（insert/update/upsert/remove）
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function() { cl.insert( { "a": 3 } ) } );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function() { cl.update( { "$set": { "b": 3 } }, { "a": 1 } ) } );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function() { cl.upsert( { "$set": { "c": 4 } }, { "a": 4 } ) } );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function() { cl.remove( { "a": 2 } ) } );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function() { cl.find( {} ).update( { "$set": { "b": 5 } } ).toArray() } );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function() { cl.find( {} ).remove().toArray() } );

   // 不被禁用的操作
   // 结构化数据操作
   commCompareResults( cl.find(), [{ "a": 1 }] );
   commCompareResults( cl.findOne(), [{ "a": 1 }] );

   commCompareResults( cl.aggregate( { "$match": { "a": 1 } } ), [{ "a": 1 }] );
   assert.equal( cl.count(), 1 );

   cl.truncate();
   assert.equal( cl.count(), 0 );

   // 非结构化数据操作
   var file = new File( lobPath1 );
   file.close();

   var oid = cl.putLob( lobPath1 );
   var rc = cl.getLob( oid, lobPath2 );
   assert.notEqual( JSON.stringify( rc.toObj() ).indexOf( "LobSize" ), -1 );

   cl.truncateLob( oid, 0 );

   // 清理环境
   cmd.run( "rm -rf " + lobPath1 );
   cmd.run( "rm -rf " + lobPath2 );
}