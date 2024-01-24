/***************************************************************************
@Description :seqDB-15940 :指定自增字段批量插入记录
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

   var clName = COMMCLNAME + "_15940";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id", AcquireSize: 10 } } );

   var coordNodes = getCoordNodeNames( db );
   var coordNum = coordNodes.length;
   var expR = [];
   for( var k = 0; k < coordNum; k++ )
   {
      var coord = new Sdb( coordNodes[k] );
      //coord.invalidateCache();
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      for( var j = 0; j < 2; j++ )
      {
         var doc = [];
         for( var i = 0; i < 2001; i++ )
         {
            doc.push( { a: sortField, b: i, c: i + "test", id: i } );
            expR.push( { a: sortField, b: i, c: i + "test", id: i } );
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
