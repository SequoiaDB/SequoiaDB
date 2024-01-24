/************************************
*@Description: 分区表修改Partition
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14956, seqDB-14966
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_14956";

   var options = { ShardingType: 'hash', ShardingKey: { a: 1 } };
   var cl = commCreateCL( db, csName, clName, options, true, false, "create CL in the begin" );

   cl.setAttributes( { Partition: 8192 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Partition", 8192 );

   var group = getSplitGroup( db, csName, clName );
   alterGroup( cl, { Group: group.tarGroup } );
   var nowGroup = commGetCLGroups( db, csName + "." + clName )[0];

   assert.equal( group.srcGroup, nowGroup );

   commDropCL( db, csName, clName, true, false, "clean cl" );
}

//修改Group报-6，原本就是这样定义的，所以不改动
function alterGroup ( cl, alterOption )
{
   try
   {
      cl.setAttributes( alterOption );
      throw new Error( "ALTER_SHOULD_ERR" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
}