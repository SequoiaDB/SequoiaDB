/****************************************************
@description: seqDB-5286:limit、skip，参数边界值校验
@author:
              2019-6-3 wuyan init
****************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_query5286";
   var cl = readyCL( clName );

   var recordNum = 10000;
   insertRecs( cl, recordNum );
   queryRecsAndCheckResult( cl, recordNum );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl, recordNum )
{

   var docs = [];
   for( i = 0; i < recordNum; i++ )
   {
      var objs = { "no": i, "str": "test" + i };
      docs.push( objs );
   }
   cl.insert( docs );
}

function queryRecsAndCheckResult ( cl, recordNum )
{
   var query1 = cl.find().limit( 0 ).toArray();
   var expReturnEmpty = 0;
   assert.equal( query1.length, expReturnEmpty );

   var query2 = cl.find().limit( 1 ).skip( recordNum ).toArray();
   assert.equal( query2.length, expReturnEmpty );

   var query3 = cl.find().limit( recordNum ).skip( 0 ).toArray();
   assert.equal( query3.length, recordNum );

   var query4 = cl.find().skip( recordNum ).toArray();
   assert.equal( query4.length, expReturnEmpty );

}