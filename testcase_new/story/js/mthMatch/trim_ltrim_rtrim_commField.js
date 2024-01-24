/************************************
*@Description: use trim/ltrim/rtrim
*@author:      zhaoyu
*@createdate:  2016.10.14
*@testlinkCase:seqDB-1029/seqDB-10298/seqDB-10299/seqDB-10300
*              seqDB-10301/seqDB-10302/seqDB-10303/seqDB-10304
*              seqDB-10305/seqDB-10306/seqDB-10307/seqDB-10308
***************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ No: 1, name: " \t\r\nZha Ng\ts\ra\nn \t\r\n", major: " \t\r\nCh \t\n\rinese \t\n\r", weight: 51 },
   { No: 2, name: " \t\r\nZha Ng\ts\ra\nn1 \t\r\n", major: " \t\r\nCh \t\n\rinese1 \t\n\r", weight: 51 }];
   dbcl.insert( doc );

   var condition1 = { name: { $trim: 1, $et: "Zha Ng\ts\ra\nn" }, major: { $trim: 1, $et: "Ch \t\n\rinese" } };
   var expRecs1 = [{ No: 1, name: " \t\r\nZha Ng\ts\ra\nn \t\r\n", major: " \t\r\nCh \t\n\rinese \t\n\r", weight: 51 }];
   checkResult( dbcl, condition1, null, expRecs1, { No: 1 } );

   var condition2 = { name: { $ltrim: 1, $et: "Zha Ng\ts\ra\nn \t\r\n" }, major: { $ltrim: 1, $et: "Ch \t\n\rinese \t\n\r" } };
   var expRecs2 = [{ No: 1, name: " \t\r\nZha Ng\ts\ra\nn \t\r\n", major: " \t\r\nCh \t\n\rinese \t\n\r", weight: 51 }];
   checkResult( dbcl, condition2, null, expRecs2, { No: 1 } );

   var condition3 = { name: { $rtrim: 1, $et: " \t\r\nZha Ng\ts\ra\nn" }, major: { $rtrim: 1, $et: " \t\r\nCh \t\n\rinese" } };
   var expRecs3 = [{ No: 1, name: " \t\r\nZha Ng\ts\ra\nn \t\r\n", major: " \t\r\nCh \t\n\rinese \t\n\r", weight: 51 }];
   checkResult( dbcl, condition3, null, expRecs3, { No: 1 } );

   var condition4 = { name: { $trim: 1, $et: "Zha Ng\ts\ra\nn" }, major: { $trim: 1, $et: "Ch \t\n\rinese" }, age: { $trim: 1, $et: "a" } };
   var expRecs4 = [];
   checkResult( dbcl, condition4, null, expRecs4, { No: 1 } );

   var condition5 = { name: { $ltrim: 1, $et: "Zha Ng\ts\ra\nn \t\r\n" }, major: { $ltrim: 1, $et: "Ch \t\n\rinese \t\n\r" }, age: { $ltrim: 1, $et: "a" } };
   var expRecs5 = [];
   checkResult( dbcl, condition5, null, expRecs5, { No: 1 } );

   var condition6 = { name: { $rtrim: 1, $et: " \t\r\nZha Ng\ts\ra\nn" }, major: { $rtrim: 1, $et: " \t\r\nCh \t\n\rinese" }, age: { $rtrim: 1, $et: "a" } };
   var expRecs6 = [];
   checkResult( dbcl, condition6, null, expRecs6, { No: 1 } );

   condition7 = { name: { $trim: "a", $et: 8 } };
   InvalidArgCheck( dbcl, condition7, null, SDB_INVALIDARG );

   condition8 = { name: { $trim: 1 } };
   InvalidArgCheck( dbcl, condition8, null, SDB_INVALIDARG );

   condition9 = { name: { $ltrim: "a", $et: 8 } };
   InvalidArgCheck( dbcl, condition9, null, SDB_INVALIDARG );

   condition10 = { name: { $ltrim: 1 } };
   InvalidArgCheck( dbcl, condition10, null, SDB_INVALIDARG );

   condition11 = { name: { $rtrim: "a", $et: 8 } };
   InvalidArgCheck( dbcl, condition11, null, SDB_INVALIDARG );

   condition12 = { name: { $rtrim: 1 } };
   InvalidArgCheck( dbcl, condition12, null, SDB_INVALIDARG );

   //Non-String
   var condition13 = { name: { $trim: 1, $et: "Zha Ng\ts\ra\nn" }, major: { $trim: 1, $et: "Ch \t\n\rinese" }, weight: { $trim: 1, $et: null } };
   var expRecs13 = [{ No: 1, name: " \t\r\nZha Ng\ts\ra\nn \t\r\n", major: " \t\r\nCh \t\n\rinese \t\n\r", weight: 51 }];
   checkResult( dbcl, condition13, null, expRecs13, { No: 1 } );

   var condition14 = { name: { $ltrim: 1, $et: "Zha Ng\ts\ra\nn \t\r\n" }, major: { $ltrim: 1, $et: "Ch \t\n\rinese \t\n\r" }, weight: { $trim: 1, $et: null } };
   var expRecs14 = [{ No: 1, name: " \t\r\nZha Ng\ts\ra\nn \t\r\n", major: " \t\r\nCh \t\n\rinese \t\n\r", weight: 51 }];
   checkResult( dbcl, condition14, null, expRecs14, { No: 1 } );

   var condition15 = { name: { $rtrim: 1, $et: " \t\r\nZha Ng\ts\ra\nn" }, major: { $rtrim: 1, $et: " \t\r\nCh \t\n\rinese" }, weight: { $trim: 1, $et: null } };
   var expRecs15 = [{ No: 1, name: " \t\r\nZha Ng\ts\ra\nn \t\r\n", major: " \t\r\nCh \t\n\rinese \t\n\r", weight: 51 }];
   checkResult( dbcl, condition15, null, expRecs15, { No: 1 } );
}
