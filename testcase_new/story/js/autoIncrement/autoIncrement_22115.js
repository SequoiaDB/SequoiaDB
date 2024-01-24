/***************************************************************************
  @Description :seqDB-22115: 复合唯一索引包含自增字段，指定自增字段插入记录，唯一索引冲突 
  @Modify list :
  2020-04-26  liuxiaoxuan  Create
 ****************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   };

   var clName = COMMCLNAME + "_22115";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id" } } );
   commCreateIndex( dbcl, "id_a", { id: 1, a: 1 }, { Unique: true } );

   dbcl.insert( [{ a: 1 }, { a: 1 }] );

   //获取自增字段名
   var clID = getCLID( db, COMMCSNAME, clName );
   var clSequenceName = "SYS_" + clID + "_id_SEQ";

   //插入重复键
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      dbcl.insert( { id: 2, a: 1 } );
   } );

   //继续不指定自增字段值插入1条记录，检查序列缓存未丢
   var expLastGenerateID = 3;
   var ret = dbcl.insert( { a: 1 } );
   var actLastGenerateID = ret.toObj().LastGenerateID;
   var expSeq = { CurrentValue: 1000 };
   checkLastGenerateID( actLastGenerateID, expLastGenerateID );
   checkSequence( db, clSequenceName, expSeq );


   //更新后与原有记录冲突
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      dbcl.update( { $set: { id: 1 } }, { a: 1 } );
   } );

   //继续插入1条记录，检查序列缓存未丢
   expLastGenerateID = 4;
   ret = dbcl.insert( { a: 4 } );
   actLastGenerateID = ret.toObj().LastGenerateID;
   checkLastGenerateID( actLastGenerateID, expLastGenerateID );
   checkSequence( db, clSequenceName, expSeq );

   expR = [{ id: 1, a: 1 }, { id: 2, a: 1 }, { id: 3, a: 1 }, { id: 4, a: 4 }];
   actR = dbcl.find().sort( { id: 1 } );
   commCompareResults( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
