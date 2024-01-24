/******************************************************************************
 * @Description   : seqDB-23244:索引支持数组，测试覆盖索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.01.19
 * @LastEditTime  : 2021.01.22
 * @LastEditors   : Yi Pan
 ******************************************************************************/

testConf.clName = COMMCLNAME + "_23244";
var idxname = "idx23244";

main( test );

function test ()
{
   var cl = testPara.testCL;

   //修改配置文件
   db.updateConf( { indexcoveron: true }, { Global: true } );

   //创建索引 NotArray指定为false
   cl.createIndex( idxname, { 'age': 1 }, { NotArray: false } );

   //插入数据
   cl.insert( { 'age': 1 } );
   cl.insert( { 'age': 2 } );

   //查询数据
   var actResult = cl.find( { 'age': 1 }, { 'age': '' } ).hint( { "": idxname } );
   expResult = [{ 'age': 1 }];
   commCompareResults( actResult, expResult );
   //查询分析
   var explain = cl.find( { 'age': 1 }, { 'age': '' } ).hint( { "": idxname } ).explain();
   checkIndexCover( explain, false );

   //条件查询
   var actResult = cl.find( { 'age': { $et: 2 } }, { 'age': "" } ).hint( { "": idxname } );
   expResult = [{ 'age': 2 }];
   commCompareResults( actResult, expResult );
   //条件分析
   var explain = cl.find( { 'age': { $et: 2 } }, { 'age': "" } ).hint( { "": idxname } ).explain();
   checkIndexCover( explain, false );

}