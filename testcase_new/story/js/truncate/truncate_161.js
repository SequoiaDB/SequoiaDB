/******************************************************************************
*@Description: 测试切分表写入大于多个数据页的大对象记录，然后再truncate
*@Modify list:
*              2015-5-13  xiaojun Hu   Init
*              2019-05-27 wuyan        modify
******************************************************************************/

main( test );


function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   //less two groups no split 
   var groups = commGetGroups( db );
   if( groups.length < 2 )
   {
      return;
   }

   var csName = CHANGEDPREFIX + "_largeThanPageCS161";
   var clName = CHANGEDPREFIX + "_cl161";
   commDropCS( db, csName, true, "drop cs begin" );

   var pageSize = 4096;
   var lobPageSize = 4096;
   var srcGroup = groups[0][0]["GroupName"];
   var destGroup = groups[1][0]["GroupName"];
   commCreateCS( db, csName, true, "", { PageSize: pageSize, LobPageSize: lobPageSize } );
   var cl = createCLAndPutLob( srcGroup, csName, clName, pageSize );

   splitCL( cl, srcGroup, destGroup );
   truncateAndCheckResult( cl, csName, clName );
   commDropCS( db, csName, true, "drop cs end" );
}

function createCLAndPutLob ( srcGroup, csName, clName, pageSize )
{
   var clOption = {
      "ShardingKey": { "ID_Default": 1 }, "ShardingType": "hash", "Group": srcGroup
   };

   var cl = commCreateCL( db, csName, clName, clOption, true, false, "create collection begin" );

   var lobNum = 4;
   var lobSize = pageSize * 4;

   // putLob And check LobPages
   truncatePutLob( cl, lobSize, lobNum );
   return cl;
}

function splitCL ( cl, srcGroup, destGroup )
{
   var percent = 50;
   cl.split( srcGroup, destGroup, percent );
}

function truncateAndCheckResult ( cl, csName, clName )
{
   cl.truncate();
   //after truncate ,the TotalLobPages is 0
   truncateVerify( db, csName + "." + clName );

   var expCount = 0;
   var actCount = cl.listLobs();
   assert.equal( expCount, actCount );
}
