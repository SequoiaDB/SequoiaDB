/******************************************************************************
 * @Description : seqDB-24319:创建复合索引指定NotNull，插入记录中有一个索引键不存在
 * @Author        : Zhang Yanan
 * @CreateTime    : 2021.08.09
 * @LastEditTime  : 2021.08.12
 * @LastEditors   : Zhang Yanan
******************************************************************************/

testConf.clName = COMMCLNAME + "_24319";
main( test );

function test ( args )
{
   var varCL = args.testCL;

   var NotNull = true;
   var indexName = "abIndex";
   var indexKey = { a: 1, b: 1 };
   varCL.createIndex( indexName, indexKey, { NotNull: NotNull } );

   var strType = [{ a: "abcdefghijklmn" }];

   assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
   {
      varCL.insert( strType );
   } );

   var timeType = [{ a: { "$timestamp": "1901-12-31T15:54:03.000Z" } },
   { a: { "$date": "2012-01-01" } }];
   assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
   {
      varCL.insert( timeType[0] );
   } );
   assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
   {
      varCL.insert( timeType[1] );
   } );

   var arrType = [{ a: [{ a1: 1 }, { a2: 2 }, { a3: 3 }, { a4: 4 }] }];
   assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
   {
      varCL.insert( arrType );
   } );

   var objOidType = [{ a: { "$oid": "123abcd00ef12358902300ef" } }];
   assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
   {
      varCL.insert( objOidType );
   } );

   var binaryType = [{ a: [{ "$binary": "aGVsbG8=" }, { "$type": "1" }] }];
   assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
   {
      varCL.insert( binaryType );
   } );

   var floatType = [{ a: 123.456 }]
   assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
   {
      varCL.insert( floatType );
   } );

   var preType = [{ a: { "$decimal": "1.88888E+308" } }];
   assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
   {
      varCL.insert( preType );
   } );

   var bolType = [{ a: true }];
   assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
   {
      varCL.insert( bolType );
   } );

   var regType = [{ a: { "$regex": "^W", "$options": "i" } }];
   assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
   {
      varCL.insert( regType );
   } );

   var intType = [{ a: 123 }, { a: 3000000000 }];
   assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
   {
      varCL.insert( intType[0] );
   } );
   assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
   {
      varCL.insert( intType[1] );
   } );

   var index = varCL.listIndexes();
   while( index.next() )
   {
      var clIndexName = index.next().toObj().IndexDef.name;
   }
   index.close();
   assert.equal( clIndexName, indexName );

   var expData = [];
   var cursor = varCL.find();
   commCompareResults( cursor, expData );
}



