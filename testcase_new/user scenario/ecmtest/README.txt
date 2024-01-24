1. recommend using 3 groups and 3 nodes environment.
2. please open transaction before test.
3.先配置一下属性文件(prop.ini)，用于指定运行过程中的参数
coordUrl=192.168.30.56:11810 // coord地址
usr=                         // 用户名
pwd=                         // 密码
connectionTimeOut = 500      // 连接最大超时时长(ms)
maxAutoConnectRetryTime = 0  // 连接最大重试次数
maxCount = 500               // 连接池最大连接数量
deltaIncCount = 20           // 连接池增幅
maxIdleCount = 20            // 连接池最大闲置连接数量
keepAliveTimeout = 0         // 
syncCoordInterval=0          // 同步coord地址的时隔
checkInterval = 60000        // 检查无效连接的间隔
threadCount = 10             // 线程数
testDataDir=D://testdir//    // 上传文件的目录
writeTranCountPerMins=150    // 每分钟写事务数
readTranCountPerMins = 100   // 每分中读事务数
maxWriteTranCountPerMins = 330  // 每分钟最大写事务数
4.`./startEcmTest.sh`启动测试
`./stopEcmTest.sh`停止测试
`./listEcmTest.sh`查看测试进程
`./showTestStat.sh`查看当前测试结果
