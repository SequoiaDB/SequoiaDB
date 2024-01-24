/***************************************************************************************************
 * @Description: 设置事务同步一致性策略
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
 *    验证事务同步一致性策略
 * 测试步骤：
 *    1. 设置事务同步一致性策略
 *    2. 设置事务同步一致性策略非法值
 * 期望结果：
 *    步骤2：非法值报错，-6
 **************************************************************************************************/
testConf.skipStandAlone = true;

main(test);
function test()
{
   var options_1 = { "transconsistencystrategy":1 };
   var options_2 = { "transconsistencystrategy":2 };
   var options_3 = { "transconsistencystrategy":3 };
   var options_4 = { "transconsistencystrategy":true };
   var options_5 = { "transconsistencystrategy":{"bson":1}};
   var options_6 = { "transconsistencystrategy":[11] };
   var options_7 = { "transconsistencystrategy":"string" };
   db.updateConf( options_1 ) ;
   db.updateConf( options_2 ) ;
   db.updateConf( options_3 ) ;
   try
   {
      db.updateConf( options_4 ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
   try
   {
      db.updateConf( options_5 ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
   try
   {
      db.updateConf( options_6 ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
   try
   {
      db.updateConf( options_7 ) ;
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

}


