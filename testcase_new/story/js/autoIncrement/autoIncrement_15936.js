/***************************************************************************
@Description :seqDB-15936 :同一个coord不指定自增字段插入记录 
@Modify list :
              2018-10-15  zhaoyu  Create
****************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   };

   var clName = COMMCLNAME + "_15936";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id" } } );
   commCreateIndex( dbcl, "id", { id: 1 }, { Unique: true }, true );

   var doc = [];
   var expR = [];
   for( var i = 1; i < 2001; i++ )
   {
      doc.push( { a: i, b: i, c: i + "test" } );
      expR.push( { a: i, b: i, c: i + "test", id: i } );
   }
   dbcl.insert( doc );

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
