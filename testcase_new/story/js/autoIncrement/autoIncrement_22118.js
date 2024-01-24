/***************************************************************************
  @Description :seqDB-22118: 更新自增字段，更新后的值与自动生成的自增字段值冲突 
  @Modify list :
  2020-04-27  liuxiaoxuan  Create
 ****************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   };

   var clName = COMMCLNAME + "_22118";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id" } } );
   commCreateIndex( dbcl, "id", { id: 1 }, { Unique: true } );

   //获取自增字段名
   var clID = getCLID( db, COMMCSNAME, clName );
   var clSequenceName = "SYS_" + clID + "_id_SEQ";

   var expLastGenerateID = 1;
   var ret = dbcl.insert( [{ a: 1 }, { a: 2 }] );
   var actLastGenerateID = ret.toObj().LastGenerateID;
   var expSeq = { CurrentValue: 1000 };
   checkLastGenerateID( actLastGenerateID, expLastGenerateID );
   checkSequence( db, clSequenceName, expSeq );

   //指定自增字段执行更新，且更新后的值与自动生成的自增字段值冲突
   dbcl.update( { $set: { id: 3 } }, { id: 2 } );

   //继续插入1条记录，检查序列缓存已丢
   expLastGenerateID = 1001;
   ret = dbcl.insert( { a: 1001 } );
   actLastGenerateID = ret.toObj().LastGenerateID;
   checkLastGenerateID( actLastGenerateID, expLastGenerateID );
   expSeq = { CurrentValue: 2000 };
   checkSequence( db, clSequenceName, expSeq );

   expR = [{ id: 1, a: 1 }, { id: 3, a: 2 }, { id: 1001, a: 1001 }];
   actR = dbcl.find().sort( { id: 1 } );
   commCompareResults( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
