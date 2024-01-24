/******************************************************************************
 * @Description   : seqDB-23246 :: 索引不支持数组，查询不满足覆盖索引必要条件 
 * @Author        : Yu Fan
 * @CreateTime    : 2021.01.09
 * @LastEditTime  : 2021.10.14
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_23246";
var indexName = "Index_23246";

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
      records.push( { a: i, b: i, c: i } );
   }
   cl.insert( records )

   // 查询数据
   // 索引扫描
   var cursor = cl.find().hint( { "": indexName } );
   commCompareResults( cursor, records );
   var explainInfo = cl.find().hint( { "": indexName } ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, false, "explainInfo = " + explainInfo );

   // 索引字段不完全包含查询条件字段；
   cursor = cl.find( { a: { $gte: 0 }, b: { $lt: 11 } }, { a: "" } ).sort( { a: 1 } ).hint( { "": indexName } );
   commCompareResults( cursor, [{ "a": 0 }, { "a": 1 }, { "a": 2 }, { "a": 3 }, { "a": 4 }, { "a": 5 }, { "a": 6 }, { "a": 7 }, { "a": 8 }, { "a": 9 }] );
   explainInfo = cl.find( { a: { $gte: 0 }, b: { $lt: 11 } }, { a: "" } ).hint( { "": indexName } ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, false, "explainInfo = " + explainInfo );

   // 索引字段不完全包含选择条件；
   cursor = cl.find( { a: { $gte: 9 } }, { a: "", b: "" } ).hint( { "": indexName } );
   commCompareResults( cursor, [{ a: 9, b: 9 }] );
   explainInfo = cl.find( { a: { $gte: 9 } }, { a: "", b: "" } ).hint( { "": indexName } ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, false, "explainInfo = " + explainInfo );

   // 索引字段不完全包含排序字段；
   cursor = cl.find( { a: { $lt: 2 } }, { a: "" } ).sort( { b: 1 } ).hint( { "": indexName } );
   commCompareResults( cursor, [{ a: 0 }, { a: 1 }] );
   explainInfo = cl.find( { a: { $lt: 2 } }, { a: "" } ).sort( { b: 1 } ).hint( { "": indexName } ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, false, "explainInfo = " + explainInfo );

   // 选择条件为object格式（如：{a:{$include:1}}）；
   cursor = cl.find( { a: 2 }, { a: { $include: 1 } } ).hint( { "": indexName } );
   commCompareResults( cursor, [{ a: 2 }] );
   explainInfo = cl.find( { a: 2 }, { a: { $include: 1 } } ).hint( { "": indexName } ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, true, "explainInfo = " + explainInfo );

   // find条件为空（如：cl.find()）;
   cursor = cl.find().hint( { "": indexName } );
   commCompareResults( cursor, records );
   explainInfo = cl.find().hint( { "": indexName } ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, false, "explainInfo = " + explainInfo );

   // 选择条件为空
   cursor = cl.find( { a: 1 } ).hint( { "": indexName } );
   commCompareResults( cursor, [{ a: 1, b: 1, c: 1 }] );
   explainInfo = cl.find( { a: 1 } ).hint( { "": indexName } ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).IndexCover, false, "explainInfo = " + explainInfo );
}

