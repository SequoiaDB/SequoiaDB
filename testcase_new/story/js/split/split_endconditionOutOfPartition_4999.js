/************************************
*@description ：seqDB-4999:hash范围切分设置endcondition条件值超出partition范围
*@author ：2019-5-30 wangkexin init; 2020-1-14 huangxiaoni modify
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
   var clName = CHANGEDPREFIX + "_split4999";

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning." );
   var options = { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 1024, Group: srcGroupName };
   var cl = commCreateCL( db, COMMCSNAME, clName, options );
   var docs = insertData( cl, 100 );

   cl.split( srcGroupName, dstGroupName, { "Partition": 10 }, { "Partition": 1025 } );

   // check records
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { a: 1 } );
   commCompareResults( cursor, docs );

   // check cl groups
   var clGroupNames = commGetCLGroups( db, COMMCSNAME + "." + clName );
   var expGroupNames = [srcGroupName, dstGroupName];
   if( clGroupNames.length !== 2 || JSON.stringify( clGroupNames ) !== JSON.stringify( expGroupNames ) )
   {
      throw new Error( "expCLGroups = [" + expGroupNames + "], actCLGroups = [" + clGroupNames + "]" );
   }

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end." );
}