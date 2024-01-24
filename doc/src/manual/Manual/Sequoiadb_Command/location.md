命令位置参数用于控制命令执行的位置信息，包括命令是否在全局运行还是在本地运行、命令运行的复制组、命令运行的节点、节点的选取方式、命令运行的角色等。该参数以 Json 对象作为命令的参数传入，并且只在协调节点生效。具体可指定的参数如下：

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ------   | ------ | ------ |
| Global | boolean    | 是否在全局执行 | 否 |
| GroupID | number/array | 分区组 ID | 否 |
| GroupName | string/array | 分区组名 | 否 |
| NodeID | number/array | 节点 ID | 否 |
| HostName | string/array | 节点的主机名称 | 否 |
| ServiceName | string/array | 节点的服务名 | 否 |
| svcname | string/array | 节点的服务名 | 否 |
| NodeName | string/array | 节点名称，格式为 \<HostName\>:\<svcname1\>[:svcname2...] ,<br> 如：sdbserver:11820:11830 | 否 |
| NodeSelect | string | 在未指定节点时分区组的节点选择策略，取值如下：<br>  "all"：选择该组所有节点<br> "master"/"primary"：选择该组主节点<br> "any"：选择该组任意节点<br> "secondary"：选择该组任意备节点  | 否 |
| Role | string/array | 指定命令运行的节点角色，取值如下：<br>  "data"：数据节点<br>  "catalog"：编目节点<br>  "coord"：协调节点<br>  "all"：所有节点 | 否 |
| RawData | boolean | 是否返回原始数据，仅对 [list](manual/Manual/Sequoiadb_Command/Sdb/list.md) 或 [snapshot](manual/Manual/Sequoiadb_Command/Sdb/snapshot.md) 命令生效，<br>为 true 则返回各节点的原始数据，不在协调节点进行聚集处理 | 否 |
| InstanceID | number/array | 节点的实例 ID（数据节点通过的配置项 instanceid 指定）<br>有效取值范围：1 - 255 <br>指定 InstanceID 时仅选取数据节点 | 否 |
|Force|boolean|是否强制写入，仅对 [updateConf](manual/Manual/Sequoiadb_Command/Sdb/updateConf.md) 命令生效，在需要配置非官方配置项时指定为 true，默认值为 false|否
> **Note:**
>
> * 当设置了GroupID、GroupName、NodeID、HostName、ServiceName 或 NodeName 时，Global 取值被忽略，在指定的复制组或节点上执行。
> * GroupID、GroupName：指定复制组过滤条件，缺省指所有复制组；GroupID 和 GroupName 为或的关系，如：`{GroupID:1001, GroupName:"db1"}`，那么复制组 1001 和 db1 都是执行的复制组。
> * NodeID、HostName、ServiceName、NodeName：指定复制组中节点过滤条件，对于查询命令，缺省值为该组所有节点，对于操作命令，缺省值为该组主节点。上述字段为与的关系，如 `{NodeID:1001, ServiceName:'11810'}`，如果节点 1001 的ServiceName 不为 11810，则节点为空。
> * Groups：为了兼容之前的命令而保留，与 GroupName 作用相同，不推荐使用。
> * svcname：与 ServiceName 参数功能相同，都表示设置节点服务名。

