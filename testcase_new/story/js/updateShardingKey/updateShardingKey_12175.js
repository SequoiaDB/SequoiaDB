/************************************
 *@Description: 测试用例 seqDB-12175 :: 版本: 1 :: 分区表上upsert更新分区键
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var clName = CHANGEDPREFIX + "_cl_12175";
//main( test );

function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   var mycl = createCL( COMMCSNAME, clName, { a: 1 } );

   insertList = [{ a: 1 }, { a: { b: 1 } }, { a: { b: { c: 1 } } }, { a: [1, 2, 3, 4, 5] }];
   upsertList = [{ a: 1 }, { a: { b: 1 } }, { a: { b: { c: 1 } } }, { a: [1, 2, 3, 4, 5] }];

   for( i in insertList )
   {
      mycl.remove()
      mycl.insert( insertList[i] );
      for( j in upsertList )
      {
         mycl.upsert( { $set: upsertList[j] }, {}, {}, {}, { KeepShardingKey: true } );
         checkResult( mycl, null, null, [upsertList[j]] )
      }
   }
   commDropCL( db, COMMCSNAME, clName )
}
