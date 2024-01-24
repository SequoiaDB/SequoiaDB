/******************************************************************************
 * @Description   : seqDB-23604:cl.alter，配置RepairCheck:false，做数据操作
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2021.03.05
 * @LastEditTime  : 2021.03.08
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = CHANGEDPREFIX + "_cl_23604";

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
   cl.alter( { "RepairCheck": false } );

   // 被禁用的操作（insert/update/upsert/remove）
   cl.insert( { "a": 3 } );
   cl.update( { "$set": { "b": 3 } }, { "a": 1 } );
   cl.upsert( { "$set": { "c": 4 } }, { "a": 4 } );
   cl.remove( { "a": 2 } );
   commCompareResults( cl.find().sort( { "a": 1 } ), [{ "a": 1, "b": 3 }, { "a": 3 }, { "a": 4, "c": 4 }] );

   cl.insert( { "a": 5 } );
   cl.find( {} ).update( { "$set": { "b": 5 } } ).toArray();
   cl.find( { "a": 3 } ).remove().toArray();
   commCompareResults( cl.find().sort( { "a": 1 } ), [{ "a": 1, "b": 5 }, { "a": 4, "b": 5, "c": 4 }, { "a": 5, "b": 5 }] );

   // 不被禁用的操作
   // 结构化数据操作
   commCompareResults( cl.findOne(), [{ "a": 1, "b": 5 }] );

   commCompareResults( cl.aggregate( { "$match": { "a": 1 } } ), [{ "a": 1, "b": 5 }] );
   assert.equal( cl.count(), 3 );

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