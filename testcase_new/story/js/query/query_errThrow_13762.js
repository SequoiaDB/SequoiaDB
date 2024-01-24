/*******************************************************************************
*@Description : [seqDB-13762]when query like: db.foo.bar.find({$a:1}), we should
*               throw error -6 and print string: Invalid Argument
*@Modify List :
*               2014-9-26   xiaojunHu  Init
                2020-08-13 Zixian Yan  Modify
*******************************************************************************/
testConf.clName = COMMCLNAME + "_13762";
//SEQUOIADBMAINSTREAM-1200
//main( test );

function test ( testPara )
{
   var cl = testPara.testCL;

   // insert record
   cl.insert( { a: 1 } );
   cl.insert( { b: "testcase" } );
   // query by use db.cs.cl.find({$a:1}).getLastErrMsg() will get the message
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { $a: 1 } )
   } );

}
