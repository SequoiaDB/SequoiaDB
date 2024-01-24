/******************************************************************************
 * @Description   : seqDB-23460 :: $elemMatch作为匹配符，条件同时包匹配符和普通字段 
 *                  seqDB-23461 :: $elemMatch/$elemMatchOne作为选择符，条件同时包匹配符和普通字段 
 *                  seqDB-23462 :: $elemMatch作为匹配符，条件为空数组、空字符串、null 
 *                  seqDB-23463 :: $elemMatch/$elemMatchOne作为选择符，条件为空数组、空字符串、null 
 *                  seqDB-23464 :: $elemMatch或 $elemMatchOne条件中以$开头且非操作符 
 * @Author        : Yu Fan
 * @CreateTime    : 2021.01.20
 * @LastEditTime  : 2021.01.20
 * @LastEditors   : Yu Fan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_23460";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;

   // $elemMatch作为匹配符，条件同时包匹配符和普通字段 
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { b: { $elemMatch: { $gte: 1, a: 1 } } } ).toArray()
   } );

   // $elemMatch/$elemMatchOne作为选择符，条件同时包匹配符和普通字段 
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { b: { $elemMatch: { $gte: 1, a: 1 } } } ).toArray()
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { b: { $elemMatchOne: { $gte: 1, a: 1 } } } ).toArray()
   } );

   // $elemMatch作为匹配符，条件为空数组、空字符串、null 
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { b: { $elemMatch: [] } } ).toArray()
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { b: { $elemMatch: "" } } ).toArray()
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { b: { $elemMatch: null } } ).toArray()
   } );

   //  $elemMatch/$elemMatchOne作为选择符，条件为空数组、空字符串、null 
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { b: { $elemMatch: [] } } ).toArray()
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { b: { $elemMatch: "" } } ).toArray()
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { b: { $elemMatch: null } } ).toArray()
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { b: { $elemMatchOne: [] } } ).toArray()
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { b: { $elemMatchOne: "" } } ).toArray()
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { b: { $elemMatchOne: null } } ).toArray()
   } );

   // $elemMatch或 $elemMatchOne条件中以$开头且非操作符 
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { b: { $elemMatch: { $k: 1 } } } ).toArray()
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { b: { $elemMatch: { $default: 1 } } } ).toArray()
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { b: { $elemMatchOne: { $k: 1 } } } ).toArray()
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { b: { $elemMatchOne: { $default: 1 } } } ).toArray()
   } );
}

