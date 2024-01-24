/******************************************************************************
 * @Description   : seqDB-23829:回收时配置SkipRecycleBin:false回收项目
 * @Author        : Yang Qincheng
 * @CreateTime    : 2021.04.23
 * @LastEditTime  : 2022.06.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.csName = CHANGEDPREFIX + "_cs_23829";
testConf.clName = COMMCLNAME + "_23829";

main( test );

function test ()
{
   assert.equal( db.getRecycleBin().getDetail().toObj().Enable, true );

   var clFullName = testConf.csName + "." + testConf.clName;
   var cs = testPara.testCS;
   var cl = testPara.testCL;

   cleanRecycleBin( db, testConf.csName );
   cl.insert( { "a": 1 } );

   // 回收 truncate 项目
   cl.truncate( { "SkipRecycleBin": false } );
   var recycleName = getRecycleName( db, clFullName, "Truncate" );
   assert.notEqual( recycleName, undefined );

   // 回收 dropCL 项目
   cs.dropCL( testConf.clName, { "SkipRecycleBin": false } );
   var recycleName = getRecycleName( db, clFullName, "Drop" );
   assert.notEqual( recycleName, undefined );

   // 回收 dropCS 项目
   db.dropCS( testConf.csName, { "SkipRecycleBin": false } );
   var recycleName = getRecycleName( db, clFullName, "Drop" );
   assert.notEqual( testConf.csName, undefined );
}
