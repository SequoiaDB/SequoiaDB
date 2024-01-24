/******************************************************************************
 * @Description   : seqDB-33373:多个索引，其中较优索引在较后的索引槽位时选择最优索引
 * @Author        : wuyan
 * @CreateTime    : 2022.09.12
 * @LastEditTime  : 2023.09.12
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_index33373";

main( test );
function test ()
{
   var cl = testPara.testCL;
   cl.createIndex( 'primary', { jrn_no: 1, acct_date: 1, jrn_seq_no: 1, app_id: 1 } )
   cl.createIndex( 'index1', { update_ts: 1, src_app_id: 1, del_flag: 1 } )
   cl.createIndex( 'index2', { own_acct_no: 1, acct_date: 1, create_ts: 1, jrn_seq_no: 1 } )
   cl.createIndex( 'index3', { own_trx_media: 1, acct_date: 1, orig_trx_acct_date: 1, trx_amt: 1, core_product_code: 1 } )
   cl.createIndex( 'index4', { trx_br: 1, create_tlr_no: 1, acct_date: 1 } )
   cl.createIndex( 'index5', { jrn_no: 1, orig_trx_acct_date: 1 } )
   cl.createIndex( 'index6', { part_no: 1, acct_date: 1 } )
   cl.createIndex( 'index7', { trx_summ_core: 1, acct_date: 1, cntrprt_acct_no: 1, cntrprt_trx_media: 1, oppo_acct_no: 1, oppo_trx_media: 1 } )
   cl.createIndex( 'index8', { cntrprt_trx_media: 1, acct_date: 1 } )
   cl.createIndex( 'index9', { cntrprt_acct_no: 1, acct_date: 1 } )

   insertData( cl );

   var queryConf = {
      $and: [{ del_flag: "N" },
      { app_id: "SVCT" },
      { print_flag: "Y" },
      { trx_sts: "N" },
      { debit_cr_flag: "D" },
      { cntrprt_trx_media: { $in: ["85555555", "MrchntNo00001"] } },
      {
         $and: [{ acct_date: { $gte: "20230401" } },
         { acct_date: { $lte: "20230831" } }]
      },
      { own_trx_media: { $ne: "" } },
      { own_trx_media: { $ne: { $field: "own_acct_no" } } }]
   };
   var orderBy = { acct_date: 1, create_ts: 1, jrn_seq_no: 1 };
   var hint = { "": "" };

   //查询条件和排序条件都满足选择索引为index8和index3，其中index8为最优匹配索引
   var expIndexName = "index8";
   var indexName = cl.find( queryConf ).sort( orderBy ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expIndexName, "use index is " + indexName );

   indexName = cl.find( queryConf ).sort( orderBy ).hint( hint ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expIndexName, "use index is " + indexName );

   //执行analyze收集统计信息后，再次查询  
   db.analyze( { Collection: COMMCSNAME + "." + testConf.clName } );
   indexName = cl.find( queryConf ).sort( orderBy ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expIndexName, "use index is " + indexName );

   indexName = cl.find( queryConf ).sort( orderBy ).hint( hint ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expIndexName, "use index is " + indexName );
}

function insertData ( dbcl )
{
   var batchSize = 10000;
   for( j = 0; j < 5; j++ ) 
   {
      data = [];
      for( i = 0; i < batchSize; i++ )
      {
         data.push( {
            jrn_no: i + j * batchSize,
            acct_date: i + j * batchSize,
            jrn_seq_no: i + j * batchSize,
            app_id: i + j * batchSize,
            update_ts: i + j * batchSize,
            src_app_id: i + j * batchSize,
            del_flag: i + j * batchSize,
            own_acct_no: i + j * batchSize,
            create_ts: i + j * batchSize,
            own_trx_media: i + j * batchSize,
            orig_trx_acct_date: i + j * batchSize,
            trx_amt: i + j * batchSize,
            core_product_code: i + j * batchSize,
            trx_br: i + j * batchSize,
            create_tlr_no: i + j * batchSize,
            part_no: i + j * batchSize,
            trx_summ_core: i + j * batchSize,
            cntrprt_acct_no: i + j * batchSize,
            cntrprt_trx_media: i + j * batchSize,
            oppo_acct_no: i + j * batchSize,
            oppo_trx_media: i + j * batchSize
         } );
      }
      dbcl.insert( data );
   }
}

