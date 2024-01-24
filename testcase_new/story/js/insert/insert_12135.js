/******************************************************************************
 * @Description   : seqDB-12135:单条记录中包含多个字段
 * @Author        : Wu Yan
 * @CreateTime    : 2017.07.11
 * @LastEditTime  : 2021.07.02
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_12135";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   var veryBigJsonString = "{'_id':1, \"a0\":0";
   var i = 0;
   for( i = 0; i < 1000; i++ )
   {
      veryBigJsonString += ",\"a" + i + "\":" + i;
   }
   veryBigJsonString += "}";
   var veryBigJson = eval( '(' + veryBigJsonString + ')' );

   varCL.insert( veryBigJson );
   var actual = varCL.find().count();
   var expected = 1;
   assert.equal( actual, expected );

   commCompareObject( veryBigJson, varCL.find().next().toObj() );
}
