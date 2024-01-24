[^_^]:
    索引升级工具

sdbupgradeidx 是 SequoiaDB 巨杉数据库的索引升级工具，用于在编目节点上添加索引的元数据信息，并生成 UniqueID。

在 SequoiaDB v3.6/5.0.3 及以上版本创建索引时，编目节点上会添加索引的元数据信息并生成 UniqueID。因此，用户将 SequoiaDB 升级至 v3.6/5.0.3 及以上版本后，需手动执行 sdbupgradeidx 工具以升级索引。

##语法规则##

```lang-text
sdbupgradeidx [ options ] ...
```

##参数说明##

- **--help, -h**  

    获取帮助信息

- **--version**  

    获取版本信息

- **--hostname, -s \<coord hostname\>**  

    指定协调节点所在的主机名
  
- **--svcname, -p \<coord port\>**  
  
    指定协调节点的端口号
    
- **--username, -u \<user name\>**  

    指定用户名，默认为空字符串

- **--password, -w \<password\>**  

    指定用户密码，默认为空字符串

- **--cipher \<boolean\>**  

    是否使用密文模式输入密码，默认为false，取值如下：

    - "true": 使用密文模式输入密码，配合 --cipherfile --token 参数使用，关于密文模式的介绍可参考[密码管理][passwd]

    - "false": 使用明文模式 --password 输入密码

- **--cipherfile \<cipher file\>**  

    指定密文文件路径，默认值为 ~/sequoiadb/passwd

- **--token \<token\>**  

    指定密文文件的加密令牌

    如果创建密文文件时未指定 token，可忽略该参数

- **--output, -o \<output file\>**  

    指定输出报告的文件路径，默认输出在当前路径下的 `sdbupgradeidx.log` 文件中
  
- **--action \<action\>**  

    指定操作，取值如下：
 
    - "check": 只做升级前的检验
 
    - "upgrade": 先做检验，校验通过后执行升级索引

##示例##

下述以协调节点 `localhost:11810`，工具所在路径 `/opt/sequoiadb/tools/upgrade` 为例，演示索引升级步骤。

1. 检验索引信息

    ```lang-bash
    $ ./sdbupgradeidx -s localhost -p 11810 --action check    
    Generate report: ./sdbupgradeidx.log 
    ```

2. 查看检验报告，获取当前集群的索引情况

    ```lang-bash
    $ vi sdbupgradeidx.log
    ```

3. 执行升级索引

    ```lang-bash
    $ ./sdbupgradeidx -s localhost -p 11810 --action upgrade
    Generate report: ./sdbupgradeidx.log 
    ```

4. 查看升级报告，确认升级结果

    ```lang-bash
    $ vi sdbupgradeidx.log
    ```

##报告解析##

###查看不需要升级的索引###

“No Need to Upgrade”表示索引均已存在合法的 UniqueID，无需升级。

```lang-text
===================== Check Result ( No Need to Upgrade ) ======================
 ID      Collection      IndexName :    IndexType
  1  sample.employee           $id :   Consistent
  2  sample.employee       nameIdx :   Consistent
```

IndexType 取值如下：

* Consistent：一致性索引，表示在编目节点和各数据节点上均有索引 UniqueID
* Standalone：[独立索引][standalone]

###查看可升级的索引###

“Can be Upgraded”表示索引在集合的各数据节点上定义一致，但缺失 UniqueID，需要升级

```lang-text
===================== Check Result ( Can be Upgraded ) =========================
 ID           Collection      IndexName :      IndexKey                IndexAttr
  1       sample.January            $id :     {"_id":1} Unique|Enforced|NotArray
  2       sample.January        nameIdx :    {"name":1}                        -
  3       sample.January         ageIdx :     {"age":1}                   Unique
```

###查看不可升级的索引###

“Cannot be Upgraded”表示索引均不可升级，具体不可升级的原因及解决办法可参考后续说明。

```lang-text
===================== Check Result ( Cannot be Upgraded ) ======================
 ID           Collection      IndexName :      IndexKey                IndexAttr      Reason
  1      sample.February        nameIdx :    {"name":1}                        -     Missing
  2      sample.February         ageIdx :     {"age":1}                        -    Conflict
  2      sample.February         ageIdx :    {"age1":1}                        -    Conflict
  4           sample.May            $id :     {"_id":1} Unique|Enforced|NotArray    Local CL

  ---------- Index ( ID: 1 ) Missing -----------
   GroupName             NodeName  Missing
      group2     sdbserver1:11830        Y
      group2     sdbserver2:11830        Y
      group2     sdbserver3:11830        N

  ---------- Index ( ID: 2 ) Conflict -----------
   IndexName   IndexKey  IndexAttr  GroupName  NodeName
      ageIdx  {"age":1}          -     group2  sdbserver1:11830,sdbserver2:11830
      ageIdx {"age1":1}          -     group2  sdbserver3:11830

  ---------- Index ( ID: 3 ) Conflict -----------
   IndexName   IndexKey  IndexAttr  GroupName  NodeName
      ageIdx {"age1":1}          -     group2  sdbserver3:11830
      ageIdx  {"age":1}          -     group2  sdbserver1:11830,sdbserver2:11830

  ---------- Index ( ID: 4 ) Local CL -----------
   Collection             NodeName
   sample.May     sdbserver1:11830
```

