/******************************************************************************
 * @Description   : seqDB-23845:clearSync清理回收站某个项目
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2021.06.24
 * @LastEditTime  : 2022.08.17
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = CHANGEDPREFIX + "_cs_23845";
testConf.clName = COMMCLNAME + "_23845";

main( test );
function test ( testPara )
{
   var csName = testConf.csName;
   var clName = testConf.clName;
   var cl = testPara.testCL;
   var fullCLName = csName + "." + clName;

   cleanRecycleBin( db, csName );
   cl.insert( { "a": 1 } );
   cl.truncate();
   var recycleName = getOneRecycleName( db, fullCLName, "Truncate" );
   assert.notEqual( recycleName, undefined );

   // 清理存在的回收站项目
   db.getRecycleBin().dropItem( recycleName, { "Async": true } );
   waitItemTaskFinished( db, recycleName );
   assert.equal( getRecycleName( db, fullCLName, "Truncate" ), [] );

   // 重复清理相同回收站项目（清理不存在的回收站项目）
   assert.tryThrow( SDB_RECYCLE_ITEM_NOTEXIST, function()
   {
      db.getRecycleBin().dropItem( recycleName );
   } );
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