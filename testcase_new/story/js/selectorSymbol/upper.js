/************************************
*@Description: use upper:{field:<$upper:Value>}, 
               1.one or two fields,seqDB-5705/seqDB-8228
               2.exist or Non-exist,seqDB-5710
               3.with plank space or tab,seqDB-5707
               4.the string is null,seqDB-5708
               5.the string with Non-letter,seqDB-5706
               6.Non-string,seqDB-5709
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
   { No: 3, name: " Wa ng wu ", major: "abcdefghijklmnopqrstuvwxyz" },
   { No: 4, name: 123, major: " " },
   { No: 5, name: [" \n\t\rString\n\t "] }];
   dbcl.insert( doc );

   /*1.one or two fields,seqDB-5705/seqDB-8228
     2.exist or Non-exist,seqDB-5710
     3.with plank space or tab,seqDB-5707
     4.the string is null,seqDB-5708
     5.the string with Non-letter,seqDB-5706
     6.Non-string,seqDB-5709*/
   var selectCondition1 = { name: { $upper: 1 }, major: { $upper: 1 }, class: { $upper: 1 } };
   var expRecs1 = [{ No: 1, name: "ZHANGSAN", major: "123" },
   { No: 2, name: "", major: "CHINESE" },
   { No: 3, name: " WA NG WU ", major: "ABCDEFGHIJKLMNOPQRSTUVWXYZ" },
   { No: 4, name: null, major: " " },
   { No: 5, name: [" \n\t\rSTRING\n\t "] }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { No: 1 } );

   /*3.placeholder set Non-1*/
   selectCondition3 = { name: { $upper: 0 } };
   InvalidArgCheck( dbcl, null, selectCondition3, -6 );
}
