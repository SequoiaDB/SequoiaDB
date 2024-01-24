/***************************************************************************
  @Description :seqDB-22116: 不指定自增字段单插/批插记录，自增字段上的唯一索引冲突 
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

   var clName = COMMCLNAME + "_22116";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id" } } );
   commCreateIndex( dbcl, "id", { id: 1 }, { Unique: true } );

   dbcl.insert( [{ a: 1 }, { a: 2 }] );

   //获取自增字段名
   var clID = getCLID( db,  COMMCSNAME, clName );
   var clSequenceName = "SYS_" + clID + "_id_SEQ";

   //修改CurrentValue:1
   dbcl.setAttributes( { AutoIncrement: { Field: "id", CurrentValue: 1 } } );

   //不指定自增字段值插入1条记录，检查序列缓存已丢
   var expLastGenerateID = 1002;
   var ret = dbcl.insert( { a: 3 } );
   var actLastGenerateID = ret.toObj().LastGenerateID;
   var expSeq = { CurrentValue: 2001 };
   checkLastGenerateID( actLastGenerateID, expLastGenerateID );
   checkSequence( db, clSequenceName, expSeq );

   //检查查询结果
   var expR = [{ id: 1, a: 1 }, { id: 2, a: 2 }, { id: 1002, a: 3 }];
   var actR = dbcl.find().sort( { id: 1 } );
   commCompareResults( actR, expR );

   //再次修改CurrentValue:1
   dbcl.setAttributes( { AutoIncrement: { Field: "id", CurrentValue: 1 } } );

   //再次不指定自增字段值单插时，与id:1002索引键冲突
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      dbcl.insert( { a: 4 } );
   } );

   //检查序列缓存还原为2001
   expSeq = { CurrentValue: 2001 };
   checkSequence( db, clSequenceName, expSeq );

   //修改CurrentValue:1001
   dbcl.setAttributes( { AutoIncrement: { Field: "id", CurrentValue: 1001 } } );

   //不指定自增字段值批插时，与id:1002索引键冲突
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      dbcl.insert( [{ a: 5 }, { a: 6 }, { a: 7 }] );
   } );

   //检查序列缓存还原为2001
   expSeq = { CurrentValue: 2001 };
   checkSequence( db, clSequenceName, expSeq );

   //检查查询结果
   expR = [{ id: 1, a: 1 }, { id: 2, a: 2 }, { id: 1002, a: 3 }];
   actR = dbcl.find().sort( { id: 1 } );
   commCompareResults( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
