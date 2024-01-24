/************************************
 *@Description: 测试用例 seqDB-12177 :: 版本: 1 :: 切分表开启flag执行upsert更新分区键
 1、向cl中插入数据
 2、执行切分操作
 3、执行upsert更新分区键，设置flag为true，其中使用匹配符$et匹配不到数据
 4、检查更新结果
 1、更新返回失败，查看匹配不到记录时未追加更新字段
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var clName = CHANGEDPREFIX + "_cl_12177";
main( test );

function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   var allGroupName = getGroupName( db, true );
   if( 1 === allGroupName.length )
   {
      return;
   }
   commDropCL( db, COMMCSNAME, clName );
   var mycl = createCL( COMMCSNAME, clName, { a: 1 } );
   //insert data
   var docs = [{ a: 1 }, { a: 1024 }];
   mycl.insert( docs );

   clSplit( COMMCSNAME, clName, 50 );
   //updateData
   updateDataError( mycl, "upsert", { $set: { a: "test" } }, { "a": { "$et": 1 } } )

   //check the update result
   var expRecs = docs;
   checkResult( mycl, null, {}, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
