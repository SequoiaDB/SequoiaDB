/***************************************************************************
@Description : seqDB-15977:更新记录，自增字段值保留，分区键与自增字段相同
@Modify list :
              2018-10-17  zhaoyu  Create
****************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15977_1";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { id: 1 } } );
   dbcl.insert( { a: 1 } );

   dbcl.createAutoIncrement( { Field: "id" } );
   dbcl.insert( { a: 2 } );
   dbcl.update( { $replace: { c: 100 } } );
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{ c: 100 }, { c: 100, id: 1 }];
   checkRec( actR, expR );

   dbcl.update( { $set: { c: 1000 } } );
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{ c: 1000 }, { c: 1000, id: 1 }];
   checkRec( actR, expR );

   dbcl.update( { $set: { id: 1000 } } );
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{ c: 1000 }, { c: 1000, id: 1 }];
   checkRec( actR, expR );

   dbcl.insert( { a: 1 } );
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{ c: 1000 }, { c: 1000, id: 1 }, { a: 1, id: 2 }];
   checkRec( actR, expR );

   dbcl.update( { $unset: { id: "" } }, { id: { $exists: 1 } } );
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{ c: 1000 }, { c: 1000, id: 1 }, { a: 1, id: 2 }];
   checkRec( actR, expR );

   dbcl.update( { $replace: { id: 100 } } );
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{}, { id: 1 }, { id: 2 }];
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
