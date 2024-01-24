/***************************************************************************************************
 * @Description: 复合索引选择
 * @ATCaseID: <填写 story 文档中验收用例的用例编号>
 * @Author: Zhou Hongye
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 05/12/2023 Zhou Hongye   Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：
 * 测试场景：
 *    在集合上有普通索引和复合索引，对集合执行查询，查询条件为复合索引覆盖的所有字段，查看访问计划选择的索引为该复合索引
 * 测试步骤：
 *    1.创建集合
 *    2.在集合上创建字段c的普通索引，字段a与c的复合索引，字段b与d的复合索引，字段c与d的复合索引，字段a,b,c,d,e的复合唯一索引
 *    3.写入一定量数据
 *    4.执行查询，查询条件为{a:1, b:1, c:1, d:1, e:1} 
 *    
 * 期望结果：
 *    访问计划选择的索引为字段a,b,c,d,e的复合唯一索引
 *    
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "_compositeIndex_at_1";

main(test);
function test(testPara)
{
    var cl = testPara.testCL;
    cl.createIndex("c", {"c":1});
    cl.createIndex("ac", {"a":1,"c":1});
    cl.createIndex("bd", {"b":1,"d":1});
    cl.createIndex("cd", {"c":1,"d":1});
    cl.createIndex("uniqueIdx", {"a": 1,"b":1,"c":1,"d":1,"e":1}, {"Unique": true});

    var batchSize = 10000;
    for (j = 0; j < 5; j++) {
      data = [];
      for (i = 0; i < batchSize; i++)
        data.push({
          a: i + j * batchSize,
          b: i + j * batchSize,
          c: i + j * batchSize,
          d: i + j * batchSize,
          e: i + j * batchSize,
        });
      cl.insert(data);
    }

    assert.equal(
      cl.find({ a: 1, b: 1, c: 1, d: 1, e: 1 }).explain().current().toObj()[
        "IndexName"
      ],
      "uniqueIdx"
    );
}