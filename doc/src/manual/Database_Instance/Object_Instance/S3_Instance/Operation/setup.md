
本章介绍 SequoiaS3 的安装、配置、启动与停止。

安装
----

SequoiaS3 集成于 SequoiaDB 巨杉数据库的安装包中。SequoiaDB 安装完成后，用户可到安装路径下的 `tools/sequoias3` 目录查看相关组件。


配置SequoiaDB
----

SequoiaS3 对接的 SequoiaDB 需开启 RC 级别事务，且配置为等锁模式

```
> var db = new Sdb( "localhost", 11810 )
> db.updateConf( { transactionon:true, transisolation:1, translockwait:true} )
```

配置SequoiaS3
----

1. 切换至安装目录下的 `tools/sequoias3` 目录

   ```lang-bash
   $ cd tools/sequoias3
   ```

2. 打开 `config` 目录中的 `application.properties` 文件

   ```lang-bash
   $ vi config/application.properties
   ```

3. 修改文件中的如下配置：

 配置对外监听端口为 8002

   ```lang-ini
   server.port=8002
   ```

 配置 coord 节点的 IP 和端口，可以配置多组并使用逗号分隔

   ```lang-ini
   sdbs3.sequoiadb.url=sequoiadb://192.168.20.37:11810,192.168.20.38:11810
   ```

 如果在 SequoiaDB 中已经为 SequoiaS3 的存储创建了专属的域，需在此处配置

   ```lang-ini
   sdbs3.sequoiadb.meta.domain=domain1
   sdbs3.sequoiadb.data.domain=domain2
   ```

 > **Note:**
 >
 > 上述配置是启动 SequoiaS3 的基础配置，其他配置可参考本章末尾的配置说明。

启动与停止
----

配置修改完成后，通过 `./sequoias3.sh` 可执行脚本启动 SequoiaS3

```shell
$ ./sequoias3.sh start
```

如需停止 SequoiaS3 进程，可执行 `stop -p {port}` 停止监听指定端口的 SequoiaS3 进程，或执行 `stop -a` 停止所有 SequoiaS3 进程

```shell
$ ./sequoias3.sh stop -p 8002
```

配置说明
----

### 基础配置

|参数|配置说明|
|----|----------|
|server.port | SequoiaS3 监听端口号|
|sdbs3.sequoiadb.url | SequoiaS3 所对接 SequoiaDB 的 coord 节点 IP 和端口，以 sequoiadb://为前缀，多组之间使用逗号分隔  <br/>例如：sdbs3.sequoiadb.url=sequoiadb://sdbserver1:11810,sdbserver2:11810,sdbserver3:11810  </br> 默认值为：sdbs3.sequoiadb.url=sequoiadb://localhost:11810|
|sdbs3.sequoiadb.auth | SequoiaS3 对接的 SequoiaDB 用户名密码，如果 SequoiaDB 未配置密码，则此处不需要配置 |
|sdbs3.sequoiadb.meta.csName  | SequoiaS3 存储元数据的集合空间名称，默认为 S3_SYS_Meta；系统启动时如果检测到没有此集合空间，则会自动创建|
|sdbs3.sequoiadb.meta.domain  | SequoiaS3 存储元数据的集合空间所在域，只在初次启动系统时生效 |
|sdbs3.sequoiadb.data.csName  | SequoiaS3 存储对象数据的集合空间名称前缀，默认为 S3_SYS_Data，系统会随着上传对象时的年份变化创建不同的集合空间  <br> 例如：2019 年上传的对象会存储在名为 S3_SYS_Data_2019 的集合空间中，上传对象数据时如果没有对应的集合空间，系统会自动创建|
|sdbs3.sequoiadb.data.domain  | SequoiaS3 存储对象数据的集合空间所在域 |
|sdbs3.sequoiadb.data.csRange  | SequoiaS3 在同一时间段可以创建的存储对象数据的集合空间数量 |
|sdbs3.sequoiadb.data.lobPageSize  |SequoiaS3 存储对象数据的集合空间的 lobPageSize |
|sdbs3.sequoiadb.data.replSize  |SequoiaS3 存储对象数据的集合空间内集合的 replSize |

### SequoiaS3 与 SequoiaDB 之间的连接池配置

|参数|配置说明|
|----|----------|
|sdbs3.sequoiadb.maxConnectionNum | SequoiaS3 会建立与 SequoiaDB 数据库的连接池，该参数指定连接池内最大连接数量 |
|sdbs3.sequoiadb.maxIdleNum | 连接池最大空闲连接数量，也是系统初始建立的连接数量 |
|sdbs3.sequoiadb.deltaIncCount | 连接池单次增加连接的数量 |
|sdbs3.sequoiadb.keepAliveTime | 连接池中空闲连接存活时间，单位为毫秒，0 表示不关心连接隔多长时间没有收发消息 |
|sdbs3.sequoiadb.CheckInterval | 连接池检测空闲连接的周期，单位为毫秒，将超过 maxIdleNum 的空闲连接关闭|
|sdbs3.sequoiadb.validateConnection | 使用连接前先检查该连接是否可用 |

### 桶配置

|参数|配置说明|
|----|----------|
|sdbs3.bucket.limit  |每位用户允许创建存储桶的最大数量，默认为 100 个 |
|sdbs3.bucket.allowreput  |是否允许重复创建同名存储桶而不报错 |

### 分段上传配置

|参数      |配置说明  |
|--------------|----------|
|sdbs3.multipartupload.partlistinuse  | 是否使用 Complete Multipart Upload 请求中携带的分段列表进行合并，如果该配置为  true，则根据请求携带的分段列表中指定的分段进行合并；如果该配置为 false，则根据系统中已经收到的所有分段按分段编码顺序进行合并，不使用请求中的分段列表，也不检查请求中的分段列表的内容有效性 |
|sdbs3.multipartupload.partsizelimit  | 合并分段时是否检查分段的大小，当配置为 true 时，除最后一个分段外，其他分段必须处于 5M~5G 的范围内，超出范围则合并失败（该参数在 partlistinuse 配置为 true 时生效） |
|sdbs3.multipartupload.incompletelifecycle  | 已初始化未完成的分段上传请求保留天数，默认配置为三天；当一个分段上传请求初始化三天后仍未完成，则清理该请求和已上传的分段 |

### 鉴权配置

|参数|配置说明|
|------|----------|
|sdbs3.authorization.check  |是否对用户进行鉴权，如果配置为 false，则对所有访问用户都不做合法性检查，所有用户对系统进行访问都按照默认系统用户拥有最大权限进行访问 |

### 查询上下文配置

|参数      |配置说明  |
|--------------|----------|
|sdbs3.context.lifecycle  |查询对象列表的上下文保存周期，单位为分钟 <br> 查询对象列表时，如果有未查完的记录，系统记录上下文，并返回上下文的 token，等待下一次查询；查询完成后清理上下文，如超时未收到下次查询，清理上下文|
|sdbs3.context.cron  |上下文过期清理检测周期，格式为 cron                                                                                                                                                                                                                                                                                                                                                                                                                                  |
