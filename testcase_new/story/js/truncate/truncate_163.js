/******************************************************************************
*@Description: 测试普通表写入大于多个数据页的普通记录和大对象记录，然后再truncate
*@Modify list:
*              2015-5-8  xiaojun Hu   Init
*              2019-05-27 wuyan        modify
******************************************************************************/

main( test );


function test ()
{
   var csName = CHANGEDPREFIX + "_largeThanPageCS163";
   var clName = CHANGEDPREFIX + "_cl163";
   commDropCS( db, csName, true, "drop cs begin" );

   var pageSize = 4096;
   var cl = createCLAndInsertData( csName, clName, pageSize )
   truncateAndCheckResult( cl, csName, clName )
   commDropCS( db, csName, true, "drop cs end" );
}

function createCLAndInsertData ( csName, clName, pageSize )
{
   var clOption = {
      "ShardingKey": { "ID_Default": 1 }, "ShardingType": "hash"
   };

   var cl = commCreateCL( db, csName, clName, clOption, true, false, "create collection begin" );

   var lobNum = 4;
   var lobSize = pageSize * 4;
   var recordNum = 3;
   var recordSize = pageSize * 2;
   truncatePutLob( cl, lobSize, lobNum );
   truncateInsertRecord( cl, recordNum, recordSize );

   return cl;
}

function truncateAndCheckResult ( cl, csName, clName )
{
   cl.truncate();
   //after truncate ,the TotalLobPages is 0
   truncateVerify( db, csName + "." + clName );

   var expCount = 0;
   var actLobCount = cl.listLobs();
   var actRecordCount = cl.count();
   if( Number( expCount ) !== Number( actLobCount ) || Number( expCount ) !== Number( actRecordCount ) )
   {
      throw new Error( "expCount: " + expCount + "\nactLobCount: " + actLobCount + "\nexpCount: " + expCount + "\nactRecordCount: " + actRecordCount );
   }
}
