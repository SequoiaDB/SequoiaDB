【介绍】
runomtest.sh是用来测试om svc的API的脚本。

【使用说明】
1. 在testcase_new\om\packet下，放入SequoiaDB、SequoiaSQL-PostgreSQL、SequoiaSQL-MySQL三个run包，企业版社区版都可以。
2. 准备1台或以上的Linux系统主机（没有安装SequoiaDB的，有的请先卸载）用来测试OM，需要在testcase_new\om\js创建一个testcase.conf的文件，
   文件格式如下例子：
   {
       "Host": [
           {
               "HostName": "h1",
               "IP": "192.168.20.151",
               "User": "root",
               "Passwd": "sequoiadb1"
           },
           {
               "HostName": "h2",
               "IP": "192.168.20.152",
               "User": "root",
               "Passwd": "sequoiadb2"
           },
           {
               "HostName": "h3",
               "IP": "192.168.20.153",
               "User": "root",
               "Passwd": "sequoiadb3"
           }
       ]
   }

   HostName：主机名；IP：该主机的IP地址；User：管理员用户名，现在必须要root；Passwd：管理员密码；

3. 执行runomtest.sh，命令行执行示例：./runomtest.sh -hn 192.168.20.151 -sp sequoiadb1
   runomtest.sh可以执行 ./runomtest.sh --help来查看参数
