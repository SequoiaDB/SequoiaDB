/***************************************************************************
@Description : seqDB-16282:事务中执行replace shardingKey/自增字段，回滚功能验证, 分区键与自增字段相同
@Modify list :
2018-10-29  zhaoyu  Create
****************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_16282_1";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { id: 1 } } );
   dbcl.insert( { a: 1 } );

   dbcl.createAutoIncrement( { Field: "id" } );
   dbcl.insert( { a: 2 } );

   db.transBegin();
   dbcl.update( { $replace: { id: 100 } } );
   db.transRollback();
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{ a: 1 }, { a: 2, id: 1 }];
   checkRec( actR, expR );

   db.transBegin();
   dbcl.update( { $replace: { id: 100, a: 100, c: 100 } } );
   db.transRollback();
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{ a: 1 }, { a: 2, id: 1 }];
   checkRec( actR, expR );

   db.transBegin();
   dbcl.update( { $replace: { id: 100, a: 100, c: 100 } } );
   db.transCommit();
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{ a: 100, c: 100 }, { id: 1, a: 100, c: 100 }];
   checkRec( actR, expR );


   db.transBegin();
   dbcl.update( { $replace: { id: 100 } } );
   db.transCommit();
   var actR = dbcl.find().sort( { _id: 1 } );
   var expR = [{}, { id: 1 }];
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
