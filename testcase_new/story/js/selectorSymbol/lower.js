/************************************
*@Description: use lower:{field:<$lower:Value>}, 
               1.one or two fields,seqDB-5698/seqDB-8227
               2.exist or Non-exist,seqDB-5703
               3.with plank space or tab,seqDB-5700
               4.the string is null,seqDB-5701
               5.the string with Non-letter,seqDB-5699
               6.Non-string,seqDB-5702
*@author:      zhaoyu
*@createdate:  2016.7.15
*@testlinkCase:
***************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ No: 1, name: "ZhaNgsan", major: "123" },
   { No: 2, name: "", major: "CHINESE" },
   { No: 3, name: " Wa ng wu ", major: "ABCDEFGHIJKLMNOPQRSTUVWXYZ" },
   { No: 4, name: 123, major: " " },
   { No: 5, name: [" \n\t\rString\n\t "] }];
   dbcl.insert( doc );

   /*1.one or two fields,seqDB-5698/seqDB-8227
     2.exist or Non-exist,seqDB-5703
     3.with plank space or tab,seqDB-5700
     4.the string is null,seqDB-5701
     5.the string with Non-letter,seqDB-5699
     6.Non-string,seqDB-5702*/
   var selectCondition1 = { name: { $lower: 1 }, major: { $lower: 1 }, class: { $lower: 1 } };
   var expRecs1 = [{ No: 1, name: "zhangsan", major: "123" },
   { No: 2, name: "", major: "chinese" },
   { No: 3, name: " wa ng wu ", major: "abcdefghijklmnopqrstuvwxyz" },
   { No: 4, name: null, major: " " },
   { No: 5, name: [" \n\t\rstring\n\t "] }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { No: 1 } );

   /*3.placeholder set Non-1*/
   selectCondition3 = { name: { $lower: 0 } };
   InvalidArgCheck( dbcl, null, selectCondition3, -6 );
}
