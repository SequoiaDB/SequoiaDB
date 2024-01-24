/******************************************************************************
 * @Description   : seqDB-23230 :: 字段存在数组数据，创建单键索引指定NotArray:true/false 
 * @Author        : Yu Fan
 * @CreateTime    : 2021.01.08
 * @LastEditTime  : 2021.01.11
 * @LastEditors   : Yu Fan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_23230";
var indexName = "Index_23230";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   // 插入记录
   cl.insert( { a: [1, 2, 3] } );

   // 创建索引, { NotArray: true }
   assert.tryThrow( SDB_IXM_KEY_NOT_SUPPORT_ARRAY, function()
   {
      cl.createIndex( indexName, { a: 1 }, { NotArray: true } )
   } );

   // 创建索引, { NotArray: false }
   cl.createIndex( indexName, { a: 1 }, { NotArray: false } )

   // 检查索引
   var indexInfo = JSON.parse( cl.getIndex( indexName ).toString() );
   assert.equal( indexInfo.IndexDef.NotArray, false, "indexInfo = " + indexInfo );

   // 检查数据
   var cursor = cl.find( { a: [1, 2, 3] } )
   commCompareResults( cursor, [{ a: [1, 2, 3] }] );
}

