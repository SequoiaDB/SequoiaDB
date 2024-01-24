/************************************
*@description: seqDB-4983:目标分区组名取最大长度值进行数据切分_ST.split.01.006
*@author :  2019-5-30 wangkexin init; 2020-1-14 huangxiaoni modify
**************************************/
main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }

   var groupNames = commGetGroups( db, false, "", true, true, true );
   var srcGroupName = groupNames[0][0].GroupName;
   var hostname = groupNames[0][1].HostName;
   // 目标组，最大长度分区组名
   var dstGroupName = "";
   for( var i = 0; i < 123; i++ )
   {
      dstGroupName += 's';
   }
   dstGroupName += "4983";

   var clName = CHANGEDPREFIX + "_split_4983";
   var recsNum = 3000;

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning." );
   removeRG( dstGroupName );
   var option = { ShardingKey: { a: 1 }, ShardingType: "range", Group: srcGroupName };
   var cl = commCreateCL( db, COMMCSNAME, clName, option, true, false );

   var docs = new Array();
   for( var i = 0; i < recsNum; i++ )
   {
      var doc = { a: i };
      docs.push( doc );
   }
   cl.insert( docs );

   try
   {
      var nodeNum = 1;
      var nodeInfo = commCreateRG( db, dstGroupName, nodeNum, hostname );
      try
      {
         cl.split( srcGroupName, dstGroupName, { a: 0 }, { a: recsNum } );
         checkResultsInGroup( srcGroupName, COMMCSNAME, clName, [] );
         checkResultsInGroup( dstGroupName, COMMCSNAME, clName, docs, { a: 1 } );
      }
      catch( e )
      {
         var svcname = nodeInfo[0].svcname;
         backupGroupLog( hostname, svcname, "split_4983" );
         throw new Error( "Error node is: " + hostname + " : " + svcname, e );
      }
   }
   finally
   {
      //清理环境
      commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end." );
      removeRG( dstGroupName );
   }
}