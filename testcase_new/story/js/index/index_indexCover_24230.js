/******************************************************************************
 * @Description   : seqDB-24230:覆盖索引，显示的DataRead计数不对
 * @Author        : YiPan
 * @CreateTime    : 2021.05.20
 * @LastEditTime  : 2021.05.20
 * @LastEditors   : Yi Pan
 ******************************************************************************/
testConf.clName = COMMCSNAME + "_cl_24230";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   // 创建索引   
   var idxName = "idx24230";
   cl.createIndex( idxName, { "a": 1 }, { "NotArray": true } )

   // 插入数据）
   cl.insert( { a: 1 } );

   //指定匹配条件为{a:1}，选择条件为{a:1}
   var result = cl.find( { a: 1 }, { a: 1 } ).explain( { Run: true } ).current().toObj();
   assert.equal( result.DataRead, 0 );
   assert.equal( result.IndexRead, 1 );

   //指定匹配条件为{a:1}，选择条件为{a:1,b:1}
   result = cl.find( { a: 1 }, { a: 1, b: 0 } ).explain( { Run: true } ).current().toObj();
   assert.equal( result.DataRead, 1 );
   assert.equal( result.IndexRead, 1 );

   //指定匹配条件为空，选择条件为{a:1}
   result = cl.find( {}, { a: 1 } ).explain( { Run: true } ).current().toObj();
   assert.equal( result.DataRead, 1 );
   assert.equal( result.IndexRead, 0 );
}