- **索引缺失**

    “Missing”表示“Cannot be Upgraded”列表中 ID 为 1 的索引，存在于数据节点 `sdbserver3:11830`，但不存在于数据节点 `sdbserver1:11830` 和 `sdbserver2:11830`。
   
    ```lang-text
    ---------- Index ( ID: 1 ) Missing -----------
     GroupName             NodeName  Missing
        group2     sdbserver1:11830        Y
        group2     sdbserver2:11830        Y
        group2     sdbserver3:11830        N
    ```
     
    缺失的索引无法通过本工具升级，用户需要手工干预。

    解决办法：
     
    - 方案1：通过协调节点补充缺失的索引，使原索引升级为一致性索引
     
        ```lang-javascript
        > db.sample.February.createIndex('nameIdx', {name: 1})
        ```

    - 方案2：将已存在的索引转换为独立索引
     
        ```lang-javascript
        > db.sample.February.createIndex('nameIdx', {name: 1}, {Standalone: true}, {NodeName: "sdbserver3:11830"})
        ```

        通过 createIndex() 转换索引时，会输出如下信息，用户可忽略该提示：

        ```lang-text
        (shell):1 uncaught exception: -247
        Redefine index:
        The same index 'ageIdx' has been defined already
        ```

    > **Note:**
    > 
    > 具有约束性的索引（唯一索引、NotNull 索引等）无法升级为独立索引，需将其删除或升级为一致性索引。
   
- **索引冲突**

    “Conflict”表示“Cannot be Upgraded”列表中 ID 为 2 的索引与其他索引冲突。当集合名相同但索引键值或者属性不同，或当索引键值相同但索引名字或者属性不同时，均会导致索引冲突。
   
    ```lang-text
    ---------- Index ( ID: 2 ) Conflict -----------
     IndexName   IndexKey  IndexAttr  GroupName  NodeName
        ageIdx  {"age":1}          -     group2  sdbserver1:11830,sdbserver2:11830
        ageIdx {"age1":1}          -     group2  sdbserver3:11830
    ```
     
    冲突的索引无法通过本工具升级，用户需要手工干预。

    解决办法：

    - 方案1：在数据节点上删除冲突的索引后，重新创建目标索引，使原索引升级为一致性索引
     
        ```lang-javascript
        > data3 = new Sdb('sdbserver3:11830')
        > data3.sample.February.dropIndex('ageIdx')
        > db.sample.February.createIndex('ageIdx', {age: 1})
        ```
     
    - 方案2：将已存在的索引转换为独立索引
     
        ```lang-javascript
        > db.sample.February.createIndex('ageIdx', {age: 1}, {Standalone: true}, {NodeName: ["sdbserver1:11830", "sdbserver2:11830"]})
        > db.sample.February.createIndex('ageIdx', {age1: 1}, {Standalone: true}, {NodeName: "sdbserver3:11830"})
        ```

        通过 createIndex() 转换索引时，会输出如下信息，用户可忽略该提示：

        ```lang-text
        (shell):1 uncaught exception: -247
        Redefine index:
        The same index 'ageIdx' has been defined already
        ```

    > **Note:**
    > 
    >  具有约束性的索引（唯一索引、NotNull 索引等）无法升级为独立索引，需将其删除或升级为一致性索引。
     
- **本地残留集合上的索引**

    “Local CL”表示“Cannot be Upgraded”列表中 ID 为 4 的索引存在于节点 `sdbserver1:11830` 的残留集合 sample.May 中。
   
    ```lang-text
    ---------- Index ( ID: 4 ) Local CL -----------
     Collection             NodeName
     sample.May     sdbserver1:11830
    ```

    本地残留集合上的索引无法通过本工具升级，用户需要手工干预。

    解决办法：

    确认该集合是否为无用的残留集合，如果是，可删除该集合。

###查看执行升级的索引###

只有标识为“Can be Upgraded”的索引才可以执行升级。

```lang-text
===================== Upgrade Index ============================================
 ID           Collection      IndexName :     Result      ResultCode
  1       sample.January            $id :    Succeed
  2       sample.January        nameIdx :    Succeed
  2       sample.January         ageIdx :     Failed      -134
```

索引升级时可能会受到其他业务操作、环境问题（如网络问题）的影响，出现少量索引升级失败。此时请根据 ResultCode 信息（如上述 -134）与 SequoiaDB 集群的日志信息排查问题，然后重新执行索引升级。

###查看报告总结###

统计无需升级的索引个数、无法升级的索引个数、可升级的索引个数、升级失败的索引个数、升级成功的索引个数

```lang-text
++++++++++++++++++++++++++
No need to upgrade : 2
Cannot be upgraded : 3
   Can be upgraded : 3
 Failed to Upgrade : 1
Succeed to Upgrade : 2
++++++++++++++++++++++++++
```


[^_^]:
    本文使用的所有引用及链接
[standalone]:manual/Distributed_Engine/Architecture/Data_Model/index.md#创建索引
[passwd]:manual/Distributed_Engine/Maintainance/Security/system_security.md#密码管理
