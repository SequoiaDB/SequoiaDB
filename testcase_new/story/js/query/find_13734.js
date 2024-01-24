/*******************************************************************************
*@Description : seqDB-13734:query.count/query.size查询，组合skip/limit
*@Modify List : 2020-08-12   wuyan   Modify
*******************************************************************************/
testConf.clName = COMMCLNAME + "_query_cl_13734";
main( test );

function test ( testPara )
{
   var recsNum = 1000;
   var rd = new commDataGenerator();
   var recs = rd.getRecords( recsNum, "int,float,string", ['a', 'b', 'c'] );
   testPara.testCL.insert( recs );

   //query.limit.count,query.skip.count
   var count1 = testPara.testCL.find().limit( 10 ).count();
   var count2 = testPara.testCL.find().skip( recsNum - 1 ).count();
   if( Number( count1 ) !== recsNum || Number( count2 ) !== recsNum )
   {
      throw new Error( "query count failed! count1= " + count1 + "\n count2= " + count2 );
   }

   //query.limit.size,query.skip.size
   var size1 = testPara.testCL.find().limit( 10 ).size();
   assert.equal( size1, 10 );
   var size2 = testPara.testCL.find().skip( recsNum - 1 ).size();
   assert.equal( size2, 1 );
}
