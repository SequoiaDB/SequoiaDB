/***************************************************************************
@Description :seqDB-15953 :Generated设置为always，插入记录
@Modify list :
              2018-10-19  zhaoyu  Create
****************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15953";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var cacheSize = 10;
   var acquireSize = 1;
   var fieldName = "id";
   var generated = "always";
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: fieldName, CacheSize: cacheSize, AcquireSize: acquireSize, Generated: generated } } );

   var clID = getCLID( db, COMMCSNAME, clName );
   var mainclSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expIncrementArr = [{ Field: fieldName, SequenceName: mainclSequenceName, Generated: generated }];
   checkAutoIncrementonCL( db, COMMCSNAME, clName, expIncrementArr );

   var expR = [];
   for( var i = 0; i < 100; i++ )
   {
      if( i % 2 === 1 )
      {
         var doc = { a: i, id: i };
         dbcl.insert( doc );
         expR.push( { a: i, id: i + 1 } );
      } else
      {
         dbcl.insert( { a: i } );
         expR.push( { a: i, id: i + 1 } );
      }
   }

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   dbcl.insert( { "id.1": 100 } );
   expR.push( { "id.1": 100, id: 101 } );

   dbcl.insert( { "id.a": 101 } );
   expR.push( { "id.a": 101, id: 102 } );
   var actR = dbcl.find().sort( { _id: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
