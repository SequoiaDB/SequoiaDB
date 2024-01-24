/******************************************************************************
 * @Description   : seqDB-23247 :: 索引不支持数组，查询条件/选择条件为{} 
 * @Author        : Yu Fan
 * @CreateTime    : 2021.01.09
 * @LastEditTime  : 2021.10.14
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_23247";
var indexName = "Index_23247";

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

   // 查询条件为{}
   var cursor = cl.find( {}, { a: "" } ).sort( { a: 1 } ).hint( { "": indexName } );
   commCompareResults( cursor, records );
   var explainInfo = cl.find( {}, { a: "" } ).sort( { a: 1 } ).hint( { "": indexName } ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, true, "explainInfo = " + explainInfo );

   // 选择条件为{}
   var cursor = cl.find( { a: 1 }, {} );
   commCompareResults( cursor, [{ a: 1 }] );
   var explainInfo = cl.find( { a: 1 }, {} ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, false, "explainInfo = " + explainInfo );
}

