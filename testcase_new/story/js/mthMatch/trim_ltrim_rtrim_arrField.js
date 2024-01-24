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
   var doc = [{ No: 1, name: [" \t\r\nZha Ng\ts\ra\nn \t\r\n"], major: [" \t\r\nCh \t\n\rinese \t\n\r"] },
   { No: 2, name: [" \t\r\nZha Ng\ts\ra\nn1 \t\r\n"], major: [" \t\r\nCh \t\n\rinese1 \t\n\r"] },
   { No: 3, name: [{ 0: " \t\r\nZha Ng\ts\ra\nn \t\r\n" }], major: [{ 0: " \t\r\nCh \t\n\rinese \t\n\r" }] },
   { No: 4, name: [{ 0: " \t\r\nZha Ng\ts\ra\nn1 \t\r\n" }], major: [{ 0: " \t\r\nCh \t\n\rinese1 \t\n\r" }] },
   { No: 5, name: { 0: " \t\r\nZha Ng\ts\ra\nn \t\r\n" }, major: { 0: " \t\r\nCh \t\n\rinese \t\n\r" } },
   { No: 6, name: { 0: " \t\r\nZha Ng\ts\ra\nn1 \t\r\n" }, major: { 0: " \t\r\nCh \t\n\rinese1 \t\n\r" } }];
   dbcl.insert( doc );

   var condition1 = { name: { $trim: 1, $et: "Zha Ng\ts\ra\nn" }, major: { $trim: 1, $et: "Ch \t\n\rinese" } };
   var expRecs1 = [{ No: 1, name: [" \t\r\nZha Ng\ts\ra\nn \t\r\n"], major: [" \t\r\nCh \t\n\rinese \t\n\r"] }];
   checkResult( dbcl, condition1, null, expRecs1, { No: 1 } );

   var condition2 = { "name.0": { $trim: 1, $et: "Zha Ng\ts\ra\nn" }, "major.0": { $trim: 1, $et: "Ch \t\n\rinese" } };
   var expRecs2 = [{ No: 1, name: [" \t\r\nZha Ng\ts\ra\nn \t\r\n"], major: [" \t\r\nCh \t\n\rinese \t\n\r"] },
   { No: 3, name: [{ 0: " \t\r\nZha Ng\ts\ra\nn \t\r\n" }], major: [{ 0: " \t\r\nCh \t\n\rinese \t\n\r" }] },
   { No: 5, name: { 0: " \t\r\nZha Ng\ts\ra\nn \t\r\n" }, major: { 0: " \t\r\nCh \t\n\rinese \t\n\r" } }];
   checkResult( dbcl, condition2, null, expRecs2, { No: 1 } );

   var condition3 = { name: { $rtrim: 1, $et: " \t\r\nZha Ng\ts\ra\nn" }, major: { $rtrim: 1, $et: " \t\r\nCh \t\n\rinese" } };
   var expRecs3 = [{ No: 1, name: [" \t\r\nZha Ng\ts\ra\nn \t\r\n"], major: [" \t\r\nCh \t\n\rinese \t\n\r"] }];
   checkResult( dbcl, condition3, null, expRecs3, { No: 1 } );

   var condition4 = { "name.0": { $rtrim: 1, $et: " \t\r\nZha Ng\ts\ra\nn" }, "major.0": { $rtrim: 1, $et: " \t\r\nCh \t\n\rinese" } };
   var expRecs4 = [{ No: 1, name: [" \t\r\nZha Ng\ts\ra\nn \t\r\n"], major: [" \t\r\nCh \t\n\rinese \t\n\r"] },
   { No: 3, name: [{ 0: " \t\r\nZha Ng\ts\ra\nn \t\r\n" }], major: [{ 0: " \t\r\nCh \t\n\rinese \t\n\r" }] },
   { No: 5, name: { 0: " \t\r\nZha Ng\ts\ra\nn \t\r\n" }, major: { 0: " \t\r\nCh \t\n\rinese \t\n\r" } }];
   checkResult( dbcl, condition4, null, expRecs4, { No: 1 } );

   var condition5 = { name: { $ltrim: 1, $et: "Zha Ng\ts\ra\nn \t\r\n" }, major: { $ltrim: 1, $et: "Ch \t\n\rinese \t\n\r" } };
   var expRecs5 = [{ No: 1, name: [" \t\r\nZha Ng\ts\ra\nn \t\r\n"], major: [" \t\r\nCh \t\n\rinese \t\n\r"] }];
   checkResult( dbcl, condition5, null, expRecs5, { No: 1 } );

   var condition6 = { "name.0": { $ltrim: 1, $et: "Zha Ng\ts\ra\nn \t\r\n" }, "major.0": { $ltrim: 1, $et: "Ch \t\n\rinese \t\n\r" } };
   var expRecs6 = [{ No: 1, name: [" \t\r\nZha Ng\ts\ra\nn \t\r\n"], major: [" \t\r\nCh \t\n\rinese \t\n\r"] },
   { No: 3, name: [{ 0: " \t\r\nZha Ng\ts\ra\nn \t\r\n" }], major: [{ 0: " \t\r\nCh \t\n\rinese \t\n\r" }] },
   { No: 5, name: { 0: " \t\r\nZha Ng\ts\ra\nn \t\r\n" }, major: { 0: " \t\r\nCh \t\n\rinese \t\n\r" } }];
   checkResult( dbcl, condition6, null, expRecs6, { No: 1 } );
}
