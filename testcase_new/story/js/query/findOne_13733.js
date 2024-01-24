/*******************************************************************************
*@Description : seqDB-13733:findOne简单查询
*@Modify List : 2014-9-26   xiaojunHu  Init
                2016-3-17   Ting YU    modify
                2020-08-12  wuyan      modify
*******************************************************************************/
testConf.clName = COMMCLNAME + "_query_cl_13733";
main( test );

function test ( testPara )
{
   var recs = [{ a: 1 }, { a: 1 }, { a: 2 }];
   testPara.testCL.insert( recs );

   var rc = testPara.testCL.findOne();
   var cnt = rc.toArray().length;
   assert.equal( cnt, 1 );
   rc.close();

   var rc = testPara.testCL.findOne( { a: { $lte: 1 } } );
   var cnt = 0;
   while( rc.next() )
   {
      cnt++;
      var val = rc.current().toObj().a;
      assert.equal( val, 1 );
   }
   rc.close();
   assert.equal( cnt, 1 );
}
