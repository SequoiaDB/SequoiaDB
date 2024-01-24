/******************************************************************************
 * @Description   : seqDB-23840:Sdb.list 带 cond/sel/sort 匹配回收站项目
 * @Author        : Yang Qincheng
 * @CreateTime    : 2021.04.25
 * @LastEditTime  : 2022.06.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.csName = CHANGEDPREFIX + "_cs_23840";
testConf.clName = COMMCLNAME + "_23840";

main( test );
function test ()
{
   assert.equal( db.getRecycleBin().getDetail().toObj().Enable, true );

   db.getRecycleBin().dropAll();
   var clFullName = testConf.csName + "." + testConf.clName;
   var cs = testPara.testCS;
   var cl = testPara.testCL;

   cleanRecycleBin( db, testConf.csName );
   cl.insert( { "a": 1 } );

   // 构造回收站项目
   cl.truncate();
   var recycleName1 = getOneRecycleName( db, clFullName, "Truncate" );
   cs.dropCL( testConf.clName );
   var recycleName2 = getOneRecycleName( db, clFullName, "Drop" );

   // list，带cond/sel/sort
   var cursor = db.list( SDB_LIST_RECYCLEBIN, { "OriginName": clFullName }, { "RecycleName": "" }, { "RecycleID": -1 } );
   commCompareResults( cursor, [{ "RecycleName": recycleName2 }, { "RecycleName": recycleName1 }] );
}