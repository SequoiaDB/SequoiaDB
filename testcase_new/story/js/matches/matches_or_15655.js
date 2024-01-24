/************************************
 * Description:   hash表使用$or查询，$or指定多个子条件  
 * Author:        linsuqiang
 * Date:          2018.08.21
 * Testlink:      seqDB-15655
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   if( 2 > commGetGroupsNum( db ) )
   {
      return;
   }

   // get src and dest group
   var groups = commGetGroups( db );
   var srcGroupName = groups[0][0].GroupName;
   var dstGroupName = groups[1][0].GroupName;

   var csName = COMMCSNAME;
   commCreateCS( db, csName, true );

   var clName = "cl15655";
   var options = { Group: srcGroupName, ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0 };
   var cl = commCreateCL( db, csName, clName, options, true );

   cl.split( srcGroupName, dstGroupName, 50 );

   cl.insert( { a: "I" } );
   cl.insert( { a: "K" } );
   cl.insert( { a: "Y" } );
   var cursor = cl.find( { $or: [{ a: { $lt: "K" } }, { a: { $et: "Y" } }] },
      { _id: { $include: 0 } } )
      .sort( { a: 1 } );
   var expRes = [{ a: "I" }, { a: "Y" }];
   var actRes = [];
   while( cursor.next() )
   {
      actRes.push( cursor.current().toObj() );
   }
   var expResStr = JSON.stringify( expRes );
   var actResStr = JSON.stringify( actRes );
   if( expResStr != actResStr )
   {
      throw new Error( "main", null, "query", expResStr, actResStr );
   }

   commDropCL( db, csName, clName, false );
}


