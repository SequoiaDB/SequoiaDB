/******************************************************************************
 * @Description   : seqDB-23842:SdbRecycle.snapshot 带 cond/sel/sort 匹配所有项目
 * @Author        : Yang Qincheng
 * @CreateTime    : 2021.04.25
 * @LastEditTime  : 2022.06.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.csName = CHANGEDPREFIX + "_cs_23842";
testConf.clName = COMMCLNAME + "_23842";

main( test );
function test ()
{
   var recycleBin = db.getRecycleBin();
   assert.equal( recycleBin.getDetail().toObj().Enable, true );

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

   // list，cond匹配不存在的项目
   var cursor = recycleBin.snapshot( { "RecycleName": "notExist" } );
   assert.equal( cursor.size(), 0 );
   // list，cond匹配存在的项目
   var cursor = recycleBin.snapshot( { "RecycleName": recycleName1 } );
   assert.equal( cursor.size(), 1 );

   // list, sel匹配不存在的字段
   var cursor = recycleBin.snapshot( { "RecycleName": recycleName1 }, { "notExist": "" } );
   assert.equal( cursor.next().toObj().notExist, "" );
   // list，sel匹配存在的项目
   var cursor = recycleBin.snapshot( { "RecycleName": recycleName1 }, { "RecycleName": "" } );
   assert.equal( cursor.next().toObj().RecycleName, recycleName1 );

   // list, sort正序
   var cursor = recycleBin.snapshot( { "OriginName": clFullName }, { "RecycleName": "" }, { "RecycleID": 1 } );
   commCompareResults( cursor, [{ "RecycleName": recycleName1 }, { "RecycleName": recycleName2 }] );
   // list, sort逆序
   var cursor = recycleBin.snapshot( { "OriginName": clFullName }, { "RecycleName": "" }, { "RecycleID": -1 } );
   commCompareResults( cursor, [{ "RecycleName": recycleName2 }, { "RecycleName": recycleName1 }] );
}