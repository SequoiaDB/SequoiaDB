[^_^]:
    节点数据一致性检测工具
    作者：赵玉静
    时间：20190308
    评审意见
    王涛：
    许建辉：
    市场部：20190521


sdbinspect 是 SequoiaDB 巨杉数据库的数据一致性检测工具。

SequoiaDB 支持集合级别的一致性配置。当集合被配置为[强一致][architecture]时，该集合的所有数据副本保持完全一致；当集合被配置为最终一致时，其主备节点之间的数据由于异步复制的原因可能会存在短暂的差别。sdbinspect 工具可以被用于检测多个副本之间数据的一致性并生成详细的检测报告。

>**Note:**
>
> 当用户使用该工具检测一致性时，系统会进行大量的表扫描进行比对，产生大量 IO，对系统性能有一定影响。为了不影响系统正常运行，建议在系统不繁忙时使用该工具进行检测。

运行需求
----

sdbinspect 需要接入协调节点。

语法规则
----

```lang-text
sdbinspect [--coord | -d arg] [--output | -o arg]
sdbinspect [--coord | -d arg] [--output | -o arg] [--collectionspace | -c arg] [--view | -w arg] [--loop | -t arg]
```

参数说明
----

- **--help, -h**  

 返回帮助信息

- **--auth, -u**
    
 - 指定数据库鉴权需要的用户名和密码，格式为：username:password，默认值为"":""
 - 只提供用户名不输入密码时，工具会通过交互式界面提示用户输入密码，格式为：username

- **--version, -v**  

 返回工具版本及所属 SequoiaDB 版本

- **--action, -a**  

  指定检查数据的方式，分为 inspect 和 report，如果不指定则默认为 inspect 方式
  - inspect 方式：根据指定参数进行检测，生成中间文件后生成检查报告
  - report 方式：通过 file 参数指定已存在的中间文件，读取中间文件的内容生成检查报告

- **--coord, -d**  

 指定协调节点的主机名和服务端口，格式为 hostname:servicename，默认值为 `localhost:11810`

- **--loop, -t**

 - 指定检查的最大尝试次数，默认为 5 次
 - 当检查到节点间数据有不一致时，会尝试再次检查，直到检查到节点间数据一致或检测次数达到最大尝试次数为止

- **--group, -g**  

 指定要检查的复制组（group）名称，如果不指定则检查所有复制组

- **--collectionspace, -c**  

 指定检查的集合空间名称，如果不指定则检查所有集合空间

- **--collection, -l**  

 指定检查的集合名称，如果不指定则检查所有集合

- **--file, -f**
  
 - 指定中间文件，该中间文件应为 sdbinspect 生成的二进制中间文件
 - 当 action 参数指定检查方式为 inspect 时，本参数指定的文件中的检查选项会覆盖本次查询命令行携带的检查选项（-o 除外）
 - 当 action 参数指定检查方式为 report 时，本参数指定的文件中的检查结果用于生成检查报告

- **--output, -o**

 - 指定输出的文件名，默认名称为 `inspect.bin`
 - 当 action 参数指定检查方式为 inspect 时，本参数指定的是中间文件的名称，结果中会生成一个本参数指定的二进制中间文件，并生成一个以本参数为名并以“.report”为后缀的检查报告
 - 当 action 参数指定检查方式为 report 时，本参数指定的是最终检查报告的文件名称

- **--view, -w**  

 指定生成检查报告中的内容按照复制组（group）或集合（collection）查看，默认为组 (group)

- **--token**  

 指定密文文件的加密令牌

 >**Note:**
 >
 > 如果创建密文文件时未指定 token，可忽略该参数.

- **--cipher**  

 指定是否使用加密文件输入密码，默认为 false，不使用密文模式输入密码；关于密文模式的介绍，详细可参考[密码管理][system_security]

- **--cipherfile**   

 指定加密文件，默认为 `~/sequoiadb/passwd`

示例
----

* sdbinspect 检查协调节点 `localhost:12900` 下的全部集群，将中间文件输出到 `item.bin` 中，解析 `item.bin` 文件，按照组（group）划分，将文本结果输出到 `item.bin.report` 文件，同时输出总的检查结果

   ```lang-bash
   $ sdbinspect -d localhost:12900 -o item.bin
   ```

   执行结果如下：

   ```lang-text
   inspect done
   Inspect result:
   Total inspected group count       : 3
   Total inspected collection        : 20
   Total different collections count : 0
   Total different records count     : 0
   Total time cost                   : 37 ms
   Reason for exit : exit with no records different
   ```

* sdbinspect 检查协调节点 `localhost:12900` 下的全部集群中的集合空间 example（3次），并将中间文件结果输出到 `item.bin` 中。同时会解析 `item.bin` 文件，把文本结果按 collection 划分，输出到 `item.bin.report` 文件中。

   ```lang-bash
   $ sdbinspect -d localhost:12900 -o item.bin -c example -w collection -t 3
   ```

   执行结果如下：

   ```lang-text
   inspect done
   Inspect result:
   Total inspected group count       : 3
   Total inspected collection        : 13
   Total different collections count : 0
   Total different records count     : 0
   Total time cost                   : 33 ms
   Reson for exit : exit with no records different
   ```

