/******************************************************************************
 * @Description   : seqDB-23847:Async异步清理回收站项目
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2021.06.24
 * @LastEditTime  : 2022.08.17
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = CHANGEDPREFIX + "_23847";
testConf.clName = COMMCLNAME + "_23847";

main( test );
function test ( testPara )
{
   var csName = testConf.csName;
   var clName = testConf.clName;
   var cl = testPara.testCL;
   var fullCLName = csName + "." + clName;
   var itemNum = 30;

   try
   {
      db.getRecycleBin().alter( { "MaxVersionNum": -1 } );
      cleanRecycleBin( db, csName );

      for( var i = 0; i < itemNum; i++ )
      {
         cl.insert( { "a": i } );
         cl.truncate();
      }
      var recycleNames = getRecycleName( db, fullCLName, "Truncate" );
      assert.equal( recycleNames.length, itemNum );

      // 清理存在的回收站项目
      db.getRecycleBin().dropAll( { "Async": true } );
      for( var i in recycleNames )
      {
         waitItemTaskFinished( db, recycleNames[i] );
      }
      assert.equal( getRecycleName( db, fullCLName, "Truncate" ), [] );
   } finally
   {
      db.getRecycleBin().alter( { "MaxVersionNum": 2 } );
   }
}

function waitItemTaskFinished ( sdb, recycleName )
{
   var maxretryTimes = 600;
   var curRetryTimes = 0;
   while( curRetryTimes <= maxretryTimes )
   {
      var recycleItem = 0;
      var cursor = sdb.getRecycleBin().snapshot( { "RecycleName": recycleName } );
      while( cursor.next() )
      {
         recycleItem++;
      }
      cursor.close();
      if( recycleItem == 0 )
      {
         break;
      }
      else
      {
         sleep( 500 );
         curRetryTimes++;
      }
   }
   if( curRetryTimes > maxretryTimes )
   {
      throw new Error( "wait task finished timeout." );
   }
}