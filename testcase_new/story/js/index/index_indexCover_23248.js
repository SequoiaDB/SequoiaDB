/******************************************************************************
 * @Description   : seqDB-23248 :: 索引不支持数组，复合索引，查询条/选择/排序字段均为单索引键字段 
 * @Author        : Yu Fan
 * @CreateTime    : 2021.01.09
 * @LastEditTime  : 2021.01.11
 * @LastEditors   : Yu Fan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_23248";
var indexName = "Index_23248";

main( test );
function test ( testPara )
{
   db.updateConf( { indexcoveron: true } );
   var cl = testPara.testCL;
   // 创建索引
   cl.createIndex( indexName, { a: 1, b: 1, c: 1 }, { NotArray: true } )

   // 插入数据
   var records = new Array();
   for( var i = 0; i < 10; i++ )
   {
      records.push( { a: i, b: i, c: i, d: i } );
   }
   cl.insert( records )

   // 查询条件、选择条件、排序字段均为同一单索引字段（首/中间/末尾索引字段），选择条件非object格式
   var cursor = cl.find( { a: { $gt: 7 } }, { a: "" } ).sort( { a: 1 } ).hint( { "": indexName } );
   commCompareResults( cursor, [{ a: 8 }, { a: 9 }] );
   var explainInfo = cl.find( { a: { $gt: 7 } }, { a: "" } ).sort( { a: 1 } ).hint( { "": indexName } ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, true, "explainInfo = " + explainInfo );

   // 查询条件、选择条件为不同的单索引字段，且选择条件索引字段包含/等于排序索引字段，选择条件非object格式； 其他条件均满足覆盖索引条件；
   var cursor = cl.find( { c: { $gt: 7 } }, { a: "", b: "" } ).sort( { a: 1 } ).hint( { "": indexName } );
   commCompareResults( cursor, [{ a: 8, b: 8 }, { a: 9, b: 9 }] );
   var explainInfo = cl.find( { c: { $gt: 7 } }, { a: "", b: "" } ).sort( { a: 1 } ).hint( { "": indexName } ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, true, "explainInfo = " + explainInfo );
}

