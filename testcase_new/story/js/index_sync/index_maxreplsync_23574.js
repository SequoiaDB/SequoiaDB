/******************************************************************************
 * @Description   : seqDB-23574:存在索引，修改maxreplsync=0
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2021.02.22
 * @LastEditTime  : 2021.02.23
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = CHANGEDPREFIX + "_cl_23574";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;

   // 创建索引
   cl.createIndex( "idx", { a: 1 }, true );

   // 插入/删除数据，并修改maxreplsysnc=0
   cl.insert( [{ a: 1 }, { a: 1000 }, { a: 3000 }] );

   var recsArray = [
      { a: 1, b: '12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890' },
      { a: 1000, b: '123456789012345678901234567890123456789012345678901234567890123456789012 34567890123456789012345678901234567890123456789012345678901234567890' },
      { a: 3000, b: '123456789012345678901234567890123456789012345678901234567890123456789012 34567890123456789012345678901234567890123456789012345678901234567890' }
   ];
   for( i = 0; i < 2500; i++ ) 
   {
      cl.remove( { a: 1 } )
      cl.insert( recsArray[0] )

      cl.remove( { a: 1000 } )
      cl.insert( recsArray[1] );

      cl.remove( { a: 3000 } )
      cl.insert( recsArray[2] );

      if( i % 100 == 0 )
      {
         if( i % 200 == 0 )
         {
            db.updateConf( { maxreplsync: 10 } )
         }
         else 
         {
            db.updateConf( { maxreplsync: 0 } )
         }
      }
   }

   // 检查结果
   var cursor = cl.find();
   commCompareResults( cursor, recsArray );
}