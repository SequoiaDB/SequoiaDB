/************************************
*@Description: seqDB-22231:插入错误$date参数报错_insert({a:{$date:null}})
*@Author      : 2020.06.01:chimanzhao
**************************************/
testConf.clName = COMMCLNAME + "_22231";
main( test );

function test ( testPara )
{
   var dbcl = testPara.testCL;
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.insert( { a: { $date: null } } )
   } );
}