/************************************
*@Description:capped cl find use sort/limit/skip 
*@author:      zhaoyu
*@createdate:  2017.7.11
*@testlinkCase: seqDB-11802/seqDB-11803/seqDB-11804
**************************************/
main( test );
function test ()
{
   var clName = COMMCAPPEDCLNAME + "_11802_11803_11804";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, false, true );

   insertDatas( dbcl );

   var sortConf1 = { a: 1, b: -1, c: 1 };
   var limitConf1 = 1;
   var expRecs1 = [{ a: 0, b: 2, c: 0 }];
   checkRecords( dbcl, null, null, sortConf1, limitConf1, null, expRecs1 );

   var sortConf2 = { a: -1, b: 1, c: -1 };
   var skipConf2 = 26;
   var expRecs2 = [{ a: 0, b: 2, c: 0 }];
   checkRecords( dbcl, null, null, sortConf2, null, skipConf2, expRecs2 );

   var sortConf3 = { a: 1, b: -1, c: -1 };
   var limitConf3 = 1;
   var skipConf3 = 3;
   var expRecs3 = [{ a: 0, b: 1, c: 2 }];
   checkRecords( dbcl, null, null, sortConf3, limitConf3, skipConf3, expRecs3 );

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}

function insertDatas ( dbcl )
{
   for( var k = 0; k < 3; k++ )
   {
      for( var i = 0; i < 3; i++ )
      {
         var doc = [];
         for( var j = 0; j < 3; j++ )
         {
            doc.push( { a: k, b: i, c: j } );
         }
         dbcl.insert( doc );
      }
   }
}