* sdbinspect 检查协调节点 `localhost:12900` 下复制组 dg1 中的集合 sample.employee（5次），并将中间文件结果输出到 `inspect.bin` 中。同时会解析 `inspect.bin` 文件，把文本结果按（默认的）group 划分，输出到 `inspect.bin.report` 文件中

   ```lang-bash
   $ sdbinspect -d localhost:12900 -g dg1 -c sample -l employee
   ```

   执行结果如下：

   ```lang-text
   inspect done
   Inspect result:
   Total inspected group count       : 1
   Total inspected collection        : 1
   Total different collections count : 0
   Total different records count     : 0
   Total time cost                   : 6 ms
   Reason for exit : exit with no records different
   ```

检查报告解析
----

用户执行检查命令时界面输出的内容是每个组的分析结果，集合空间和集合的分析结果需要查看检查报告。

1. 打开检查报告，用户可以看到工具名称和版本

   ```lang-text
   Tool Name    : sdbInspect
   Tool Version : 0.1
   ```

2. 查看本次检查使用的参数

   ```lang-text
   Parameters:
   Loop        : 5
   action      : inspect
   coorAddress : localhost
   serviceName : 12900
   username    : ""
   group       : dg1
   cs name     : sample
   cl name     : employee
   file path   : 
   output file : inspect.bin
   view        : group
   ```

3. 用户可以按组或按集合显示的详细查询结果

   - **按组（group）查看的检查结果解析**

     每组结果以 Replica Group 开始，依次显示该组的 ID（ID）、组名（Name）、组内节点数量（count）、组内每个节点的信息、组内各个集合信息及集合中数据不一致的记录。

     如下报告显示 dg1 组内的集合 sample.employee 和  sample.dept 均无不一致的记录：
  
     ```lang-text
       Replica Group:
       Group ID     : 1000
       Group Name   : dg1
       Nodes count  : 3
         Node index       : 1
         Node ID          : 1002
         Node HostName    : hostname1
         Node ServiceName : 13920
         Node State       : Normal
     
         Node index       : 2
         Node ID          : 1000
         Node HostName    : hostname1
         Node ServiceName : 13900
         Node State       : Normal
     
         Node index       : 3
         Node ID          : 1001
         Node HostName    : hostname1
         Node ServiceName : 13910
         Node State       : Normal
     
       Collection Full Name  : employee.employee
       Main Collection Name  : None
         There is no record different
     
       Collection Full Name  : employee.dept
       Main Collection Name  : None
         There is no record different
     ```
  
   - **按集合（collection）查看的检查结果分析**
  
     每组结果以 Collection Full Name 开始，显示集合全称和主表名称。然后在下面列出集合所在的组（group）、该组的 ID
  （ID）、组名（Name）、组内节点数量（count）及组内每个节点的信息。
  
     当集合检查到有不一致的记录时，用 `-record` 显示节点间不一致的记录，并在每一条记录下用 `-Node State` 显示各节点的记录状态，记录按照 Node index 的序列号大小升序显示，状态 0 表示没有记录，状态 1 表示有记录。
  
      如下示例中显示 sample.employee 集合中有一条记录在 dg1 组内三个数据节点间不一致。“-Node State :  0 1 0”表示在 Node index 为 1 的节点没有该记录，在 Node index 为 2 的节点中有该记录，在 Node index 为 3 的节点没有该记录：
  
     ```lang-text
       Collection Full Name  : sample.employee
       Main Collection Name  : None
     
       Replica Group:
       Group ID     : 1000
       Group Name   : dg1
       Nodes count  : 3
         Node index       : 1
         Node ID          : 1002
         Node HostName    : hostname1
         Node ServiceName : 13920
         Node State       : Normal
     
         Node index       : 2
         Node ID          : 1000
         Node HostName    : hostname1
         Node ServiceName : 13900
         Node State       : Normal
     
         Node index       : 3
         Node ID          : 1001
         Node HostName    : hostname1
         Node ServiceName : 13910
         Node State       : Normal
     
       # Node state 1 means node has the record, or 0 means not, and x means node invliad
       # The order is ascended by node index.
         There is [1] piece of records that haven't been synchronized.
     
       -record     : { "_id": { "$oid": "5c870d6784942ca9812d9455" }, "User": "test", "age": 10 }
       -Node State :  0 1 0
       ```

4. 显示检查结果总结

  总结中显示检查的组（group）数量、集合（collection）数量、不一致的集合数量、不一致的记录数量、总耗时以及最终结束检查的原因。结束检查的原因有两种：一种是检查数据节点间的记录全部一致，结束检查返回报告；另一种是检查数据节点间有不一致的记录，且尝试 loop 参数指定的最大次数后仍然不一致，结束检查返回报告。

   - 检查各数据节点间记录完全一致

     ```lang-text
     Inspect result:
     Total inspected group count       : 1
     Total inspected collection        : 1
     Total different collections count : 0
     Total different records count     : 0
     Total time cost                   : 6 ms
     Reason for exit : exit with no records different
     ```

    - 尝试最大次数后仍检查到节点间有不一致记录的结果
     
     ```lang-text
     Inspect result:
     Total inspected group count       : 1
     Total inspected collection        : 1
     Total different collections count : 1
     Total different records count     : 1
     Total time cost                   : 15 ms
     Reason for exit : loop is limited
     ```

[^_^]:
     本文使用到的所有连接及引用。
    
[architecture]:manual/Distributed_Engine/Architecture/Replication/architecture.md#节点一致性
[system_security]:manual/Distributed_Engine/Maintainance/Security/system_security.md#密码管理