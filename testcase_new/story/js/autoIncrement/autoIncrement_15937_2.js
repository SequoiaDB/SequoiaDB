/***************************************************************************
@Description :seqDB-15937 :不同coord不指定自增字段插入记录，趋势递增
@Modify list :
              2018-10-15  zhaoyu  Create
****************************************************************************/

main( test );
function test ()
{
   var sortField = 0;
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15937_2";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var acquireSize = 10;
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id", AcquireSize: acquireSize } } );
   commCreateIndex( dbcl, "id", { id: 1 }, { Unique: true }, true );

   var coordNodes = getCoordNodeNames( db );
   var coordNum = coordNodes.length;
   var expR = [];
   var getCacheNum = 0;
   for( var j = 1; j < 30; j++ )
   {
      if( j > 1 && j % 2 === 1 )
      {
         getCacheNum++;
      }

      for( var k = 0; k < coordNum; k++ )
      {
         var coord = new Sdb( coordNodes[k] );
         var cl = coord.getCS( COMMCSNAME ).getCL( clName );
         var doc = [];
         for( var i = 1; i < 6; i++ )
         {
            doc.push( { a: sortField, b: i, c: i + "test" } );
            if( j % 2 !== 0 )
            {
               expR.push( { a: sortField, b: i, c: i + "test", id: getCacheNum * coordNum * acquireSize + k * acquireSize + i } );
            } else
            {
               expR.push( { a: sortField, b: i, c: i + "test", id: getCacheNum * coordNum * acquireSize + k * acquireSize + 5 + i } );
            }
            sortField++;
         }
         cl.insert( doc );
         coord.close();
      }

   }

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
