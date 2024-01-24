/******************************************************************************
 * @Description   : seqDB-23828:回收时配置 SkipRecycleBin:true 强制删除
 * @Author        : Yang Qincheng
 * @CreateTime    : 2021.04.23
 * @LastEditTime  : 2022.06.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.csName = CHANGEDPREFIX + "_cs_23828";
testConf.clName = COMMCLNAME + "_23828";

main( test );

function test ()
{
   assert.equal( db.getRecycleBin().getDetail().toObj().Enable, true );

   var clFullName = testConf.csName + "." + testConf.clName;
   var cs = testPara.testCS;
   var cl = testPara.testCL;

   cleanRecycleBin( db, testConf.csName );
   cl.insert( { "a": 1 } );

   // 指定强制不回收 truncate 项目
   cl.truncate( { "SkipRecycleBin": true } );
   assert.equal( getRecycleName( db, clFullName, "Truncate" ), [] );

   // 指定强制不回收 dropCL 项目
   cs.dropCL( testConf.clName, { "SkipRecycleBin": true } );
   assert.equal( getRecycleName( db, clFullName, "Drop" ), [] );

   // 指定强制不回收 dropCS 项目
   db.dropCS( testConf.csName, { "SkipRecycleBin": true } );
   assert.equal( getRecycleName( db, testConf.csName, "Drop" ), [] );
}
