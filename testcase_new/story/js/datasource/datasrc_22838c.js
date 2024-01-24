/******************************************************************************
 * @Description   : seqDB-22838:创建数据源，设置访问权限为ALL
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.16
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc22838C";
   var csName = "cs_22838C";
   var srcCSName = "datasrcCS_22838C";
   var clName = "cl_22838C";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   var scs = datasrcDB.createCS( srcCSName );
   scs.createCL( clName );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { AccessMode: "ALL" } );
   var dsMarjorVersion = getDSMajorVersion( dataSrcName );
   //集合级使用数据源
   var cs = db.createCS( csName );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   crudAndCheckResult( dbcl );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   //集合空间级使用数据源
   db.dropCS( csName );
   var cs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = db.getCS( csName ).getCL( clName );
   crudAndCheckResult( dbcl );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   clearDataSource( csName, dataSrcName );
   datasrcDB.dropCS( srcCSName );
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

function lobAndCheckResult ( dbcl, dsMarjorVersion )
{
   var filePath = WORKDIR + "/lob22838/";
   var fileName = "filelob_22838";
   var fileSize = 1024;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );

   //putLob and getLob
   var lobID = dbcl.putLob( filePath + fileName );
   dbcl.getLob( lobID, filePath + "checkputlob22838" );
   var actMD5 = File.md5( filePath + "checkputlob22838" );
   assert.equal( fileMD5, actMD5 );

   //list Lob
   var rc = dbcl.listLobs();
   while( rc.next() )
   {
      var obj = rc.current().toObj();
      var listSize = obj["Size"];
      assert.equal( fileSize, listSize );
   }

   //truncateLob
   var size = 200;
   //只有3.0以上版本才支持truncateLob操作，2.8版本不支持报错-315（SDB_OPERATION_INCOMPATIBLE）   
   if( dsMarjorVersion < 3 )
   {
      assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
      {
         dbcl.truncateLob( lobID, size );
      } );
   }
   else
   {
      dbcl.truncateLob( lobID, size );
      cmd.run( "truncate -s " + ( size ) + " " + filePath + fileName );
      var expMD5 = File.md5( filePath + fileName );
      dbcl.getLob( lobID, filePath + "checktruncateLob22838c" );
      var actMD5 = File.md5( filePath + "checktruncateLob22838c" );
      assert.equal( expMD5, actMD5 );
   }

   //deleteLob
   dbcl.deleteLob( lobID );
   assert.tryThrow( SDB_FNE, function()
   {
      dbcl.getLob( lobID, filePath + "checkdeletelob22838c" );
   } );

   deleteTmpFile( filePath );
}