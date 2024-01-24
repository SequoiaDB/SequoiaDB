/******************************************************************************
 * @Description   : seqDB-13655:插入记录失败后，重新执行查询操作从上一次的数据节点查询
 * @Author        : Wang Kexin
 * @CreateTime    : 2019.03.12
 * @LastEditTime  : 2021.07.02
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_13655";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   var insert_string = { _id: 1, field: 123, name: "赵钱孙李陈" };
   varCL.insert( insert_string );
   varCL.createIndex( "myIndex13655", { field: 1, name: 1 } );

   var robj = varCL.find().next().toObj();
   commCompareObject( insert_string, robj );

   var exp = varCL.find().explain().toArray();
   var obj = eval( '(' + exp + ')' );
   var NodeName = obj['NodeName'];


   // 插入部分失败记录，返回错误码覆盖:-6,-24,-37,-38,-39,-108
   var errInsertString1 = { _id: [1, 2, 3] };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      varCL.insert( errInsertString1 );
   } )

   var errInsertString2 = { a: repeat( "a", 16 * 1024 * 1024 ) };
   assert.tryThrow( SDB_DMS_RECORD_TOO_BIG, function()
   {
      varCL.insert( errInsertString2 );
   } )

   var errInsertString3 = { field: [1, 2, 3], name: [4, 5, 6] };
   assert.tryThrow( SDB_IXM_MULTIPLE_ARRAY, function()
   {
      varCL.insert( errInsertString3 );
   } )

   var errInsertString4 = { _id: 1 };
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      varCL.insert( errInsertString4 );
   } )

   var errInsertString5 = { field: repeat( "field", 1024 ) };
   assert.tryThrow( SDB_IXM_KEY_TOO_LARGE, function()
   {
      varCL.insert( errInsertString5 );
   } )
   getNodeName( varCL, NodeName );

   var robj = varCL.find().next().toObj();
   commCompareObject( insert_string, robj );

}

function getNodeName ( varCL, NodeName )
{
   var exp = varCL.find().explain().toArray();
   var obj = eval( '(' + exp + ')' );
   var actNodeName = obj['NodeName'];
   assert.equal( NodeName, actNodeName, SDB_CLS_COORD_NODE_CAT_VER_OLD );
}

function repeat ( str, n )
{
   return new Array( n + 1 ).join( str );
}
