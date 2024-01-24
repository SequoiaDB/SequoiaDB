/************************************
*@Description: seqDB-18623:反转自增队列方向，自增字段未使用时，使CurrentValue不在修改后的区间内
*@Author     : 2019.07.18 yinzhen 
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_auto_increment_18623";
   commDropCL( db, COMMCSNAME, clName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   cl.createAutoIncrement( { Field: "id1", CacheSize: 1, AcquireSize: 1, Increment: -1, StartValue: -100 } );
   cl.createAutoIncrement( { Field: "id2", CacheSize: 1, AcquireSize: 1, Increment: 1, StartValue: 100 } );

   // 检查自增字段属性是否正确
   var clUniqueID = getCLID( db, COMMCSNAME, clName );
   var sequenceName_1 = "SYS_" + clUniqueID + "_id1_SEQ";
   var sequenceName_2 = "SYS_" + clUniqueID + "_id2_SEQ";
   var expSequenceObj_1 = { CacheSize: 1, AcquireSize: 1, Increment: -1, StartValue: -100, CurrentValue: -100, "MaxValue": -1, "MinValue": { "$numberLong": "-9223372036854775808" } };
   var expSequenceObj_2 = { CacheSize: 1, AcquireSize: 1, Increment: 1, StartValue: 100, CurrentValue: 100 };
   checkSequence( db, sequenceName_1, expSequenceObj_1 );
   checkSequence( db, sequenceName_2, expSequenceObj_2 );

   // 修改自增属性字段，自增序列修改为自减序列、自减序列修改为自增序列，同时使修改前自增序列的CurrentValue不在修改后[MinValue,MaxValue]范围内
   expSequenceObj_1 = { CacheSize: 1, AcquireSize: 1, Increment: 1, StartValue: 102, CurrentValue: 102, "MaxValue": 100000, "MinValue": 0 };
   expSequenceObj_2 = { CacheSize: 1, AcquireSize: 1, Increment: -1, StartValue: -102, CurrentValue: -102, MinValue: -100000, MaxValue: -1 };
   cl.alter( { AutoIncrement: { Field: "id1", CacheSize: 1, AcquireSize: 1, Increment: 1, StartValue: 102, MinValue: 0, MaxValue: 100000 } } );
   cl.alter( { AutoIncrement: { Field: "id2", CacheSize: 1, AcquireSize: 1, Increment: -1, StartValue: -102, MinValue: -100000, MaxValue: -1 } } );
   checkSequence( db, sequenceName_1, expSequenceObj_1 );
   checkSequence( db, sequenceName_2, expSequenceObj_2 );

   // 通过本coord和其它coord插入记录查询
   var coordList = getCoordNodeNames( db );
   var insertCount = { count: 0 };
   var expList = [];
   for( var i in coordList )
   {
      var dbcl = new Sdb( coordList[i] ).getCS( COMMCSNAME ).getCL( clName );
      var cur = dbcl.find().sort( { "id1": 1 } );
      insertAndGetExpList( dbcl, 1, -1, 102 + insertCount.count - 1, -102 - insertCount.count + 1, expList, insertCount )
      checkRec( cur, expList );
   }

   commDropCL( db, COMMCSNAME, clName, true, true );
}
