/******************************************************************************
 * @Description   : seqDB-24996:$strlenBytes功能测试
 * @Author        : liuli
 * @CreateTime    : 2022.02.10
 * @LastEditTime  : 2022.02.11
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24996";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   // 构造数据
   var docs = [{ a: "Sequoiadb" }, { a: "巨杉" }, { a: "巨杉数据库" }, { a: "巨杉Sequoiadb" }, { a: ["Sequoiadb", "巨杉", 111] }, { a: [111, true] }, { a: 111 }, { a: 2022.0211 }, { a: true }, { a: { $date: "2022-02-11T15:59:59.999Z" } }, { a: { $timestamp: "2022-02-11T15:59:59.999Z" } }];
   dbcl.insert( docs );

   // $strlenCP作为匹配符使用
   var actResult = dbcl.find( { "a": { "$strlenBytes": 1, "$gte": 5 } } ).sort( { a: 1 } );
   var expResult = [{ "a": ["Sequoiadb", "巨杉", 111] }, { "a": "Sequoiadb" }, { "a": "巨杉" }, { "a": "巨杉Sequoiadb" }, { "a": "巨杉数据库" }];
   commCompareResults( actResult, expResult );

   // $strlenCP作为选择符使用
   var actResult = dbcl.find( {}, { "a": { "$strlenBytes": 1 } } ).sort( { a: 1 } );
   var expResult = [{ "a": null }, { "a": null }, { "a": null }, { "a": null }, { "a": null }, { "a": [null, null] }, { "a": [9, 6, null] }, { "a": 6 }, { "a": 9 }, { "a": 15 }, { "a": 15 }];
   commCompareResults( actResult, expResult );
}