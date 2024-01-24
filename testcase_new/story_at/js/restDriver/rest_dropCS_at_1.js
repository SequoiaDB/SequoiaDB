/***************************************************************************************************
 * @Description: rest 驱动 dropCS() 接口 options 参数测试
 * @ATCaseID: rest_dropCS_at_1
 * @Author: Yang Qincheng
 * @TestlinkCase: 无
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 02/06/2023 Yang Qincheng Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：任意正常集群
 * 测试场景：
 *    删除非空 cs
 * 测试步骤：
 *    1. 创建一个 cs
 *    2. 创建一个 cl，并写入数据
 *    3. 设置 EnsureEmpty 为 true，删除该 cs
 * 期望结果：
 *    步骤3：报 -275 错误
 **************************************************************************************************/

 main(test);
 function test() {
   var csName = "rest_dropCS_at_1_cs";
   var clName = "rest_dropCS_at_1_cl";

   commDropCS( db, csName, true, "drop cs in begin" );

   var cl = db.createCS( csName ).createCL( clName );
   cl.insert( { "a": 1 } )

   tryCatch( ["cmd=drop collectionspace", "name=" + csName, 'options={EnsureEmpty:true}'], [-275], "Not-empty collectionspace cannot be deleted" );

   commDropCS( db, csName, true, "drop cl in clean" );
 }
