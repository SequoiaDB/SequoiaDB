/***************************************************************************
  @Description :seqDB-22111: 用户指定自增字段插入/更新自增字段/非自增字段唯一索引冲突，不丢弃序列缓存 
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

   var clName = COMMCLNAME + "_22111";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id" } } );
   commCreateIndex( dbcl, "id", { id: 1 }, { Unique: true } );
   commCreateIndex( dbcl, "a", { a: 1 }, { Unique: true } );

   dbcl.insert( [{ a: 1 }, { a: 2 }] );

   //获取自增字段名
   var clID = getCLID( db, COMMCSNAME, clName );
   var clSequenceName = "SYS_" + clID + "_id_SEQ";

   //指定自增字段值单插，插入重复键
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      dbcl.insert( { id: 2 } );
   } );

   //继续不指定自增字段值插入1条记录，检查序列缓存未丢
   var expLastGenerateID = 3;
   var ret = dbcl.insert( { a: 3 } );
   var actLastGenerateID = ret.toObj().LastGenerateID;
   var expSeq = { CurrentValue: 1000 };
   checkLastGenerateID( actLastGenerateID, expLastGenerateID );
   checkSequence( db, clSequenceName, expSeq );

   //检查查询结果
   var expR = [{ id: 1, a: 1 }, { id: 2, a: 2 }, { id: 3, a: 3 }];
   var actR = dbcl.find().sort( { id: 1 } );
   commCompareResults( actR, expR );


   //更新自增字段值后与原有记录冲突
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      dbcl.update( { $set: { id: 3 } }, { a: 1 } );
   } );

   //继续插入1条记录，检查序列缓存未丢
   expLastGenerateID = 4;
   ret = dbcl.insert( { a: 4 } );
   actLastGenerateID = ret.toObj().LastGenerateID;
   checkLastGenerateID( actLastGenerateID, expLastGenerateID );
   checkSequence( db, clSequenceName, expSeq );

   expR = [{ id: 1, a: 1 }, { id: 2, a: 2 }, { id: 3, a: 3 }, { id: 4, a: 4 }];
   actR = dbcl.find().sort( { id: 1 } );
   commCompareResults( actR, expR );


   //不指定自增字段值单插，插入重复键
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      dbcl.insert( { a: 3 } );
   } );

   //继续不指定自增字段值插入1条记录，检查序列缓存未丢
   expLastGenerateID = 6;
   ret = dbcl.insert( { a: 6 } );
   actLastGenerateID = ret.toObj().LastGenerateID;
   expSeq = { CurrentValue: 1000 };
   checkLastGenerateID( actLastGenerateID, expLastGenerateID );
   checkSequence( db, clSequenceName, expSeq );

   //检查查询结果
   expR = [{ id: 1, a: 1 }, { id: 2, a: 2 }, { id: 3, a: 3 }, { id: 4, a: 4 }, { id: 6, a: 6 }];
   actR = dbcl.find().sort( { id: 1 } );
   commCompareResults( actR, expR );


   //更新非自增字段值后与原有记录冲突
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      dbcl.update( { $set: { a: 3 } }, { id: 4 } );
   } );

   //继续插入1条记录，检查序列缓存未丢
   expLastGenerateID = 7;
   ret = dbcl.insert( { a: 7 } );
   actLastGenerateID = ret.toObj().LastGenerateID;
   checkLastGenerateID( actLastGenerateID, expLastGenerateID );
   checkSequence( db, clSequenceName, expSeq );

   expR = [{ id: 1, a: 1 }, { id: 2, a: 2 }, { id: 3, a: 3 }, { id: 4, a: 4 }, { id: 6, a: 6 }, { id: 7, a: 7 }];
   actR = dbcl.find().sort( { id: 1 } );
   commCompareResults( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
