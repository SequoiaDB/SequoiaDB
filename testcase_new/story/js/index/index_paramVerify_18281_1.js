
/******************************************************************************
 * @Description   : seqDB-18281:options参数校验
 *                : seqDB-24016:createIndex接口验证
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2021.05.12
 * @LastEditTime  : 2022.01.29
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_18281_1";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "index_18281";
   var indexDef = { a: 1 };

   // 字段使用小写
   var option = { Unique: true, Enforced: true };
   cl.createIndex( indexName, indexDef, { unique: true, enforced: true } );

   checkIndex( cl, indexName, indexDef, option );
   cl.dropIndex( indexName );

   // 指定非法字段
   var keyArr = [{ isUnique: true }, { enforced: true }, { sortBufferSize: true }, { notNull: true }, { aa: true }, { notArray: true }];
   for( var i = 0; i < keyArr.length; i++ ) 
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         cl.createIndex( indexName, { a: 1 }, keyArr[i] );
      } );

      assert.tryThrow( SDB_IXM_NOTEXIST, function()
      {
         cl.getIndex( indexName );
      } );
   }

   // 使用默认字段
   cl.createIndex( indexName, { a: 1 } );
   checkIndex( cl, indexName, indexDef );
   cl.dropIndex( indexName );

   // 同一字段指定不同名称
   var keyArr = [{ enforced: true, Enforced: false }, { unique: false, Unique: false }, { NotNull: true, aa: false }, { NotArray: true, aa: false }];
   for( var i = 0; i < keyArr.length; i++ )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         cl.createIndex( indexName, { a: 1 }, keyArr[i] );
      } );
   }

   // 指定boolean类型为0
   cl.createIndex( indexName, { a: 1 }, { unique: 0, enforced: 0, NotNull: 0, NotArray: 0 } );
   var recs = [{ a: 1, b: 1 }, { b: 2 }, { a: null, b: 3 }, { a: 1, b: 4 }];
   cl.insert( recs );
   checkRecords( cl, recs );

   checkIndex( cl, indexName, indexDef );
   cl.dropIndex( indexName );
   cl.remove();

   // 指定boolean类型为1
   var option = { Unique: true, Enforced: true, NotNull: true, NotArray: 1 };
   cl.createIndex( indexName, { a: 1 }, { unique: 1, enforced: 1, NotNull: 1, NotArray: 1 } );
   checkIndex( cl, indexName, indexDef, option );

   var valRecs = [{ a: 1, b: 1 }];
   var invRecs = [{ b: 2 }, { a: null, b: 3 }];
   cl.insert( valRecs );
   for( i = 0; i < invRecs.length; i++ ) 
   {
      assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
      {
         cl.insert( invRecs[i] );
      } );
   }
   checkRecords( cl, valRecs );

   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.insert( { a: 1, b: 4 } );
   } );
   cl.dropIndex( indexName );
   cl.remove();

   // 指定NotNull对应值为其他数字或字符串
   var keyArr = [{ NotNull: "true" }, { NotNull: "false" }, { NotNull: 2 }, { NotNull: "a" }];
   for( var i = 0; i < keyArr.length; i++ )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         cl.createIndex( indexName, { a: 1 }, keyArr[i] );
      } );
   }

   // 指定NotArray对应值为其他数字或字符串
   var keyArr = [{ NotArray: "true" }, { NotArray: "false" }, { NotArray: 2 }, { NotArray: "a" }];
   for( var i = 0; i < keyArr.length; i++ )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         cl.createIndex( indexName, { a: 1 }, keyArr[i] );
      } );
   }

   // 指定Enforced对应值为其他数字或字符串
   var keyArr = [{ Enforced: "true" }, { Enforced: "false" }, { Enforced: 2 }, { Enforced: "a" }];
   for( var i = 0; i < keyArr.length; i++ )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         cl.createIndex( indexName, { a: 1 }, keyArr[i] );
      } );
   }

   // 指定Unique对应值为其他数字或字符串
   var keyArr = [{ Unique: "true" }, { Unique: "false" }, { Unique: 2 }, { Unique: "a" }];
   for( var i = 0; i < keyArr.length; i++ )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         cl.createIndex( indexName, { a: 1 }, keyArr[i] );
      } );
   }

   // 索引name正常值
   var indexName = "index_24016";
   cl.createIndex( indexName, indexDef );
   checkIndex( cl, indexName, indexDef );
   cl.dropIndex( indexName );

   // 索引name长度为1
   indexName = "a";
   cl.createIndex( indexName, indexDef );
   checkIndex( cl, indexName, indexDef );
   cl.dropIndex( indexName );

   // 索引name长度为1023
   for( var i = 0; i < 1021; i++ )
   {
      indexName = indexName + "a";
   }
   cl.createIndex( indexName, indexDef );
   checkIndex( cl, indexName, indexDef );
   cl.dropIndex( indexName );

   // 索引指定合法参数
   indexName = "index_24016";
   var option = { Unique: true, Enforced: true, NotNull: true };
   cl.createIndex( indexName, indexDef, option );
   checkIndex( cl, indexName, indexDef, option );
   cl.dropIndex( indexName );

   // 索引指定SortBufferSize
   var option = { SortBufferSize: 100 };
   cl.createIndex( indexName, indexDef, option );
   cl.dropIndex( indexName );

   // 指定非法空name
   indexName = "";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.createIndex( indexName, indexDef );
   } );

   // 指定已$开头包含.的非法name
   indexName = "$a.b";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.createIndex( indexName, indexDef );
   } );
}

function checkIndex ( cl, indexName, indexDef, option )
{
   if( option == undefined )
   {
      option = { Unique: false, Enforced: false, NotNull: false, NotArray: false };
   }
   if( option.Unique == undefined ) { option.Unique = false; }
   if( option.Enforced == undefined ) { option.Enforced = false; }
   if( option.NotNull == undefined ) { option.NotNull = false; }
   if( option.NotArray == undefined ) { option.NotArray = false; }

   var idx = cl.getIndex( indexName ).toObj();
   assert.equal( idx.IndexDef.key, indexDef );
   assert.equal( idx.IndexDef.unique, option.Unique );
   assert.equal( idx.IndexDef.enforced, option.Enforced );
   assert.equal( idx.IndexDef.NotNull, option.NotNull );
   assert.equal( idx.IndexDef.NotArray, option.NotArray );
}

function checkRecords ( cl, expRecs ) 
{
   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { b: 1 } );
   var actRecs = new Array();
   while( tmpRecs = rc.next() )
   {
      actRecs.push( tmpRecs.toObj() );
   }
   assert.equal( expRecs, actRecs );
}