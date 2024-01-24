/******************************************************************************
 * @Description   : seqDB-23122 : 使用数据源创建cl，执行CRUD操作
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/

testConf.skipStandAlone = true;
main( test );

function test ()
{
   var srcCSName = CHANGEDPREFIX + "_srcCS_23122";
   var srcCLName = "_srcCL_23122";
   var csName = CHANGEDPREFIX + "cS_23122";
   var clName = "cl_23122";
   var srcDataName = "srcData23122";

   commDropCS( datasrcDB, srcCSName );
   commDropCS( datasrcDB, csName );
   commDropCS( db, srcCSName );
   clearDataSource( csName, srcDataName );
   commCreateCS( datasrcDB, csName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, csName, clName );
   commCreateCL( datasrcDB, srcCSName, srcCLName );

   db.createDataSource( srcDataName, datasrcUrl, userName, passwd );
   var cs = db.createCS( csName );

   //test a、指定Mapping参数映射集合全名（同名cs/cl）   
   var dbcl = cs.createCL( clName, { DataSource: srcDataName, Mapping: csName + "." + clName } );
   crudAndCheckResult( dbcl );

   //test a、指定Mapping参数映射集合全名（不同名cs/cl）
   cs.dropCL( clName );
   var dbcl = cs.createCL( clName, { DataSource: srcDataName, Mapping: srcCSName + "." + srcCLName } );
   crudAndCheckResult( dbcl );

   //test b：指定Mapping参数映射集合短名,同名cl  
   cs.dropCL( clName );
   var dbcl = cs.createCL( clName, { DataSource: srcDataName, Mapping: clName } );
   crudAndCheckResult( dbcl );

   //test b：指定Mapping参数映射集合短名,不同名cl
   var name = "testsrc23122";
   var dbcl = cs.createCL( name, { DataSource: srcDataName, Mapping: clName } );
   crudAndCheckResult( dbcl );

   //test c：不指定Mapping映射  
   cs.dropCL( clName );
   var dbcl = cs.createCL( clName, { DataSource: srcDataName } );
   crudAndCheckResult( dbcl );

   clearDataSource( csName, srcDataName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.dropCS( csName );
   datasrcDB.close();
}

function crudAndCheckResult ( dbcl )
{
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "testb" }, { a: 3, b: { a: 1 } }, { a: 4, b: ["tsta", "testb"] }, { a: 5, b: 9223372036854 },
   { a: 6, b: { "$date": "2021-01-01" } }, { a: 7, b: { "$timestamp": "2037-12-31-23.59.59.999999" } },
   { a: 8, b: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "255" } }, { a: 9, b: { "$minKey": 1 } },
   { a: 10, b: { $decimal: "100.01" } }];
   dbcl.insert( docs );
   var cursor = dbcl.find();
   commCompareResults( cursor, docs );

   dbcl.update( { $set: { 'b.0': "testaaaaaa" } }, { a: 4 } );
   var cursor = dbcl.find();
   var expRecs = [{ a: 1, b: 1 }, { a: 2, b: "testb" }, { a: 3, b: { a: 1 } }, { a: 4, b: ["testaaaaaa", "testb"] }, { a: 5, b: 9223372036854 },
   { a: 6, b: { "$date": "2021-01-01" } }, { a: 7, b: { "$timestamp": "2037-12-31-23.59.59.999999" } },
   { a: 8, b: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "255" } }, { a: 9, b: { "$minKey": 1 } },
   { a: 10, b: { $decimal: "100.01" } }];
   commCompareResults( cursor, expRecs );

   dbcl.upsert( { $set: { 'b.a': "testupsertaaaaaa" } }, { a: 11 } );
   var cursor = dbcl.find();
   var expRecs = [{ a: 1, b: 1 }, { a: 2, b: "testb" }, { a: 3, b: { a: 1 } }, { a: 4, b: ["testaaaaaa", "testb"] }, { a: 5, b: 9223372036854 },
   { a: 6, b: { "$date": "2021-01-01" } }, { a: 7, b: { "$timestamp": "2037-12-31-23.59.59.999999" } },
   { a: 8, b: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "255" } }, { a: 9, b: { "$minKey": 1 } },
   { a: 10, b: { $decimal: "100.01" } }, { "a": 11, "b": { "a": "testupsertaaaaaa" } }];
   commCompareResults( cursor, expRecs );

   dbcl.find( { a: 4 } ).update( { $set: { b: "testfindandupdate" } } ).toArray();
   var cursor = dbcl.find();
   var expRecs = [{ a: 1, b: 1 }, { a: 2, b: "testb" }, { a: 3, b: { a: 1 } }, { a: 4, b: "testfindandupdate" }, { a: 5, b: 9223372036854 },
   { a: 6, b: { "$date": "2021-01-01" } }, { a: 7, b: { "$timestamp": "2037-12-31-23.59.59.999999" } },
   { a: 8, b: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "255" } }, { a: 9, b: { "$minKey": 1 } },
   { a: 10, b: { $decimal: "100.01" } }, { "a": 11, "b": { "a": "testupsertaaaaaa" } }];
   commCompareResults( cursor, expRecs );

   dbcl.find( { a: 11 } ).remove().toArray();
   var cursor = dbcl.find();
   var expRecs = [{ a: 1, b: 1 }, { a: 2, b: "testb" }, { a: 3, b: { a: 1 } }, { a: 4, b: "testfindandupdate" }, { a: 5, b: 9223372036854 },
   { a: 6, b: { "$date": "2021-01-01" } }, { a: 7, b: { "$timestamp": "2037-12-31-23.59.59.999999" } },
   { a: 8, b: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "255" } }, { a: 9, b: { "$minKey": 1 } },
   { a: 10, b: { $decimal: "100.01" } }];
   commCompareResults( cursor, expRecs );


   dbcl.remove( { a: { $gt: 2 } } );
   var cursor = dbcl.find();
   var expRecs = [{ a: 1, b: 1 }, { a: 2, b: "testb" }];
   commCompareResults( cursor, expRecs );

   var count = dbcl.count();
   var expNum = 2;
   assert.equal( expNum, count );

   dbcl.truncate();
   var expNum = 0;
   var count = dbcl.count();
   assert.equal( expNum, count );
}
