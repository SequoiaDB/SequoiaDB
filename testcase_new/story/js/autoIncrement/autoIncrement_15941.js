/***************************************************************************
@Description :seqDB-15941 :指定自增字段、不指定自增字段交替插入记录
@Modify list :
              2018-10-16  zhaoyu  Create
****************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15941";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id", AcquireSize: 10 } } );

   var expR = [];
   var currentValue = 0;
   for( var i = 0; i < 100; i++ )
   {
      if( i % 2 === 0 )
      {
         currentValue = i + 10;
         var doc = { a: i, id: currentValue };
         dbcl.insert( doc );
         expR.push( doc );
      } else
      {
         dbcl.insert( { a: i } );
         expR.push( { a: i, id: currentValue + 1 } );
      }
   }

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
