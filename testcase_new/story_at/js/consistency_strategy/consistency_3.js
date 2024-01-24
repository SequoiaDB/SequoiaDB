/***************************************************************************************************
 * @Description: 验证事务默认同步一致性策略
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
 *    验证事务默认同步一致性策略
 * 测试步骤：
 *    1. 连接sdb
 *    2. 获取db的配置快照
 *    3. 查看事务的同步一致性策略
 * 期望结果：
 *    步骤3：事务的同步一致性策略默认值为3
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test(args)
{
   var snap = db.snapshot( 13, {}, { "transconsistencystrategy": null } );
   while ( snap.next() )
   {
      var transconsistencystrategy = snap.current().toObj()['transconsistencystrategy'];
      if( transconsistencystrategy !== 3 )
      {
         throw new Error( "check transconsistencystrategy, \nexpect: 3, \nbut found: " + transconsistencystrategy );
      }
   }
}


