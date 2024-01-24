/***************************************************************************************************
 * @Description: 复合索引选择
 * @ATCaseID: <填写 story 文档中验收用例的用例编号>
 * @Author: He Guoming
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 07/20/2023 HGM           Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：
 * 测试场景：
 *    在集合上有多个索引，较优的索引在较后的索引槽位
 * 测试步骤：
 *    1.创建集合
 *    2.在集合上创建多个索引
 *    3.执行查询，查询条件使用较后的索引为最优
 *
 * 期望结果：
 *    访问计划选择的索引为槽位较后的索引
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "_compositeIndex_at_15";

main(test);
function test(testPara)
{
   db.setSessionAttr({PreferredInstance:"m"}) ;
   var cl = testPara.testCL;

   cl.createIndex('primary',{jrn_no:1,acct_date:1,jrn_seq_no:1,app_id:1})
   cl.createIndex('hist1',{update_ts:1,src_app_id:1,del_flag:1})
   cl.createIndex('hist2',{own_acct_no:1,acct_date:1,create_ts:1,jrn_seq_no:1})
   cl.createIndex('hist3',{own_trx_media:1,acct_date:1,orig_trx_acct_date:1,trx_amt:1,core_product_code:1})
   cl.createIndex('hist4',{trx_br:1,create_tlr_no:1,acct_date:1})
   cl.createIndex('hist5',{jrn_no:1,orig_trx_acct_date:1})
   cl.createIndex('hist6',{part_no:1,acct_date:1})
   cl.createIndex('hist7',{trx_summ_core:1,acct_date:1,cntrprt_acct_no:1,cntrprt_trx_media:1,oppo_acct_no:1,oppo_trx_media:1})
   cl.createIndex('hist8',{cntrprt_trx_media:1,acct_date:1})
   cl.createIndex('hist9',{cntrprt_acct_no:1,acct_date:1})

   var query = {$and:[{del_flag:"N"},
                      {app_id:"SVCT"},
                      {print_flag:"Y"},
                      {trx_sts:"N"},
                      {debit_cr_flag:"D"},
                      {cntrprt_trx_media:{$in:["85555555","MrchntNo00001"]}},
                      {$and:[{acct_date:{$gte:"20230401"}},
                             {acct_date:{$lte:"20230831"}}]},
                      {own_trx_media:{$ne:""}},
                      {own_trx_media:{$ne:{$field:"own_acct_no"}}}]};
   var orderBy = {acct_date:1,create_ts:1,jrn_seq_no:1};
   var hint = {"":""};
   var indexName = "";
   indexName = cl.find(query).sort(orderBy).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "hist8" );
   indexName = cl.find(query).sort(orderBy).hint(hint).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "hist8" );

   var batchSize = 10000;
   for (j = 0; j < 5; j++) {
      data = [];
      for (i = 0; i < batchSize; i++)
         data.push({
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
         });
      cl.insert(data);
   }

   indexName = cl.find(query).sort(orderBy).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "hist8" );
   indexName = cl.find(query).sort(orderBy).hint(hint).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "hist8" );

   db.analyze({Collection: testConf.csName + "." + testConf.clName } );

   indexName = cl.find(query).sort(orderBy).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "hist8" );
   indexName = cl.find(query).sort(orderBy).hint(hint).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "hist8" );
}