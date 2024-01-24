/******************************************************************************
*@Description : test sdblobtool export and import normal condition with SSL
                测试时需要修改COORDSVCNAME配置文档 /conf/local/...
                usessl = TRUE 并重启集群
*@Modify list :
*               2016-06-20   XueWang Liang  Init
******************************************************************************/
import("../lib/lobtool_export_import.js")

var testPara = { useSSL: true };
// Test
lobToolMain(main, testPara);
