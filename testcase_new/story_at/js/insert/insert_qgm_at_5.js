/***************************************************************************************************
 * @Description: Insert into select from 测试
 * @ATCaseID: insert_qgm_at_5
 * @Author: He Guoming
 * @TestlinkCase: 无
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 11/14/2023 He Guoming    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：任意正常集群
 * 测试场景：
 *    对有MB以上大小的大记录的集合作为源执行insert into select from。
 * 测试步骤：
 *    1. 创建两个CL
 *    2. CL1中灌入100条1MB的数据
 *    3. 通过insert into select from把数据从CL1灌入CL2
 * 期望结果：
 *    1. 插入成功
 *
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "_insert_qgm_at_5"
var csName = "insert_qgm_at_5_cs";
var clName = "insert_flag_at_5_cl";

main(wrap)

function wrap(testPara) {
   try {
      commDropCS( db, csName, true, "init env" )
      db.createCS( csName ).createCL( clName )
      test(testPara)
   } finally {
      commDropCS(db, csName, true, "drop env");
   }
}

function test() {
   cl1 = testPara.testCL
   cl2 = db.getCS(csName).getCL(clName)
   var array = new Array(1000000)
   var string = array.join('a')
   var recNum = 100
   data=[]
   for(i = 0; i < recNum; i++)
      data.push({a:i,b:string})
   cl1.insert(data)
   db.execUpdate('insert into ' + csName + '.' + clName + " select * from " + testConf.csName + '.' + testConf.clName)
   var count2 = cl2.count()
   assert.equal(count2, recNum)
}