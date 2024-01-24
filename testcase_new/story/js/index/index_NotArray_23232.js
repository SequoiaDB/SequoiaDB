/******************************************************************************
 * @Description   : seqDB-23232:创建复合索引且NotArray:false，对索引字段做数据操作
 * @Author        : Xiaoni Huang
 * @CreateTime    : 2021.01.09
 * @LastEditTime  : 2021.01.09
 * @LastEditors   : Xiaoni Huang
 ******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_cl_23232";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   // 创建索引
   var notArray = false;
   var idxName = "idx";
   cl.createIndex( idxName, { "a": 1, "b": 1 }, { "NotArray": notArray } )
   // 检查索引NotArray属性正确性   
   var idxInfo = JSON.parse( cl.getIndex( idxName ).toString() );
   assert.equal( idxInfo.IndexDef.NotArray, notArray, "idxInfo = " + idxInfo );

   // 增删改查数据，部分索引字段插入非数组数据，1个索引字段插入为数组数据
   var record = { "num": 1, "a": 1, b: [1] };
   cl.insert( record );

   // 索引扫描
   var cond = { "num": 1, "a": { "$gte": 1 }, "b": [1] };
   var hint = { "": idxName };
   var explainInfo = cl.find( cond ).hint( hint ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).ScanType, "ixscan", "explainInfo = " + explainInfo );
   var cursor = cl.find( { "a": { "$gte": 1 }, "b": [1] } ).hint( hint );
   commCompareResults( cursor, [record] );

   // 表扫描
   var explainInfo = cl.find( cond ).explain().toArray();
   assert.equal( JSON.parse( explainInfo[0] ).ScanType, "tbscan", "explainInfo = " + explainInfo );
   var cursor = cl.find( { "a": { "$gte": 1 }, "b": [1] } ).hint( hint );
   commCompareResults( cursor, [record] );


   var record = { "num": 2, "a": [1], b: 1 };
   cl.update( { "$set": record }, cond );
   var cursor = cl.find( record );
   commCompareResults( cursor, [record] );

   cl.remove( record );
   assert.equal( cl.count(), 0 );
}