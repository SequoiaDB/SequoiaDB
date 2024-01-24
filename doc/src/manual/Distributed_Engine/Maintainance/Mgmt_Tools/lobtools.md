[^_^]:
    大对象工具
    作者：赵育
    时间：20190305
    评审意见
    王涛：20190308
    许建辉：
    市场部：20190521
    

sdblobtool 是 SequoiaDB 巨杉数据库的[大对象][lob]管理工具，可以导出、导入和迁移大对象数据，应用于大对象在集合之间的迁移，运行日志保存在当前执行路径下的 `sdblobtool.log` 文件中。

参数说明
----

| 参数名          | 缩写 | 描述                                                                   |
| ----            | ---- | ----                                                                   |
| --help          | -h   | 显示帮助信息                                                           |
| --version       | -v   | 显示版本号                                                             |
| --hostname      |      | 协调节点地址，默认值为 localhost                                       |
| --svcname       |      | 协调节点服务名，默认值为 11810                                         |
| --operation     |      | 操作类型必填，取值如下：<br> export：将集合中的大对象导出至文件 <br> import：将文件中的大对象导入至集合 <br> migration：将集合中的大对象复制到其他集合                                     |
| --collection    |      | 集合全名                                                               |
| --file          |      | 指定大对象文件的全路径，--operation 参数值为 export 或 import 该值有效 |
| --dstcollection |      | 目标数据库的集合全名，--operation 参数值为 migration 该值有效          |
| --dsthost       |      | 目标数据库的协调节点地址，--operation 参数值为 migration 该值有效      |
| --dstpasswd     |      | 目标数据库的密码，--operation 参数值为 migration 该值有效              |
| --dstservice    |      | 目标数据库的密码，--operation 参数值为 migration 该值有效              |
| --dstusrname    |      | 目标数据库的用户名，--operation 参数值为 migration 该值有效            |
| --ignorefe      |      | 是否导入目标集合中已存在的大对象，--operation 参数值为 import 或 migration 该值有效，默认值为 false，不导入目标集合中已存在的大对象                                                        |
| --prefer        |      | 优先选择的实例，--operation 参数值为 export 该值有效，默认值为 M；取值 m 或 M 指 master，s 或 S 表示 slave，a 或 A 表示 anyone，1~7 表示 node1~node7                                         |
| --ssl           |      | 是否使用 SSL 连接，默认值为 false，不使用 SSL 连接                     |
| --token         |      | 密文文件的加密令牌<br>如果创建密文文件时未指定 token，可忽略该参数       |
| --usrname       |      | 数据库用户名，数据库开启了鉴权，需指定该参数                           |
| --passwd        |      | 数据库密码                                                             |
| --cipher        |      | 是否使用密码文件的方式输入数据库密码，默认值为 false，不使用密码文件   | 
| --cipherfile    |      | 密码文件，当参数 --cipher 指定为 true 时，如果不指定该参数，则取 `~/sequoiadb/passwd` 文件为密码文件 |

示例
----

* 将集合 sample.employee 中的大对象导出至 `/opt/mylob` 文件中

   ```lang-bash
   $ ./sdblobtool --operation export --hostname localhost --svcname 11810 --collection sample.employee --file /opt/mylob
   ```

* 将 `/opt/mylob` 文件中的大对象导入到 sample.employee 集合中，当遇到集合已存在的大对象时直接跳过

   ```lang-bash
   $ ./sdblobtool --operation import --hostname localhost --svcname 11810 --collection sample.employee --file /opt/mylob --ignorefe
   ```
* 将 sdbserver1 的集合 sample.employee 中所储大对象复制到 sdbserver2 的集合 sample.employee 中

   ```lang-bash
   $ ./sdblobtool --operation migration --hostname sdbserver1 --svcname 11810 --collection sample.employee --dsthost sdbserver2 --dstservice 11810 --dstcollection sample.employee
   ```
  
   [^_^]:
    本文使用到的所有链接及引用。
[lob]:manual/Distributed_Engine/Architecture/Data_Model/lob.md
    
    
