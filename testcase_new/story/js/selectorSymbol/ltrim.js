/************************************
*@Description: use ltrim:{field:<$ltrim:Value>}, 
               1.one or two fields,with blank spacs or tab,seqDB-5712/seqDB-5713/seqDB-5714/seqDB-5715/seqDB-5716/seqDB-8229
               2.exist or Non-exist,seqDB-5720
               3.with blank space or tab,seqDB-5717
               4.the string is null,seqDB-5718
               5.Non-string,seqDB-5719
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

   /*1.one or two fields,with blank spacs or tab,seqDB-5712/seqDB-5713/seqDB-5714/seqDB-5715/seqDB-5716/seqDB-8229
     2.exist or Non-exist,seqDB-5720
     3.with blank space or tab,seqDB-5717
     4.the string is null,seqDB-5718
     5.Non-string,seqDB-5719*/
   var selectCondition1 = { name: { $ltrim: 1 }, major: { $ltrim: 1 }, class: { $ltrim: 1 } };
   var expRecs1 = [{ No: 1, name: "Zha Ng\tsa\nn ", major: "Zh a\tNg\nsan\t" },
   { No: 2, name: "", major: "" },
   { No: 3, name: "W an\tgw\nu\n", major: "Wangwu\t\n " },
   { No: 4, name: "W an\tgw\nu\r", major: "Wangwu\t\n\r " },
   { No: 5, name: null, major: "" },
   { No: 6, name: ["String\n\t "] }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { No: 1 } );

   /*3.placeholder set Non-1*/
   selectCondition3 = { name: { $ltrim: 0 } };
   InvalidArgCheck( dbcl, null, selectCondition3, -6 );
}
