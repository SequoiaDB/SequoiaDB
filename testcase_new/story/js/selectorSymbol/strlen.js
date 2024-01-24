/************************************
*@Description: use strlen:{field:<$strlen:Value>}, 
               1.one or two fields,seqDB-5690/seqDB-8226
               2.exist or Non-exist,seqDB-5696
               3.with plank space or tab,seqDB-5692
               4.the string is null,seqDB-5691/seqDB-5694
               5.the string has blank space only
               6.Non-string,seqDB-5695
*@author:      zhaoyu
*@createdate:  2016.7.15
*@testlinkCase:
**************************************/
main( test );

function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ No: 1, name: "zhangsan", major: "math" },
   { No: 2, name: "", major: "chinese" },
   { No: 3, name: " wa ng wu ", major: "english" },
   { No: 4, name: 123, major: " " },
   { No: 5, name: [" \n\t\rString\n\t "] }];
   dbcl.insert( doc );

   /*1.one or two fields,seqDB-5690/seqDB-8226
     2.exist or Non-exist,seqDB-5696
     3.with plank space or tab,seqDB-5692
     4.the string is null,seqDB-5691/seqDB-5694
     5.the string has blank space only
     6.Non-string,seqDB-5695*/
   var selectCondition1 = { name: { $strlen: 1 }, major: { $strlen: 1 }, class: { $strlen: 1 } };
   var expRecs1 = [{ No: 1, name: 8, major: 4 },
   { No: 2, name: 0, major: 7 },
   { No: 3, name: 10, major: 7 },
   { No: 4, name: null, major: 1 },
   { No: 5, name: [13] }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { No: 1 } );

   /*3.placeholder set Non-1*/
   selectCondition3 = { name: { $strlen: 0 } };
   InvalidArgCheck( dbcl, null, selectCondition3, -6 );
}
