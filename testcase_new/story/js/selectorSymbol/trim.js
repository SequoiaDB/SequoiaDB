/************************************
*@Description: use trim:{field:<$trim:Value>}, 
               1.one or two fields,with blank spacs or tab,seqDB-5732/seqDB-5733/seqDB-5734/seqDB-5735/seqDB-5736/seqDB-8231
               2.exist or Non-exist,seqDB-5740
               3.with blank space or tab,seqDB-5737
               4.the string is null,seqDB-5738
               5.Non-string,seqDB-5739
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
   var doc = [{ No: 1, name: " Zha Ng\tsa\nn ", major: "\tZh a\tNg\nsan\t" },
   { No: 2, name: " \n", major: "\t" },
   { No: 3, name: "\nW an\tgw\nu\n", major: " \t\nWangwu\t\n " },
   { No: 4, name: "\rW an\tgw\nu\r", major: " \t\n\rWangwu\t\n\r " },
   { No: 5, name: 123, major: "" },
   { No: 6, name: [" \n\t\rString\n\t "] }];
   dbcl.insert( doc );

   /*1.one or two fields,with blank spacs or tab,seqDB-5732/seqDB-5733/seqDB-5734/seqDB-5735/seqDB-5736/seqDB-8231
     2.exist or Non-exist,seqDB-5740
     3.with blank space or tab,seqDB-5737
     4.the string is null,seqDB-5738
     5.Non-string,seqDB-5739*/
   var selectCondition1 = { name: { $trim: 1 }, major: { $trim: 1 }, class: { $trim: 1 } };
   var expRecs1 = [{ No: 1, name: "Zha Ng\tsa\nn", major: "Zh a\tNg\nsan" },
   { No: 2, name: "", major: "" },
   { No: 3, name: "W an\tgw\nu", major: "Wangwu" },
   { No: 4, name: "W an\tgw\nu", major: "Wangwu" },
   { No: 5, name: null, major: "" },
   { No: 6, name: ["String"] }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { No: 1 } );

   /*3.placeholder set Non-1*/
   selectCondition3 = { name: { $trim: 0 } };
   InvalidArgCheck( dbcl, null, selectCondition3, -6 );
}
