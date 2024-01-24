/************************************
*@Description: find操作，匹配符$gte，匹配已存在的数组
*@author:      wangkexin
*@createdate:  2019.3.5
*@testlinkCase: 
**************************************/
main( test );
function test ()
{
   var csName = COMMCSNAME;
   var clName = "cl11071";

   var cl = commCreateCL( db, csName, clName );

   //insert data 
   cl.insert( { a: [1, 2, 3] } );

   var findCondition = { a: { $gte: [1, 2, 3] } };
   var expRecs = [{ "a": [1, 2, 3] }];
   checkResult( cl, findCondition, null, expRecs, { _id: 1 } );

   commDropCL( db, csName, clName, true, true, "drop CL in the end" );
}
