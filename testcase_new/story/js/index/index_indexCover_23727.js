/******************************************************************************
 * @Description   : seqDB-23727 : 如果索引中的字段名存在包含关系,查询数据
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.23
 * @LastEditTime  : 2021.03.29
 * @LastEditors   : Yi Pan
 ******************************************************************************/
testConf.clName = CHANGEDPREFIX + "cl_23727";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;

   //插入数据
   cl.insert( { "a": "x", "ab": "x" } );
   cl.insert( { "a": "f", "ab": "f" } );

   //创建索引
   cl.createIndex( "idx", { "a": 1, "ab": 1, "ac": 1 }, { "NotArray": true } );

   //查询结果
   var act = cl.find( { "ac": { "$isnull": 1 } }, { "a": null, "ab": null, "ac": null } ).sort( { "a": 1, "ab": 1, "ac": 1 } ).hint( { "": "idx" } );

   //比较结果
   var exp = [{ a: 'f', ab: 'f', ac: null }, { a: 'x', ab: 'x', ac: null }];
   commCompareResults( act, exp );
}