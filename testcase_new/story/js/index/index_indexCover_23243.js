/******************************************************************************
 * @Description   : seqDB-23243:关闭覆盖索引功能，测试覆盖索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.01.19
 * @LastEditTime  : 2021.01.19
 * @LastEditors   : Yi Pan
 ******************************************************************************/

testConf.clName = COMMCLNAME + "_23243";
var idxname = "idx23243";

main( test );

function test ()
{
   var cl = testPara.testCL;

   try
   {
      //修改配置文件关闭索引覆盖功能
      db.updateConf( { indexcoveron: false }, { Global: true } )

      //创建索引 NotArray指定为true
      cl.createIndex( idxname, { 'age': 1 }, { NotArray: true } );

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

   } finally
   {
      //清理环境 恢复配置
      db.updateConf( { indexcoveron: true }, { Global: true } );
   }

}