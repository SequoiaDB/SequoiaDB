<?php
//部署的节点配置
$globalvar_setup_conf = array(
//     配置参数名			网页输入类型		列表参数(select用)				中文描述																													应用域		默认值
//array( "installpath",	"textarea",		"",								"数据库安装路径", 																										"all",	"/opt/sequoiadb", ),
//array( "sysgroup",		"input",			"",								"系统用户组", 																											"all",	"sdbadmin_group", ),
//array( "sysuser", 		"input",			"",								"系统用户名", 																											"all",	"sdbadmin", ),
//array( "syspassword", 	"input",			"",								"系统密码", 																												"all",	"sequoiadb", ),
//array( "groupname", 		"input",			"",								"所属分区组", 																											"",		"", ),

array( "confpath", 		"",				"",								"配置文件路径", 																											"",		"", ),
array( "logpath", 		"textarea",		"",								"同步日志存储路径，默认路径为：数据文件路径/replicalog", 																										"all",	"", ),
array( "diagpath", 		"textarea",		"",								"诊断日志存储路径，默认路径为：数据文件路径/diaglog", 																										"all",	"", ),
array( "dbpath", 			"textarea",		"",								"数据存储路径", 																											"all",	"/opt/sequoiadb/database/", ),
array( "indexpath", 		"textarea",		"",								"索引文件存储路径，默认路径为：数据文件路径", 																										"all",	"", ),
array( "bkuppath", 		"textarea",		"",								"备份文件存储路径，默认路径为：数据文件路径/bakfile", 																										"all",	"", ),
array( "maxpool", 		"input",			"",								"线程池的线程数量", 																										"all",	"0", ),
array( "svcname" , 		"input",			"",								"本地服务端口", 																											"all",	"11810", ),
array( "replname", 		"input",			"",								"数据同步端口，默认为本地服务端口 + 1", 																											"all",	"", ),
array( "shardname", 		"input",			"",								"分片通讯端口，默认为本地服务端口 + 2", 																											"all",	"", ),
array( "catalogname", 	"input",			"",								"编目通讯端口，默认为本地服务端口 + 3", 																											"all",	"", ),
array( "httpname", 		"input",			"",								"REST服务端口，默认为本地服务端口 + 4", 																											"all",	"", ),
array( "diaglevel", 		"input",			"",								"日志级别，范围：[ 0, 5 ]", 																							"all",	"3", ),
array( "role", 			"",				"",								"角色", 																													"",		"", ),
array( "catalogaddr",	"",				"",								"编目节点地址",																											"",		"",  ),
array( "logfilesz", 		"input",			"",								"指定同步日志文件大小( MB )，范围：[ 64, 2048 ]", 																	"all",	"64", ),
array( "logfilenum", 	"input",			"",								"指定同步日志文件数量，范围：[ 1, 60000 ]", 																			"all",	"20", ),
array( "transactionon", "select",		array("true","false"),		"是否打开事务", 																											"all",	"false", ),
array( "numpreload", 	"input",			"",								"页面预加载代理数，范围：[ 0, 100 ]", 																				"all",	"0", ),
array( "maxprefpool", 	"input",			"",								"数据预取代理池最大数，范围：[ 0, 1000 ]", 																			"all",	"200", ),
array( "maxsubquery", 	"input",			"",								"查询任务的最大分解子查询数，范围：[ 0, 1000 ]，最大值不能超过数据预取代理池最大数", 								"all",	"10", ),
array( "logbuffsize", 	"input",			"",								"复制日志内存页面数，范围：[ 512, 1024000 ]，每个页面大小为64KB，页面内存总大小不能超过同步日志文件大小", 	"all",	"1024", ),
array( "tmppath", 		"input",			"",								"临时文件存储路径", 																										"all",	"", ),
array( "sortbuf", 		"input",			"",								"排序缓存大小", 																											"all",	"512", ),
array( "hjbuf", 			"input",			"",								"哈希join缓存大小", 																										"all",	"128", ),
array( "syncstrategy",  "input",       array("none","keepnormal","keepall"),"副本组之间数据同步控制策略,取值:none,keepnormal,keepall,默认为none。",               "all",   "none", ),
array( "preferedreplica","input",      array("M","S","A","1","2","3","4","5","6","7"), "读请求优先选择的副本,默认值:A,取值列表:M,S,A,1-7",                        "all",   "A", ),
) ;

?>