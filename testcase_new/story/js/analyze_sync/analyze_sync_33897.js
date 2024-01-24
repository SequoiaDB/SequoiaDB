/******************************************************************************
 * @Description   : seqDB-33897:不同子表查询走索引不一致，配置不同的plancachemainclthreshold查看访问计划
 * @Author        : liuli
 * @CreateTime    : 2023.11.20
 * @LastEditTime  : 2023.11.21
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test )

function test ()
{
   var csName = COMMCSNAME;
   var mainCLName = "maincl_33897";
   var subCLName1 = "subcl_33897_1";
   var subCLName2 = "subcl_33897_2";

   db.deleteConf( { plancachemainclthreshold: 1 } );
   var group = commGetDataGroupNames( db )[0];
   var cs = testPara.testCS;
   var maincl = cs.createCL( mainCLName, { IsMainCL: true, ShardingKey: { a: 1 } } );
   cs.createCL( subCLName1, { Group: group } );
   cs.createCL( subCLName2, { Group: group } );

   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: -1000000000 }, UpBound: { a: 0 } } )
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 0 }, UpBound: { a: 1000000000 } } )

   maincl.createIndex( 'abcd', { a: 1, b: 1, c: 1, d: 1 } );
   maincl.createIndex( 'e', { e: 1 } );
   maincl.createIndex( 'af', { a: 1, f: 1 } );
   maincl.createIndex( 'gh', { g: 1, h: 1 } );
   maincl.createIndex( 'ijkc', { i: 1, j: 1, k: 1, c: 1 } );
   maincl.createIndex( 'l', { l: 1 } );
   maincl.createIndex( 'm', { m: 1 } );
   maincl.createIndex( 'n', { n: 1 } );
   maincl.createIndex( 'o', { o: 1 } );
   maincl.createIndex( 'pq', { p: 1, q: 1 } );

   // 子表subcl1有数据，子表subcl2无数据
   insertDocs( maincl );

   testPlan( false );
   db.updateConf( { plancachemainclthreshold: -1 } );
   testPlan( false );
   db.updateConf( { plancachemainclthreshold: 0 } );
   testPlan( true );
   db.deleteConf( { plancachemainclthreshold: 1 } );

   // 执行analyze
   db.analyze( { Collection: csName + "." + mainCLName } );

   testPlan( false );
   db.updateConf( { plancachemainclthreshold: -1 } );
   testPlan( false );
   db.updateConf( { plancachemainclthreshold: 0 } );
   testPlan( true );
   db.deleteConf( { plancachemainclthreshold: 1 } );

   commDropCS( db, csName );
}

function testPlan ( alwaysMainCLPlan )
{
   var csName = COMMCSNAME;
   var mainCLName = "maincl_33897";
   var subCLName1 = "subcl_33897_1";
   var subCLName2 = "subcl_33897_2";

   // 使子表的访问计划有效
   var dbcs = db.getCS( csName );
   var subcl = dbcs.getCL( subCLName1 );
   for( var j = 0; j < 10; ++j )
   {
      var temp1 = [];
      for( var i = 0; i < 40 + j; ++i )
      {
         temp1.push( i );
      }
      var temp2 = [];
      for( var i = 0; i < 8; ++i )
      {
         temp2.push( i );
      }
      var query = { h: "N", j: { $in: temp2 }, l: { $in: temp1 } };
      var order = { u: 1 };
      var listIndex = subcl.find( query ).sort( order ).hint( { "": "" } ).explain().current().toObj();
      var indexName = listIndex["IndexName"];
      assert.equal( indexName, "gh", JSON.stringify( listIndex ) );
   }

   // 查询访问计划
   var maincl = dbcs.getCL( mainCLName );
   var temp1 = [];
   for( var i = 0; i < 40; ++i )
   {
      temp1.push( i );
   }
   var temp2 = [];
   for( var i = 0; i < 8; ++i )
   {
      temp2.push( i );
   }
   var query = { h: "N", j: { $in: temp2 }, l: { $in: temp1 } };
   var order = { u: 1 };

   var plan = maincl.find( query ).sort( order ).hint( { "": "" } ).explain().current().toObj();
   for( var i = 0; i < plan["SubCollections"].length; ++i )
   {
      clName = plan["SubCollections"][i]["Name"];
      indexName = plan["SubCollections"][i]["IndexName"];
      if( !alwaysMainCLPlan && clName == csName + "." + subCLName2 )
      {
         assert.equal( indexName, "l", JSON.stringify( plan ) );
      }
      else
      {
         assert.equal( indexName, "gh", JSON.stringify( plan ) );
      }
   }
}

function insertDocs ( cl )
{
   var batchSize = 10000;
   for( j = 0; j < 5; j++ )
   {
      data = [];
      for( i = 0; i < batchSize; i++ )
         data.push( {
            a: i + j * batchSize,
            b: i + j * batchSize,
            c: i + j * batchSize,
            d: i + j * batchSize,
            e: i + j * batchSize,
            f: i + j * batchSize,
            g: i + j * batchSize,
            h: i + j * batchSize,
            i: i + j * batchSize,
            j: i + j * batchSize,
            k: i + j * batchSize,
            l: i + j * batchSize,
            m: i + j * batchSize,
            n: i + j * batchSize,
            o: i + j * batchSize,
            p: i + j * batchSize,
            q: i + j * batchSize,
            r: i + j * batchSize,
            s: i + j * batchSize,
            t: i + j * batchSize,
            u: i + j * batchSize
         } );
      cl.insert( data );
   }
}