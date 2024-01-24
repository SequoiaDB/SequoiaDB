/******************************************************************************
 * @Description   : seqDB-23245 :: 索引不支持数组，覆盖索引基本功能验证 
 * @Author        : Yu Fan
 * @CreateTime    : 2021.01.09
 * @LastEditTime  : 2021.10.14
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_23245";
var indexName = "Index_23245";

main( test );
function test ( testPara )
{
   db.updateConf( { indexcoveron: true } );
   var cl = testPara.testCL;
   // 创建索引
   cl.createIndex( indexName, { a: 1 }, { NotArray: true } )

   // 插入数据
   var records = new Array();
   for( var i = 0; i < 10; i++ )
   {
      records.push( { a: i } );
   }
   cl.insert( records )

   // 查询数据
   var cursor = cl.find( { a: { $gt: 0 } }, { a: "" } ).hint( { "": indexName } );
   commCompareResults( cursor, records.slice( 1 ) );

   // 执行explain检查结果
   var explainInfo = cl.find( { a: { $gt: 0 } }, { a: "" } ).hint( { "": indexName } ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, true, "explainInfo = " + explainInfo );
}

