/******************************************************************************
*@Description: test table have amout of index pages, then truncate
*@Modify list:
*              2015-5-8  xiaojun Hu   Init
******************************************************************************/
main( test );
function test ()
{
   testTruncateNormalCLMultiIndexKey( db )
   if( true == commIsStandalone( db ) )
   {
   }
   else if( commGetGroups( db ).length < 2 )
   {
   }
   else
   {
      testTruncateMixtureCLMultiIndex( db )
   }
}
/*******************************************************************************
*@Description: 测试普通表中的索引是复合索引时,且索引增涨几个页，然后再truncate
*@Input: collection.truncate()
*@Expectation: truncate清除数据, 并且清除数据页
********************************************************************************/

function testTruncateNormalCLMultiIndexKey ( db )
{
   var clName = COMMCLNAME + "_12226";
   var indexName = CHANGEDPREFIX + "_truncate_normal_index";
   var recordNum = 3000;
   var verJsonObj = { "TotalIndexPages": 2 };

   commDropCL( db, COMMCSNAME, clName, true, true, "drop cl begin" );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false );
   commDropIndex( cl, indexName, true )

   var tableName = COMMCSNAME + "." + clName;
   truncateVerify( db, tableName );
   // insert record: { id: 1, stringKey:-1, integerKey:1 }
   cl.createIndex( indexName, { id: 1, stringKey: -1, integerKey: 1 } );
   truncateInsertRecord( cl, recordNum );
   cl.truncate();
   truncateVerify( db, tableName, verJsonObj );
   commDropCL( db, COMMCSNAME, clName, false, false, "drop cl end" );
}

/*******************************************************************************
*@Description: 测试混合分区表中创建多个索引，索引页增涨, 再做truncate.
*@Input: collection.truncate()
*@Expectation: truncate清除数据, 并且清除数据页
********************************************************************************/
function testTruncateMixtureCLMultiIndex ( db )
{
   var mainCS = CHANGEDPREFIX + "_mixtureCL_largeThanPage_mainCL12233";
   var subCS1 = CHANGEDPREFIX + "_mixtureCL_largeThanPage_subCL1_12233";
   var subCS2 = CHANGEDPREFIX + "_mixtureCL_largeThanPage_subCL2_12233";
   var mainCLOption = {
      "ShardingKey": { "id": 1 }, "ShardingType": "range", "IsMainCL": true
   };
   var subCLOption = {
      "ShardingKey": { "id": 1 }, "ShardingType": "hash"
   };
   var recordNum = 4000;
   var domainName = CHANGEDPREFIX + "_mixture_domain";
   var verJsonObj1 = { "TotalIndexPages": 5 };
   var verJsonObj2 = { "TotalIndexPages": 10 };
   var indexName1 = CHANGEDPREFIX + "_truncate_normal_index_1";
   var indexName2 = CHANGEDPREFIX + "_truncate_normal_index_2";
   var indexName3 = CHANGEDPREFIX + "_truncate_normal_index_3";

   commDropCS( db, subCS1, true, "drop sub cs1 begin" );
   commDropCS( db, subCS2, true, "drop sub cs2 begin" );
   commDropCS( db, mainCS, true, "drop main cs begin" );
   commDropDomain( db, domainName );
   // create domain
   var domainRGs = new Array();
   var groups = commGetGroups( db );
   for( var i = 0; i < groups.length; ++i )
   {
      domainRGs[i] = groups[i][0]["GroupName"];
   }
   commCreateDomain( db, domainName, domainRGs, { AutoSplit: true } );

   var mainCL = commCreateCL( db, mainCS, COMMCLNAME, mainCLOption );
   commCreateCS( db, subCS1, false, "create sub cs1", { "Domain": domainName } );
   var subCL1 = commCreateCL( db, subCS1, COMMCLNAME, subCLOption );
   commCreateCS( db, subCS2, false, "create sub cs2", { "Domain": domainName } );
   var subCL2 = commCreateCL( db, subCS2, COMMCLNAME, subCLOption );
   var subTable1 = subCS1 + "." + COMMCLNAME;
   var subTable2 = subCS2 + "." + COMMCLNAME;

   mainCL.attachCL( subTable1, { "LowBound": { "id": 0 }, "UpBound": { "id": 2000 } } );
   mainCL.attachCL( subTable2, { "LowBound": { "id": 2000 }, "UpBound": { "id": 4000 } } );
   commDropIndex( mainCL, indexName1, true )
   commDropIndex( mainCL, indexName2, true )
   commDropIndex( mainCL, indexName3, true )

   // create index and insert data
   mainCL.createIndex( indexName1, { id: -1 } );
   mainCL.createIndex( indexName2, { stringKey: 1 } );
   mainCL.createIndex( indexName3, { integerKey: -1 } );
   db.sync();
   truncateVerify( db, subTable1, verJsonObj2 );
   truncateVerify( db, subTable2, verJsonObj2 );

   truncateInsertRecord( mainCL, recordNum );
   mainCL.truncate();

   truncateVerify( db, subTable1, verJsonObj1 );
   truncateVerify( db, subTable2, verJsonObj1 );
   commDropCS( db, subCS1, false, "drop sub cs1 end" );
   commDropCS( db, subCS2, false, "drop sub cs2 end" );
   commDropCS( db, mainCS, false, "drop main cs end" );
   commDropDomain( db, domainName );
}

