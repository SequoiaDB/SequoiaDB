/***************************************************************************
@Description : seqDB-15977:更新记录，自增字段值保留，分区键与自增字段不同
               seqDB-16715:使用replace操作符replace自增字段后，再使用unset操作符更新自增字段
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

   var clName = COMMCLNAME + "_15977_2";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { a: 1 } } );
   dbcl.insert( { a: 1 } );
   dbcl.insert( { b: 1 } );

   dbcl.createAutoIncrement( { Field: "id" } );
   dbcl.insert( { a: 2 } );
   dbcl.insert( { b: 2 } );

   dbcl.update( { $replace: { c: 100 } } );
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{ a: 1, c: 100 }, { c: 100 }, { a: 2, c: 100, id: 1 }, { c: 100, id: 2 }];
   checkRec( actR, expR );

   dbcl.update( { $set: { b: 1000 } } );
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{ a: 1, c: 100, b: 1000 }, { c: 100, b: 1000 }, { a: 2, c: 100, b: 1000, id: 1 }, { c: 100, b: 1000, id: 2 }];
   checkRec( actR, expR );

   dbcl.update( { $set: { id: 1000 } } );
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{ a: 1, c: 100, b: 1000, id: 1000 }, { c: 100, b: 1000, id: 1000 }, { a: 2, c: 100, b: 1000, id: 1000 }, { c: 100, b: 1000, id: 1000 }];
   checkRec( actR, expR );

   dbcl.insert( { a: 1 } );
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{ a: 1, c: 100, b: 1000, id: 1000 }, { c: 100, b: 1000, id: 1000 }, { a: 2, c: 100, b: 1000, id: 1000 }, { c: 100, b: 1000, id: 1000 }, { a: 1, id: 3 }];
   checkRec( actR, expR );

   dbcl.update( { $replace: { a: 100 } } );
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{ a: 1, id: 1000 }, { id: 1000 }, { a: 2, id: 1000 }, { id: 1000 }, { a: 1, id: 3 }];
   checkRec( actR, expR );

   dbcl.update( { $replace: { id: 100 } } );
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{ a: 1, id: 100 }, { id: 100 }, { a: 2, id: 100 }, { id: 100 }, { a: 1, id: 100 }];
   checkRec( actR, expR );

   //SEQUOIADBMAINSTREAM-3871
   dbcl.update( { $unset: { id: "" } } );
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{ a: 1 }, {}, { a: 2 }, {}, { a: 1 }];
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
