系统配置
启动SequoiaS3系统之前需进行如下配置：
1.安装sdb数据库，配套版本3.0.0及后续版本
  数据节点下配置transactionon=true, transisolation=1, translockwait=true  并重启数据节点
2.修改jar包同级的config目录中application.properties配置文件
  2.1修改sdbs3.sequoiadb.url中的数据库IP地址和coord端口，与安装数据库的协调节点IP、端口保
  持一致。
  2.2修改元数据的集合空间名称和对象数据的集合空间，如果使用默认值则不需要修改
  2.3修改服务端口号，此端口号为客户端连接sequoias3服务的端口号
  （可参考注释样例）

系统启动
配置修改完成后,执行jar包所在目录的启动脚本"./sequoias3.sh start"启动S3服务

系统使用：
1.系统启动时需确保数据库已经正常运行，系统启动后会创建默认管理员用户，
您可以使用该默认管理员用户创建新的用户，然后使用新用户角色进行桶操作，对象操作。
默认管理员账户名：administrator
默认管理员AccessKeyID：ABCDEFGHIJKLMNOPQRST
默认管理员用户SecreatKeyID：abcdefghijklmnopqrstuvwxyz0123456789ABCD

错误排查：
1.如果系统启动失败
  a.首先排查监听端口号是否被占用，修改端口号后重新启动验证。
  b.非端口号问题，再检查application.properties中sdbs3.sequoiadb.url的IP、端口配置是否与数据库配置相匹配，修改后重新启动验证
  d.打开nohup.out，检查错误日志，排查错误后重新启动验证。



