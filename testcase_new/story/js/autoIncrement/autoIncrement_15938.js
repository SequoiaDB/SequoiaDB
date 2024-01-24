/***************************************************************************
@Description :seqDB-15938 :不指定自增字段批量插入记录
@Modify list :
              2018-10-16  zhaoyu  Create
****************************************************************************/

main( test );
function test ()
{
   var sortField = 0;
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15938";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var acquireSize = 1;
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id", AcquireSize: acquireSize } } );
   commCreateIndex( dbcl, "id", { id: 1 }, { Unique: true }, true );

   var coordNodes = getCoordNodeNames( db );
   var coordNum = coordNodes.length;
   var expR = [];
   for( var k = 0; k < coordNum; k++ )
   {
      var coord = new Sdb( coordNodes[k] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      for( var j = 0; j < 2; j++ )
      {
         var doc = [];
         for( var i = 1; i < 2001; i++ )
         {
            doc.push( { a: sortField, b: i, c: i + "test" } );
            expR.push( { a: sortField, b: i, c: i + "test", id: k * 4000 + j * 2000 + i } );
            sortField++;
         }
         cl.insert( doc );
      }
      coord.close();
   }

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
