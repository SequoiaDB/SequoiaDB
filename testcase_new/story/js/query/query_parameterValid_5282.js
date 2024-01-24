/****************************************************
@description: seqDB-5282:query[i]，i参数边界值校验
@author:
              2019-5-30 wuyan init
****************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_query5282";
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

   var query = cl.find();
   //访问第一条记录，下标值取0;访问最后一条记录取记录数，下标志为9999
   var serialFirst = 0;
   var serialLast = recordNum - 1;
   var queryFirst = query[serialFirst];
   var queryLast = query[serialLast];

   //查询第一条记录和最后一条记录
   var expQueryFirst = cl.find( { "no": serialFirst } ).toArray().toString();
   var expQueryLast = cl.find( { "no": serialLast } ).toArray().toString();

   if( queryFirst !== expQueryFirst || queryLast !== expQueryLast )
   {
      throw new Error( "checkResult fail,[compare the records]" +
         "[expQueryFirst:" + expQueryFirst + ", queryLast:" + expQueryLast + "]" +
         "[expQueryLast:" + expQueryLast + ", queryLast:" + queryLast + "]" );
   }

}