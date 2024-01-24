/******************************************************************************
 * @Description   : seqDB-12134:指定_id插入数据
 * @Author        : Wu Yan
 * @CreateTime    : 2017.07.11
 * @LastEditTime  : 2021.08.05
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_12134";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   var docs = [];
   docs.push( { a: 1, _id: { a: 1 } } );
   docs.push( { a: 2, _id: true } );
   docs.push( { a: 3, _id: "13000a" } );
   docs.push( { a: 4, _id: 1 } );
   docs.push( { a: 5, _id: { $numberLong: "9223372036854775807" } } );
   docs.push( { a: 6, _id: 12.123 } );
   docs.push( { a: 7, _id: { $decimal: "111111111111111111111111111111111111111111111111111111111111111.111111111111111111111111" } } );
   docs.push( { a: 8, _id: { $date: "2012-01-01" } } );
   docs.push( { a: 9, _id: { $timestamp: "2012-01-01-13.14.26.124233" } } );
   docs.push( { a: 10, _id: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } } );
   docs.push( { a: 11, _id: null } );
   docs.push( { a: 12, _id: { $minKey: 1 } } );
   docs.push( { a: 13, _id: { $maxKey: 1 } } );
   varCL.insert( docs );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      varCL.insert( { _id: { $regex: "^a", $options: "i" } } );
   } )
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      varCL.insert( { _id: { $regex: "^a" } } );
   } )
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      varCL.insert( { _id: [1, 2] } );
   } )
   var cursor = varCL.find();
   commCompareResults( cursor, docs, false );
}
