/******************************************************************************
 * @Description   : seqDB-26414:事务中修改无事务属性
 * @Author        : liuli
 * @CreateTime    : 2022.04.24
 * @LastEditTime  : 2022.09.20
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26414";
testConf.useSrcGroup = true;

main( test );
function test ( args )
{
   var dbcl = args.testCL;

   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   var docs2 = [];
   for( var i = 2000; i < 2500; i++ )
   {
      docs2.push( { a: i, b: i } );
   }

   // 插入数据
   dbcl.insert( docs );

   // 开启事务 
   db.transBegin();

   // 插入数据
   dbcl.insert( docs2 );

   // 修改集合属性NoTrans为true
   assert.tryThrow( SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST, function()
   {
      dbcl.alter( { NoTrans: true } );
   } );

   // 查询事务中插入的数据并校验
   var actResult = dbcl.find( { a: { "$gte": 2000 } } ).sort( { a: 1 } );
   commCompareResults( actResult, docs2 );

   // 回滚事务
   db.transRollback();

   // 校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );

   // 开启事务
   db.transBegin();

   // 查询数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );

   // 修改集合属性NoTrans为true
   dbcl.alter( { NoTrans: true } );

   // 再次插入一些数据
   var docs3 = [];
   for( var i = 2000; i < 2500; i++ )
   {
      docs3.push( { a: i, b: i } );
      docs.push( { a: i, b: i } );
   }

   dbcl.insert( docs3 );

   // 回滚事务
   db.transRollback();

   // 校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );

   var expAttribute = "Compressed | NoTrans";
   if( !commIsStandalone( db ) )
   {
      // SDB_SNAP_CATALOG快照校验集合属性
      var cursor = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + testConf.clName } );
      while( cursor.next() )
      {
         var attributeDesc = cursor.current().toObj()["AttributeDesc"];
         assert.equal( attributeDesc, expAttribute );
      }
      cursor.close();
   }

   // 校验LSN，确保备节点配置也修改成功
   commCheckLSN( db, args.srcGroupName, 120 );

   // SDB_SNAP_COLLECTIONS快照校验集合属性
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: COMMCSNAME + "." + testConf.clName, RawData: true } );
   while( cursor.next() )
   {
      var attributeDesc = cursor.current().toObj()["Details"][0]["Attribute"];
      assert.equal( attributeDesc, expAttribute );
   }
   cursor.close();
}