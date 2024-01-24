/************************************
*@Description: insert all data type
*@author:      zhaoyu
*@createdate:  2018.10.11
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_11999";
   var clFullName = COMMCSNAME + "." + clName
   var indexName = "a_11999";

   dropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var doc = [{ No: 1, a: 123 },
   { No: 2, a: { $numberLong: "9223372036854775807" } },
   { No: 3, a: 1.23 },
   { No: 4, a: { $decimal: "123" } },
   { No: 5, a: "string" },
   { No: 6, a: { $oid: "123abcd00ef12358902300ef" } },
   { No: 7, a: false },
   { No: 8, a: { $date: "2012-01-01" } },
   { No: 9, a: { $timestamp: "2012-01-01-13.14.26.124233" } },
   { No: 10, a: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } },
   { No: 11, a: { $regex: "a", $options: "i" } },
   { No: 12, a: { b: 1 } },
   { No: 13, a: ["arr1", "arr2"] },
   { No: 14, a: null },
   { No: 15, a: { $maxKey: 1 } },
   { No: 16, a: { $minKey: 1 } },
   { No: 17, a: "中文" }];
   dbcl.insert( doc );
   commCreateIndex( dbcl, indexName, { a: "text" } );
   dbcl.insert( doc );

   //all of record sync to ES
   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 6 );

   var expectRecords = dbOperator.findFromCL( dbcl, { $or: [{ a: { $type: 2, $et: "string" } }, { a: { $type: 2, $et: "array" } }] }, null, { _id: 1 } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, null, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   //string update to int,sync ES
   dbcl.update( { $set: { a: 1 } }, { a: { $type: 2, $et: "string" } } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 2 );
   var expectRecords = dbOperator.findFromCL( dbcl, { $or: [{ a: { $type: 2, $et: "string" } }, { a: { $type: 2, $et: "array" } }] } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
   checkResult( expectRecords, actRecords );

   //arr update to int,sync ES
   dbcl.update( { $set: { a: 2 } }, { a: { $type: 2, $et: "array" } } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 0 );
   var expectRecords = dbOperator.findFromCL( dbcl, { $or: [{ a: { $type: 2, $et: "string" } }, { a: { $type: 2, $et: "array" } }] } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
   checkResult( expectRecords, actRecords );

   //int update to string,sync ES
   dbcl.update( { $set: { a: "update" } }, { a: 1 } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 4 );
   var expectRecords = dbOperator.findFromCL( dbcl, { $or: [{ a: { $type: 2, $et: "string" } }, { a: { $type: 2, $et: "array" } }] } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
   checkResult( expectRecords, actRecords );

   //int update to array,sync ES
   /*dbcl.update({$set:{a:["arr1", "arr2"]}},{a:2});
   checkFullSyncToES(COMMCSNAME, clName, indexName, 6);
   var expectRecords = dbOperator.findFromCL(dbcl, {$or:[{a:{$type:2,$et:"string"}},{a:{$type:2,$et:"array"}}]});
   var actRecords = dbOperator.findFromCL(dbcl, {"":{"$Text":{query:{match_all:{}}}}});
   checkResult(expectRecords, actRecords);
   */

   //all datatype to string,sync ES
   dbcl.update( { $set: { a: "update" } }, { a: { $type: 2, $ne: "array" } } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 34 );
   var expectRecords = dbOperator.findFromCL( dbcl, { $or: [{ a: { $type: 2, $et: "string" } }, { a: { $type: 2, $et: "array" } }] } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
   checkResult( expectRecords, actRecords );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, indexName );
   dropCL( db, COMMCSNAME, clName );
   //SEQUOIADBMAINSTREAM-3983 
   checkIndexNotExistInES( esIndexNames );
}
