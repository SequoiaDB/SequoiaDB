/************************************
*@description: seqDB-4978:源分区组名取最大长度值进行数据切分_ST.split.01.001
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
   // 源组，最大长度分区组名
   var srcGroupName = "";
   for( var i = 0; i < 123; i++ )
   {
      srcGroupName += 's';
   }
   srcGroupName += "4978";
   // 目标组
   var dstGroupName = groupNames[0][0].GroupName;
   var hostname = groupNames[0][1].HostName;

   var clName = CHANGEDPREFIX + "_split_4978";
   var recsNum = 3000;

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning." );
   removeRG( srcGroupName );

   try
   {
      var nodeNum = 1;
      var nodeInfo = commCreateRG( db, srcGroupName, nodeNum, hostname );
      try
      {
         var option = { ShardingKey: { a: 1 }, ShardingType: "range", Group: srcGroupName };
         var cl = commCreateCL( db, COMMCSNAME, clName, option, true, false );

         var docs = new Array();
         for( var i = 0; i < recsNum; i++ )
         {
            var doc = { a: i };
            docs.push( doc );
         }
         cl.insert( docs );
         checkResultsInGroup( srcGroupName, COMMCSNAME, clName, docs, { a: 1 } );

         cl.split( srcGroupName, dstGroupName, { a: 0 }, { a: recsNum } );
         checkResultsInGroup( srcGroupName, COMMCSNAME, clName, [] );
         checkResultsInGroup( dstGroupName, COMMCSNAME, clName, docs, { a: 1 } );
      }
      catch( e )
      {
         var svcname = nodeInfo[0].svcname;
         backupGroupLog( hostname, svcname, "split_4978" );
         throw e;
      }
   }
   finally
   {
      //清理环境
      commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end." );
      removeRG( srcGroupName );
   }
}