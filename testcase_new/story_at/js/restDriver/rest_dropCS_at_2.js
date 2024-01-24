/***************************************************************************************************
 * @Description: rest 驱动 dropCS() 接口 options 参数测试
 * @ATCaseID: rest_dropCS_at_2
 * @Author: Yang Qincheng
 * @TestlinkCase: 无
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 02/01/2023 Yang Qincheng Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：任意正常集群
 * 测试场景：
 *    异常参数
 * 测试步骤：
 *    1. 准备两个集合空间：cs1、cs2
 *    2. 删除 cs1 时，指定异常参数 {Name: "cs2Name"}
 * 期望结果：
 *    步骤2：cs1 删除成功，cs2 不受影响
 **************************************************************************************************/

 main(test);
 function test() {
   var csName1 = "rest_dropCS_at_2_cs_A";
   var csName2 = "rest_dropCS_at_2_cs_B";
   var clName = "rest_dropCS_at_2_cl";

   prepareCS( csName1, clName );
   prepareCS( csName2, clName );

   // drop cs by rest driver
   tryCatch( ["cmd=drop collectionspace", "name=" + csName1, 'options={Name:\"' + csName2 + '\"}'], [0], "Faile to drop collectionspace" );
   
   // check result
   try
   {
      db.getCS( csName1 );
      throw new Error( "Need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_DMS_CS_NOTEXIST )
      {
         throw e;
      }
   }
   db.getCS( csName2 );

   commDropCS( db, csName1, true, "drop cl in clean" );
   commDropCS( db, csName2, true, "drop cl in clean" );
 }

 function prepareCS( csName, clName )
 {
    commDropCS( db, csName, true, "drop cs in begin" );
    var cl = db.createCS( csName ).createCL( clName );
    cl.insert( { "a": 1 } )
 }
