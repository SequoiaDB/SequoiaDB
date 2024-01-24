/******************************************************************************
@Description :   seqDB-16007:  range分区集合中存在自增字段，删除集合 
@Modify list :   2018-10-18    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   var dataGroupNames = commGetDataGroupNames( db );
   if( commIsStandalone( db ) || dataGroupNames.length < 2 )
   {
      return;
   }

   var clName = COMMCLNAME + "_16007";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, {
      Group: dataGroupNames[0], ShardingKey: { a: 1 },
      ShardingType: "range", AutoIncrement: {
         Field: "id1", Increment: 2,
         StartValue: 2, MinValue: 2, MaxValue: 998, CacheSize: 10,
         AcquireSize: 2, Cycled: true, Generated: "strict"
      }
   } );
   dbcl.insert( { a: 1, b: 1 } );

   dbcl.split( dataGroupNames[0], dataGroupNames[1], 50 );

   var rc = dbcl.find();
   var expRecs = [{ "id1": 2, "a": 1, "b": 1 }];
   checkRec( rc, expRecs );

   //drop CL and check
   commDropCL( db, COMMCSNAME, clName );

   var cursor = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   while( cursor.next() )
   {
      if( cursor.current().toObj().AutoIncrement.length !== 0 )
      {
         throw new Error( "drop failed!" );
      }
   }

   commDropCL( db, COMMCSNAME, clName );
}

