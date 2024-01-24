/************************************
*@description ：seqDB-10506:切分源组和目标组不在同一domain中 
*@author ：2019-3-13 wangkexin init; 2020-01-13 huangxiaoni modify
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

   var groupNames = commGetDataGroupNames( db );
   var srcGroupName = groupNames[0];
   var dstGroupName = groupNames[1];
   var dmName = CHANGEDPREFIX + "_split_4992";
   var csName = CHANGEDPREFIX + "_split_4992";
   var clName = "cl";

   commDropCS( db, csName );
   commDropDomain( db, dmName );
   commCreateDomain( db, dmName, [srcGroupName], { AutoSplit: true } );
   commCreateCS( db, csName, false, "", { Domain: dmName } );
   var options = { ShardingKey: { a: 1 }, ShardingType: "hash" };
   var cl = commCreateCL( db, csName, clName, options );
   insertData( cl, 100 );

   assert.tryThrow( SDB_CAT_GROUP_NOT_IN_DOMAIN, function()
   {
      cl.split( srcGroupName, dstGroupName, { Partition: 10 }, { Partition: 20 } );
   } );

   var clGroupNames = commGetCLGroups( db, csName + "." + clName );
   if( clGroupNames.length !== 1 || clGroupNames[0] !== srcGroupName )
   {
      throw new Error( "expCLGroups = [" + srcGroupName + "], actCLGroups = [" + clGroupNames + "]" );
   }

   commDropCS( db, csName, false );
   commDropDomain( db, dmName, false );
}