/*******************************************************************************
*@Description : seqDB-13755:sort指定字段名为空
*@Modify list :
*               2015-2-10  xiaojun Hu  Init  
*               2020-10-12 huangxiaoni  Modify
*******************************************************************************/
testConf.clName = COMMCLNAME + "_13755";
testConf.useSrcGroup = true;

main( test );
function test ( arg )
{
   var cl = arg.testCL;
   var insertNum = 100;
   idxAutoGenData( cl, insertNum );

   // 排序字段名为空字符串
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find().sort( { "": 1 } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find().sort( { "": "a" } ).toArray();
   } );

   // 排序字段名有效，值无效
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find().sort( { "a": 0 } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find().sort( { "a": "1" } ).toArray();
   } );
}

function idxAutoGenData ( cl, insertNum )
{
   if( undefined == insertNum ) { insertNum = 1000; }
   var record = [];
   for( var i = 0; i < insertNum; ++i )
   {
      record.push( {
         "no": i, "no1": i * 2, "no2": i * 3,
         "obj_id": { "$oid": "123abcd00ef12358902300ef" },
         "subobj": { "obj": { "val": "sub" } },
         "string": "西边个喇嘛，东边个哑巴",
         "array": [i + "arr" + i, 5 * i, 2 * i + "ARR" + i, "arrayIndex"], "no3": 4 * i
      } );
   }
   cl.insert( record );
   cnt = 0;
   while( insertNum != cl.count() && cnt < 1000 )
   {
      ++cnt;
      sleep( 2 );
   }
   assert.equal( insertNum, cl.count() );
}