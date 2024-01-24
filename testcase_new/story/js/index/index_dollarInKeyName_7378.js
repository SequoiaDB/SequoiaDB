/******************************************************************************
*@Description: create Index failed when use $ symbol in indexDef key
*              [ JIRA: SEQUOIADBMAINSTREAM-820 ]
*@Modify list:
*              2014-5-5  xiaojun Hu   Init
******************************************************************************/

/*******************************************************************************
*@Description: 在普通表上创建索引时, 指定Key的名字有"$"符号
*@Input: cl.createIndex( "indexName", { "a.$0.b": 1 })
*@Expectation: 创建索引失败，报-6错误【Invalid Argument】
********************************************************************************/

main( test );

function test ()
{
   // normal cl
   testNormalClIndex( db );
   if( false == commIsStandalone( db ) )
   {
      // main and sub cl
      testMainSubClIndex( db );
      // split cl
      testSplitClIndex( db );
   }
}

function testNormalClIndex ( db )
{
   var clName = CHANGEDPREFIX + "_idxNormalCl";
   var indexName = CHANGEDPREFIX + "_index";
   var indexDef1 = { "a.$0.b": 1 };
   var indexDef2 = { "a$b": 1 };
   var indexDef3 = { "f.d": -1, "a.$0.b": 1 };
   commDropCL( db, COMMCSNAME, clName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false );
   cl.insert( { "a": [{ "b": 1 }, { "c": 2 }] } );

   // { "a.$0.b": 1 }
   commDropIndex( cl, indexName, true );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.createIndex( indexName, indexDef1 );
   } )

   // { "a$b": 1 }
   commDropIndex( cl, indexName, true );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.createIndex( indexName, indexDef2 );
   } )

   // { "f.d":-1, "a.$0.b": 1 }
   commDropIndex( cl, indexName, true );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.createIndex( indexName, indexDef3 );
   } )
   commDropCL( db, COMMCSNAME, clName, false, false );
}

/*******************************************************************************
*@Description: 在主子表上创建索引时, 指定Key的名字有"$"符号
*@Input: cl.createIndex( "indexName", { "a.$0.b": 1 })
*@Expectation: 创建索引失败，报-6错误【Invalid Argument】
********************************************************************************/
function testMainSubClIndex ( db )
{
   var mainClName = CHANGEDPREFIX + "_indexMainCl";
   var subClName = CHANGEDPREFIX + "_indexSubCl";
   var indexName = CHANGEDPREFIX + "_indexMainSubCl";
   var indexDef = { "a.$0.b": 1 };
   var optionObj = { "ShardingKey": { a: 1 }, "IsMainCL": true };
   commDropCL( db, COMMCSNAME, mainClName, true, true );
   commDropCL( db, COMMCSNAME, subClName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, mainClName, optionObj, true, false );
   commCreateCL( db, COMMCSNAME, subClName, {}, true, false );
   commDropIndex( cl, indexName, true );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.attachCL( COMMCSNAME + "." + subClName, { LowBound: { a: 1 }, UpBound: { a: 10 } } );
      cl.insert( { a: 1 } );
      cl.insert( { a: 9 } );
      cl.createIndex( indexName, indexDef, false, true );
   } )
   commDropCL( db, COMMCSNAME, mainClName, false, false );
   /*   drop main collection will drop sub collection when in same CS
   commDropCL( db, COMMCSNAME, subClName, false, false,
               "drop sub collection end, " + funcName );
   */
}

/*******************************************************************************
*@Description: 在水平分区表上创建索引时, 指定Key的名字有"$"符号
*@Input: cl.createIndex( "indexName", { "a.$0.b": 1 })
*@Expectation: 创建索引失败，报-6错误【Invalid Argument】
********************************************************************************/
function testSplitClIndex ( db )
{
   var clName = CHANGEDPREFIX + "_idxSplitCl";
   var indexName = CHANGEDPREFIX + "_indexSplitCl";
   var domainName = CHANGEDPREFIX + "_domainIdxSplit";
   var indexDef = { "a.$0.b": 1 };
   var optionObj = { "ShardingKey": { a: -1 }, "ShardingType": "hash", "AutoSplit": true };
   commDropCL( db, COMMCSNAME, clName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, clName, optionObj, true, false );
   commDropIndex( cl, indexName, true );
   commDropDomain( db, domainName );
   var groups = commGetGroups( db );
   var domainGroups = new Array();
   for( var i = 0; i < groups.length; ++i )
   {
      domainGroups[i] = groups[i][0].GroupName;
   }
   commCreateDomain( db, domainName, domainGroups, { "AutoSplit": true } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.insert( { a: 9 } );
      cl.createIndex( indexName, indexDef, false, true );
   } );

   commDropCL( db, COMMCSNAME, clName, false, false );
   commDropDomain( db, domainName );
}
