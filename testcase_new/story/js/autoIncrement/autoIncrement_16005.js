/***************************************************************************
@Description :seqDB-16005 :创建集合时，创建自增字段允许循环
@Modify list :
              2018-10-25  zhaoyu  Create
****************************************************************************/

main( test );
function test ()
{
   var sortField = 0;
   var coordNodes = getCoordNodeNames( db );
   var coordNum = coordNodes.length;
   if( commIsStandalone( db ) || coordNum !== 3 )
   {
      return;
   };

   var clName = COMMCLNAME + "_16005";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var increment = 13;
   var acquireSize = 11;
   var cacheSize = 33;
   var minValue = -1333;
   var maxValue = 13001;
   var cycled = true;
   var fieldName = "id";
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: fieldName, CacheSize: cacheSize, AcquireSize: acquireSize, Increment: increment, MinValue: minValue, MaxValue: maxValue, Cycled: cycled } } );

   var expR = [];
   for( var i = 0; i < 1001; i++ )
   {
      dbcl.insert( { a: sortField } );
      expR.push( { a: sortField, id: 1 + increment * i } );
      sortField++;
   }

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   for( var k = 0; k < coordNum; k++ )
   {
      var coord = new Sdb( coordNodes[k] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { a: sortField } );
      expR.push( { a: sortField, id: minValue + k * acquireSize * increment } );
      sortField++;
      coord.close();
   }
   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   dbcl.dropAutoIncrement( fieldName );
   var increment = -13;
   var acquireSize = 11;
   var cacheSize = 33;
   var minValue = -13001;
   var maxValue = 0;
   dbcl.createAutoIncrement( { Field: fieldName, CacheSize: cacheSize, AcquireSize: acquireSize, Increment: increment, MinValue: minValue, MaxValue: maxValue, Cycled: cycled } );
   for( var i = 0; i < 1001; i++ )
   {
      dbcl.insert( { a: sortField } );
      expR.push( { a: sortField, id: -1 + increment * i } );
      sortField++;
   }

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   for( var k = 0; k < coordNum; k++ )
   {
      var coord = new Sdb( coordNodes[k] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { a: sortField } );
      expR.push( { a: sortField, id: maxValue + k * acquireSize * increment } );
      sortField++;
      coord.close();
   }
   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
