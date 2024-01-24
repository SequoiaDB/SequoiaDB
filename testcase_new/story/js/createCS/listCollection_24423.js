/******************************************************************************
 * @Description   : seqDB-24423:创建CL并使用CS.listCollections()
 * @Author        : liuli
 * @CreateTime    : 2021.10.14
 * @LastEditTime  : 2021.10.14
 * @LastEditors   : liuli
 ******************************************************************************/
// 使用数据源的集合在用例 datasource/datasrc_23378.js 测试
testConf.csName = COMMCSNAME + "_24423";

main( test );
function test ( args )
{
   var clName = "cl_24423_";
   var dbcs = args.testCS;

   var cursor = dbcs.listCollections();
   while( cursor.next() )
   {
      throw new Error( JSON.stringify( "expect not exist cl " + cursor.current().toObj() ) );
   }

   var clNames = [];
   for( var i = 0; i < 49; i++ )
   {
      dbcs.createCL( clName + i );
      clNames.push( testConf.csName + "." + clName + i );
   }

   dbcs.createCL( clName + 49, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   dbcs.createCL( clName + 50, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } );
   clNames.push( testConf.csName + "." + clName + 50 );
   clNames.push( testConf.csName + "." + clName + 49 );

   var actNames = [];
   var cursor = dbcs.listCollections();
   while( cursor.next() )
   {
      actNames.push( cursor.current().toObj()["Name"] );
   }
   cursor.close();
   assert.equal( actNames.sort(), clNames.sort() );
}