/***************************************************************************************************
 * @Description: 创建集合并指定同步一致性策略
 * @ATCaseID: <填写 story 文档中验收用例的用例编号>
 * @Author: JiangFeng You
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 12/08/2022 JiangFeng You Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：
 * 测试场景：
 *    验证集合同步一致性策略
 * 测试步骤：
 *    1. 创建集合并指定同步一致性策略
 *    2. 创建集合并指定同步一致性策略非法值
 * 期望结果：
 *    步骤2：非法值报错，-6
 **************************************************************************************************/
testConf.skipStandAlone = true;

main(test);
function test()
{
   var clName = "consistency_2";
   commDropCL( db, COMMCSNAME, clName );
   var options_1 = { "ConsistencyStrategy":1 };
   var options_2 = { "ConsistencyStrategy":2 };
   var options_3 = { "ConsistencyStrategy":3 };
   var options_4 = { "ConsistencyStrategy":true };
   var options_5 = { "ConsistencyStrategy":{"bson":1}};
   var options_6 = { "ConsistencyStrategy":[11] };
   var options_7 = { "ConsistencyStrategy":"string" };
   commCreateCL( db, COMMCSNAME, clName, options_1  );
   commDropCL( db, COMMCSNAME, clName  );
   commCreateCL( db, COMMCSNAME, clName, options_2 );
   commDropCL( db, COMMCSNAME, clName  );
   commCreateCL( db, COMMCSNAME, clName, options_3 );
   commDropCL( db, COMMCSNAME, clName  );
   try
   {
      commCreateCL( db, COMMCSNAME, clName, options_4 );
   }
   catch( e )
   {
      commDropCL( db, COMMCSNAME, clName  );
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
   try
   {
      commCreateCL( db, COMMCSNAME, clName, options_5 );
   }
   catch( e )
   {
      commDropCL( db, COMMCSNAME, clName  );
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
   try
   {
      commCreateCL( db, COMMCSNAME, clName, options_6 );
   }
   catch( e )
   {
      commDropCL( db, COMMCSNAME, clName  );
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
   try
   {
      commCreateCL( db, COMMCSNAME, clName, options_7 );
   }
   catch( e )
   {
      commDropCL( db, COMMCSNAME, clName  );
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

}

