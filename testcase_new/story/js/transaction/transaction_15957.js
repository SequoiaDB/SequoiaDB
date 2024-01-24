/***************************************************************************
@Description : seqDB-15957:事务中操作自增字段的集合，事务回滚功能
@Modify list :
2018-10-30  zhaoyu  Create
****************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName1 = COMMCLNAME + "_15957_1";
   var clName2 = COMMCLNAME + "_15957_2";
   commDropCL( db, COMMCSNAME, clName1, true, true );
   commDropCL( db, COMMCSNAME, clName2, true, true );

   var dbcl1 = commCreateCL( db, COMMCSNAME, clName1, { AutoIncrement: { Field: "id" } } );

   db.transBegin();
   var dbcl2 = commCreateCL( db, COMMCSNAME, clName2, { AutoIncrement: { Field: "id" } } );
   var doc = [];
   var expR = [];
   for( var i = 0; i < 100; i++ )
   {
      doc.push( { a: i } );
   }
   dbcl1.insert( doc );
   dbcl2.insert( doc );
   db.transRollback();

   var actR = dbcl1.find().sort( { _id: 1 } );
   checkRec( actR, expR );

   var actR = dbcl2.find().sort( { _id: 1 } );
   checkRec( actR, expR );

   dbcl1.insert( { a: "insert" } );
   dbcl2.insert( { a: "insert" } );
   expR.push( { a: "insert", id: 101 } );

   var actR = dbcl1.find().sort( { _id: 1 } );
   checkRec( actR, expR );

   var actR = dbcl2.find().sort( { _id: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName1, true, true );
   commDropCL( db, COMMCSNAME, clName2, true, true );
}
