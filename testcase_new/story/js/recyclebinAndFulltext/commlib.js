/******************************************************************************
 * @Description   : 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2022.08.23
 * @LastEditors   : liuli
 ******************************************************************************/
import( "../lib/recyclebin_commlib.js" );
import( "../lib/fulltext_commlib.js" );
testConf.testGroups = ["recycleBin"];
// 由于全文索引环境不稳定，暂时跳过全文索引相关用例
testConf.skipTest = true;
