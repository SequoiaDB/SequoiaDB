[^_^]:
    基于 Location 的容灾工具

用户可以通过安装路径 `tools/dr_location` 下的脚本查看、检查并初始化 SequoiaDB 巨杉数据库集群的 Location 信息。该工具基于集群级 `SdbDC` 接口，适用于已经采用 Location 机制规划多数据中心、同城双中心或两地多中心容灾架构的集群。

基于 Location 的容灾工具
----

基于 Location 的容灾工具用于辅助用户完成以下操作：

- 查看当前集群中各主机、节点的 Location 分布
- 查看当前集群的 ActiveLocation 和 GroupMode 状态
- 识别未设置 Location、主机内 Location 不一致、主节点不在 ActiveLocation 等异常情况
- 根据配置文件或工具生成的 Location 文件检查当前集群配置差异
- 根据配置文件或 Location 文件批量设置主机 Location 和集群 ActiveLocation
- 按 Domain 范围初始化 Location / ActiveLocation，或按 Domain 过滤 `show` 展示结果
- 按 Location、主机或 Domain 开启/关闭 MaintenanceMode 和 CriticalMode
- 检查集群节点状态，并在集群恢复正常后关闭全部 GroupMode
- 根据 `reelectLevel` 配置执行切主分析和切主操作，使主节点分布符合当前运行状态

典型的应用场景可参考[高可用与容灾][HA_DR]章节。

## 场景选择速查

一线运维或客户在使用工具时，建议先根据当前集群状态选择对应场景，再参考后文的工具参数说明。

| 当前场景 | 推荐操作 | 主要命令 |
| -------- | -------- | -------- |
| 新部署集群尚未设置 Location | 初始化 Location 和 ActiveLocation | `sdb_init_location.sh show/check/init` |
| 已配置 Location，需要巡检 | 查看当前状态并检查是否与配置一致 | `sdb_init_location.sh show/check` |
| 扩容、迁移或 Location 规划调整 | 修改配置后检查差异，再执行初始化 | `sdb_init_location.sh check/init` |
| 仅需对部分 Domain 初始化或巡检 Location | 在 `initLocationObject` 中配置 `domains`/`domainMode` 后执行 | `sdb_init_location.sh init/show` |
| 计划维护或部分节点异常，但集群仍可正常服务 | 对目标 Location 或主机开启 MaintenanceMode | `sdb_start_maintenance.sh` |
| 中心故障、复制组无主或多数派不可用，需要容灾接管 | 对幸存 Location 或主机开启 CriticalMode | `sdb_start_critical.sh` |
| 故障节点已恢复，需要解除指定 GroupMode | 按 Location、主机或文件关闭 GroupMode | `sdb_stop_maintenance.sh`、`sdb_stop_critical.sh` |
| 整个集群已恢复正常，需要关闭所有 GroupMode | 检查所有节点正常后执行集群恢复 | `sdb_restore_cluster.sh` |

> **Note:**
>
> 如果无法确认开启 MaintenanceMode 或 CriticalMode 的范围，优先执行 `sdb_init_location.sh show` 查看当前 Location、主节点和 GroupMode 状态。CriticalMode 属于容灾接管操作，执行前应确认故障范围和数据一致性风险。

## 使用场景

本节按照常见运维流程说明工具使用方式。完整参数和文件格式说明可参考后文“工具与参数参考”。
用户在首次使用前需要先从模板复制一份配置文件.

```lang-bash
cp config/config.js.sample config/config.js
```

### 未初始化 Location 的集群

对于尚未设置 Location 的集群，用户可通过本工具根据规划批量初始化各主机的 Location，并设置 ActiveLocation。

操作流程如下：

1. 根据集群部署规划修改 `config/config.js`，配置各 Location 下包含的主机。

   ```lang-javascript
   initLocationObject = {
      "GuangZhou": [
         "host1",
         "host2"
      ],
      "ShenZhen": [
         "host3"
      ]
   } ;

   activeLocation = "GuangZhou" ;
   ```

2. 查看当前集群状态，确认是否存在未设置 Location 的主机或其它异常。

   ```lang-bash
   $ ./bin/sdb_init_location.sh show
   ```

3. 检查当前集群与配置文件中的期望 Location 是否一致。

   ```lang-bash
   $ ./bin/sdb_init_location.sh check
   ```

4. 确认检查结果符合预期后，初始化 Location。

   ```lang-bash
   $ ./bin/sdb_init_location.sh init
   ```

5. 初始化后再次执行 `show` 和 `check`，确认所有主机 Location、ActiveLocation 已符合预期。

   ```lang-bash
   $ ./bin/sdb_init_location.sh show
   $ ./bin/sdb_init_location.sh check
   ```

### 已初始化 Location 的集群

对于已经配置过 Location 的集群，用户可通过 `show` 和 `check` 定期检查当前集群 Location 是否与配置文件一致，也可以在扩容、迁移或规划调整后修改 Location 配置。

#### 检查已有配置

1. 查看当前集群 Location、ActiveLocation 和 GroupMode 状态。

   ```lang-bash
   $ ./bin/sdb_init_location.sh show
   ```

2. 对比当前集群状态与配置文件。

   ```lang-bash
   $ ./bin/sdb_init_location.sh check
   ```

   如果输出中没有差异，说明当前集群 Location 配置与配置文件一致。

#### 修改已有配置

1. 修改 `config/config.js` 中的 `initLocationObject` 或 `activeLocation`。

2. 执行 `check` 查看变更会带来的差异。

   ```lang-bash
   $ ./bin/sdb_init_location.sh check
   ```

3. 确认差异符合预期后执行 `init`，将配置应用到集群。

   ```lang-bash
   $ ./bin/sdb_init_location.sh init
   ```

4. 如果希望基于当前 `show` 的结果进行人工调整，可先生成 Location 文件，然后修改该文件并通过 `-f` 参数执行检查或初始化。

   ```lang-bash
   $ ./bin/sdb_init_location.sh show
   $ vi output/location.info
   $ ./bin/sdb_init_location.sh check -f output/location.info
   $ ./bin/sdb_init_location.sh init -f output/location.info
   ```

### 出现故障时开启合适的 GroupMode

当集群出现节点、主机或 Location 级别故障时，用户可根据故障范围选择开启 MaintenanceMode 或 CriticalMode。

- MaintenanceMode：适用于计划维护、单节点或部分节点故障等场景。处于 MaintenanceMode 的节点不参与选主和同步一致性计算。
- CriticalMode：适用于复制组不满足正常选主条件、需要在指定 Location 或主机范围内恢复服务的容灾接管场景。开启 CriticalMode 前应确认故障范围和数据一致性风险。

#### 开启 MaintenanceMode

如果某个 Location 或主机需要维护，或其中节点异常但集群仍可正常提供服务，可开启 MaintenanceMode。

```lang-bash
# 对 GuangZhou 开启 MaintenanceMode
$ ./bin/sdb_start_maintenance.sh -l GuangZhou

# 对 host1、host2 开启 MaintenanceMode
$ ./bin/sdb_start_maintenance.sh -H host1,host2

# 仅对 domain1 中 host1 涉及的数据组开启 MaintenanceMode
$ ./bin/sdb_start_maintenance.sh -H host1 -d domain1
```

#### 开启 CriticalMode

如果主中心故障、复制组无主或多数派不可用，需要由幸存 Location 或主机接管服务，可开启 CriticalMode。

```lang-bash
# 在 GuangZhou 范围内开启 CriticalMode
$ ./bin/sdb_start_critical.sh -l GuangZhou

# 在 host1、host2 范围内开启 CriticalMode
$ ./bin/sdb_start_critical.sh -H host1,host2

# 仅对 domain1 中 GuangZhou 涉及的数据组开启 CriticalMode
$ ./bin/sdb_start_critical.sh -l GuangZhou -d domain1
```

如果目标范围由文件维护，可使用 `-f` 参数：

```lang-bash
$ ./bin/sdb_start_maintenance.sh -f target.info
$ ./bin/sdb_start_critical.sh -f target.info
```

开启 GroupMode 后，建议执行 `show` 查看当前 GroupMode 运行状态和异常信息。

```lang-bash
$ ./bin/sdb_init_location.sh show
```

> **Note:**
>
> 开启 CriticalMode 时，如果配置项 `enforce` 为 true，可能导致数据回滚或数据丢失。生产环境中应谨慎开启，并确认故障节点和幸存节点的数据状态。

### 故障恢复后关闭 GroupMode

当故障节点、主机或 Location 恢复正常后，用户可关闭已开启的 MaintenanceMode 或 CriticalMode，使集群恢复正常运行状态。

#### 按目标关闭 GroupMode

用户可按 Location、主机或文件指定关闭范围：

```lang-bash
# 关闭 GuangZhou 上的 MaintenanceMode，并在关闭前检查节点状态
$ ./bin/sdb_stop_maintenance.sh -l GuangZhou --check

# 关闭 host1、host2 上的 CriticalMode，并在关闭前检查节点状态
$ ./bin/sdb_stop_critical.sh -H host1,host2 --check

# 根据文件关闭 GroupMode
$ ./bin/sdb_stop_maintenance.sh -f target.info --check
$ ./bin/sdb_stop_critical.sh -f target.info --check
```

#### 恢复整个集群

如果集群故障已经完全恢复，且希望关闭所有 MaintenanceMode 和 CriticalMode，可执行集群恢复工具：

```lang-bash
$ ./bin/sdb_restore_cluster.sh
```

`restore` 会先检查集群中所有节点状态。只有所有节点正常时，工具才会关闭全部 MaintenanceMode 和 CriticalMode；如果仍存在异常节点，工具会报错并停止恢复。

恢复完成后，建议再次查看和检查集群状态：

```lang-bash
$ ./bin/sdb_init_location.sh show
$ ./bin/sdb_init_location.sh check
```

## 工具与参数参考

Location 容灾工具包括如下主要文件：

- `bin/sdb_init_location.sh`：Location 初始化工具，支持查看、检查和初始化集群 Location 信息

- `bin/sdb_start_maintenance.sh`：开启 MaintenanceMode 工具，支持按 Location、Host 和 Domain 过滤

- `bin/sdb_stop_maintenance.sh`：关闭 MaintenanceMode 工具，支持按 Location、Host 和 Domain 过滤

- `bin/sdb_start_critical.sh`：开启 CriticalMode 工具，支持按 Location、Host 和 Domain 过滤

- `bin/sdb_stop_critical.sh`：关闭 CriticalMode 工具，支持按 Location、Host 和 Domain 过滤

- `bin/sdb_restore_cluster.sh`：集群恢复工具，关闭所有 MaintenanceMode 和 CriticalMode 

- `config/config.js.sample`：参数配置文件模板，用于配置协调节点地址、鉴权信息、期望 Location 分布、ActiveLocation 等信息

- `lib/lib.js`：通用函数库，用于输出表格、格式化时间和处理工具输出

- `output/`：工具输出目录，用于输出工具生成的文件

工具运行时会从 `/etc/default/sequoiadb` 中读取 SequoiaDB 安装目录，并调用安装目录下的 `bin/sdb` 执行 JS 逻辑。因此，执行机器需已安装 SequoiaDB，并存在 `/etc/default/sequoiadb` 文件。

### 配置文件参数说明

用户在首次使用前先从模板复制一份配置文件

```lang-bash
cp config/config.js.sample config/config.js
```

然后需要根据实际环境修改 `config/config.js`，主要参数说明如下：

| 参数名 | 描述 |
| ------ | ---- |
| sdbCoord | 协调节点地址。可以为字符串，例如 `"sdbserver1:11810"` |
| sdbUser | 登录 SequoiaDB 集群的用户名。如果数据库未开启鉴权，可以不填 |
| sdbPassword | 登录 SequoiaDB 集群的密码。如果数据库未开启鉴权，可以不填 |
| sdbToken | 密文文件鉴权使用的 token。不使用密文文件鉴权时可以不填 |
| sdbCipherFile | 密文文件路径。不使用密文文件鉴权时可以不填 |
| sdbCm | 本机 sdbcm 端口号，用于编目组无主时通过 Oma 获取本机编目节点并执行临时升主 |
| initLocationObject | 期望的 Location 分布。key 为 Location 名称，value 为主机名数组。另外可包含 `domains`（Domain 名称数组，默认为空）和 `domainMode`（`0` 为排除模式，`1` 为选择模式，默认 `1`），用于限制 `init`/`show` 的 Domain 作用范围 |
| activeLocation | 期望设置的 ActiveLocation。为空字符串时，`init` 不修改 ActiveLocation |
| reelectLevel | 执行 `init`、GroupMode 开关和 `restore` 后是否切主。取值为 0、1、2 时分别按 Maintenance/Critical、ActiveLocation、RunStatusWeight 级别执行切主分析；0、1、2 为向下包含关系，1 包含 0 的功能，2 包含 0 和 1 的功能；其他取值表示不切主 |
| minKeepTime | MaintenanceMode 和 CriticalMode 运行最低窗口时间，取值范围[1, 10080]，单位分钟 |
| maxKeepTime | MaintenanceMode 和 CriticalMode 运行最高窗口时间，取值范围[1, 10080]，单位分钟 |
| enforce | 是否强制开启 GroupMode 或强制执行编目节点升主。设置为 true 可能导致数据回滚或数据丢失，需谨慎使用 |

`initLocationObject` 配置示例如下：

```lang-javascript
initLocationObject = {
   "GuangZhou": [
      "host1",
      "host2"
   ],
   "ShenZhen": [
      "host3"
   ],
   // domains 为空时表示作用于整个集群；非空时配合 domainMode 限定 Domain 范围
   "domains": [],
   // 0: 排除模式（对 domains 以外的域生效）；1: 选择模式（仅对 domains 中的域生效，默认）
   "domainMode": 1
} ;
```

按 Domain 初始化或过滤展示时，可配置为：

```lang-javascript
initLocationObject = {
   "GuangZhou": [
      "host1",
      "host2"
   ],
   "ShenZhen": [
      "host3"
   ],
   "domains": ["domain1"],
   "domainMode": 1
} ;
```

> **Note:**
>
> - `domains` 为空时，`init`/`show` 按集群范围处理，行为与未配置 Domain 时一致。
> - `domainMode` 为 `1`（选择模式）时，仅对 `domains` 列表中的域生效；为 `0`（排除模式）时，对集群中除列表外的其它域生效。
> - `domains` 中的 Domain 必须已存在于集群中，否则工具报错退出。
> - 配置了非空 `domains` 后：`init` 调用 `dc.setLocation()` / `dc.setActiveLocation()` 时会传入 `Domain` 选项；`show` 会按 Domain 涉及的组、主机和节点过滤展示结果。若某主机上任意一个组属于目标 Domain，该主机的 Location 布局信息仍会完整展示。

### Location 初始化工具

`sdb_init_location.sh` 支持如下子命令：

| 子命令 | 描述 |
| ------ | ---- |
| show | 展示当前集群 Location、ActiveLocation、GroupMode 和异常信息，并生成 Location 信息文件。若配置了非空 `domains`，按 Domain 过滤展示结果 |
| check | 对比当前集群 Location 信息与配置文件或指定文件中的期望配置 |
| init | 根据配置文件或指定文件设置集群 Location 信息。若配置了非空 `domains`，仅在对应 Domain 范围内设置 Location / ActiveLocation |

进行 Location 初始化必须满足如下条件：

- SequoiaDB 集群版本支持 Location 和 `SdbDC` 相关接口

- 集群中存在可连接的协调节点

- 执行工具的用户具有执行 `SdbDC.setLocation()` 和 `SdbDC.setActiveLocation()` 的权限

- 集群的主机与 Location 规划已经明确，例如将主中心主机配置为 `GuangZhou`，灾备中心主机配置为 `ShenZhen`

- 建议在执行 `init` 前先执行 `show` 和 `check`，预览将要修改的配置


`sdb_init_location.sh` 支持如下命令行参数：

| 参数 | 描述 |
| ---- | ---- |
| -h, --help | 显示帮助信息 |
| -c, --conf | 指定配置文件，默认为 `config/config.js` |
| -f, --file | 指定 Location 信息文件。指定后，`check` 和 `init` 将以该文件作为期望配置 |
| -q, --quiet | 工具执行 `show` 后会默认打印集群信息到屏幕，指定后将不再打印 |

#### 操作步骤

1. 进入工具目录。

   ```lang-bash
   $ cd /opt/sequoiadb/tools/dr_location
   ```

2. 根据实际集群环境修改 `config/config.js`。

   例如将 `host1`、`host2` 配置为主中心 `GuangZhou`，将 `host3` 配置为灾备中心 `ShenZhen`：

   ```lang-javascript
   sdbCoord = "host1:11810" ;
   sdbUser = "sdbadmin" ;
   sdbPassword = "sdbadmin" ;

   initLocationObject = {
      "GuangZhou": [
         "host1",
         "host2"
      ],
      "ShenZhen": [
         "host3"
      ]
   } ;

   activeLocation = "GuangZhou" ;
   reelectLevel = "" ;
   minKeepTime = 100 ;
   maxKeepTime = 1000 ;
   enforce = false ;
   ```

3. 查看当前集群 Location 信息。

   ```lang-bash
   $ ./sdb_init_location.sh show
   ```

   `show` 会展示如下信息：

   - 当前集群的 Location 分布
   - 当前 ActiveLocation
   - 当前 MaintenanceMode 或 CriticalMode 状态
   - 未设置 Location、主机内 Location 不一致、主节点不在 ActiveLocation 等异常信息

   如果 `initLocationObject` 中配置了非空 `domains`，`show` 会按 Domain 过滤上述结果，并在输出中提示当前生效的 `domainMode` 与 domains 范围；未配置或 `domains` 为空时，提示作用范围为 cluster。

   同时，工具会在 `output` 目录下生成 `location.info` 文件。该文件可作为 `-f` 参数输入，用于后续 `check` 或 `init`。

   `show` 还会生成 `output/show.out`，用于保存本次展示结果。

4. 检查配置文件与当前集群是否一致。

   ```lang-bash
   $ ./sdb_init_location.sh check
   ```

   如果希望基于 `show` 生成的文件进行检查，可以执行：

   ```lang-bash
   $ ./sdb_init_location.sh check -f output/location.info
   ```

   `check` 只展示差异，不修改集群。输出内容包括：

   - 主机 Location 差异
   - ActiveLocation 差异

   如果存在差异，工具会生成 `output/check.out` 保存检查结果；如果配置完全一致，则提示检查通过。

5. 初始化集群 Location 信息。

   ```lang-bash
   $ ./sdb_init_location.sh init
   ```

   如果希望使用 `show` 生成并人工调整后的 Location 文件进行初始化，可以执行：

   ```lang-bash
   $ ./sdb_init_location.sh init -f output/location.info
   ```

   `init` 会执行如下操作：

   - 调用 `dc.setLocation(<hostname>, <location>)` 设置主机上节点的 Location；若配置了非空 `domains`，则传入 `{Domain: <domains>}`，仅对目标 Domain 内的数据组生效
   - 当 `activeLocation` 配置非空时，调用 `dc.setActiveLocation(<location>)` 设置 ActiveLocation；若配置了非空 `domains`，同样传入 `{Domain: <domains>}`，避免影响域外复制组
   - 根据 `reelectLevel` 配置调用 `dc.reelectAnalyze()` 进行切主分析和切主

6. 初始化完成后再次查看并检查集群状态。

   ```lang-bash
   $ ./sdb_init_location.sh show
   $ ./sdb_init_location.sh check
   ```

#### Location 文件格式

`sdb_init_location.sh show` 生成的 Location 信息文件格式如下：

```lang-text
[GuangZhou(active)]
host1
host2

[ShenZhen]
host3
```

表示 `GuangZhou` 是 `activeLocation` 包含 `host1,host2` 主机，`ShenZhen` 包含 `host3` 主机。
用户可以根据实际规划修改该文件，然后通过 `-f` 参数指定该文件作为 `check` 或 `init` 的输入。

如果希望在 `-f` 文件中限定 Domain 范围，可增加 `[domains(1)]` 或 `[domains(0)]` 段：

```lang-text
[domains(1)]
domain1
domain2

[GuangZhou(active)]
host1
host2

[ShenZhen]
host3
```

其中：

- `[domains(1)]`：选择模式，仅对列表中的 Domain 生效
- `[domains(0)]`：排除模式，对列表以外的 Domain 生效
- Domain 段中每一行填写一个 Domain 名称

> **Note:**
>
> `-f` 文件中的 Location / Domain 配置会覆盖 `config.js` 中的 `initLocationObject` 与 `activeLocation`。Domain 段写法必须为 `[domains(0)]` 或 `[domains(1)]`，其它写法将报错。

#### show 命令输出说明

`sdb_init_location.sh show` 生成的结果文件 `output/show.out` 会包含以下非空内容：

| 表格 | 描述 |
| ---- | ---- |
| `[Location Layout Info]` | 集群 Location 信息 |
| `[GroupMode Runtime Info]` | 集群中正在运行的 GroupMode 信息 |
| `[Abnormal: Empty Location Host]` | 异常信息：当前集群中未配置 Location 的主机 |
| `[Abnormal: Mixed Location Info]` | 异常信息：节点 Location 与主机多数派不一致 |
| `[Abnormal: Primary Node Not In ActiveLocation]` | 异常信息：主节点不在 ActiveLocation 中 |
| `[Abnormal: Node Not In GroupMode]` | 异常信息：主机处于 GroupMode 时，主机中未开启 GroupMode 的节点 |
| `[Abnormal: Group Not In GroupMode]` | 异常信息：Location 处于 GroupMode 时，Location 中未开启 GroupMode 的组 |

#### check 命令输出说明

`sdb_init_location.sh check` 生成的结果文件 `output/check.out` 会包含以下非空内容：

| 表格 | 描述 |
| ---- | ---- |
| `[Diff: Host Location]` | 主机当前 Location 与配置中不一致 |
| `[Diff: Empty Location]` | 主机当前未设置 Location, 且配置文件中也未配置 Location |
| `[Diff: ActiveLocation]` | 集群当前 ActiveLocation 与配置中不一致 |

#### 注意事项

- `init` 会修改集群节点的 Location 信息，生产环境执行前应先执行 `show` 和 `check`。

- `activeLocation` 配置为空时，`init` 不会修改 ActiveLocation。

- 使用 `-f` 参数时，文件中的 Location / Domain 配置优先于 `config.js` 中的配置。

- 配置非空 `domains` 后，`init` 仅修改目标 Domain 内数据组的 Location / ActiveLocation；`show` 仅展示与目标 Domain 相关的信息。排除模式生效后若没有剩余 Domain，工具会跳过 Domain 范围的 Location 更新，而不会回退为集群级修改。

### GroupMode 管理工具

`sdb_start_maintenance.sh`、`sdb_stop_maintenance.sh`、`sdb_start_critical.sh`、`sdb_stop_critical.sh` 支持如下命令行参数：

| 参数 | 描述 |
| ---- | ---- |
| -h, --help | 显示帮助信息 |
| -c, --conf | 指定配置文件，默认为 `config/config.js` |
| -H, --hostname | 指定主机开启或关闭 GroupMode |
| -l, --location | 指定 Location 开启或关闭 GroupMode |
| -d, --domain | 指定 Domain，对 Host 和 Location 进行二次过滤，仅涉及域中的数据组 |
| -f, --file | 指定 Location 信息文件。指定后，开关 GroupMode 的目标以文件中配置为准 |
| --check | 停止 GroupMode 时，检查节点状态，需要节点正常才能停止 |

> **Note:**
>
> `-f` 参数优先级高于 `-l`、`-H` 和 `-d` 等命令行目标参数。指定 `-f` 后，工具将使用文件中的 `[location]`、`[hostname]` 和 `[domain]` 作为目标范围。

GroupMode 管理工具的行为如下：

- 开启 MaintenanceMode 时，工具调用 `dc.startMaintenanceMode()`，并传入 `MinKeepTime`、`MaxKeepTime` 和 `Enforced`。
- 开启 CriticalMode 时，工具会先尝试将编目主节点切换到目标 Location 或 Host，再调用 `dc.startCriticalMode()`。
- 关闭 MaintenanceMode 或 CriticalMode 时，如果未指定 Location、Host、Domain 或文件目标，则默认关闭集群中所有对应类型的 GroupMode。
- 使用 `--check` 关闭 GroupMode 时，工具会先检查目标节点状态，只有节点状态正常时才继续关闭。
- 执行 GroupMode 开关后，工具会根据 `reelectLevel` 配置调用 `dc.reelectAnalyze()` 进行切主。

#### 操作步骤

1. 进入工具目录。

   ```lang-bash
   $ cd /opt/sequoiadb/tools/dr_location
   ```

2. 根据实际集群环境修改 `config/config.js`。

   例如将 `reelectLevel` 配置为 0，将 `minKeepTime` 配置为 10：

   ```lang-javascript
   reelectLevel = 0 ;
   minKeepTime = 10 ;
   maxKeepTime = 1000 ;
   enforce = false ;
   ```

3. 开启 MaintenanceMode。

	在 GuangZhou 上开启

   ```lang-bash
   $ ./sdb_start_maintenance.sh -l GuangZhou
   ```

	在 host1 上 domain1 包含的数据组中开启

   ```lang-bash
   $ ./sdb_start_maintenance.sh -H host1 -d domain1
   ```

4. 关闭 MaintenanceMode。

	在 GuangZhou 上关闭，关闭前检查节点状态

   ```lang-bash
   $ ./sdb_stop_maintenance.sh -l GuangZhou --check
   ```

	在 host1 上 domain1 包含的数据组中关闭

   ```lang-bash
   $ ./sdb_stop_maintenance.sh -H host1 -d domain1
   ```

5. 开启 CriticalMode。

	在 GuangZhou 上开启

   ```lang-bash
   $ ./sdb_start_critical.sh -l GuangZhou
   ```

	在 host1 上 domain1 包含的数据组中开启

   ```lang-bash
   $ ./sdb_start_critical.sh -H host1 -d domain1
   ```

6. 关闭 CriticalMode。

	在 GuangZhou 上关闭，关闭前检查节点状态

   ```lang-bash
   $ ./sdb_stop_critical.sh -l GuangZhou --check
   ```

	在 host1 上 domain1 包含的数据组中关闭

   ```lang-bash
   $ ./sdb_stop_critical.sh -H host1 -d domain1
   ```

7. 开关 GroupMode 后可通过 `show` 查看集群状态

   ```lang-bash
   $ ./sdb_init_location.sh show
   ```

#### 目标文件格式

GroupMode 管理工具 `-f,--file` 支持文件格式如下
```lang-text
[location]
location1
location2

[hostname]
host1
host2

[domain]
domain1
domain2
```
用户可以根据实际规划修改该文件，然后通过 `-f` 参数指定该文件 GroupMode 管理工具的输入，工具会在指定的目标上开关 GroupMode。

> **Note:**
>
> 文件中 `domain` 会作为过滤条件使用。例如同时指定 `[location]` 和 `[domain]` 时，工具只会对指定 Domain 范围内的目标 Location 生效。

### 集群恢复工具

`sdb_restore_cluster.sh` 支持如下命令行参数：

| 参数 | 描述 |
| ---- | ---- |
| -h, --help | 显示帮助信息 |
| -c, --conf | 指定配置文件，默认为 `config/config.js` |

#### 操作步骤

1. 进入工具目录。

   ```lang-bash
   $ cd /opt/sequoiadb/tools/dr_location
   ```

2. 根据实际集群环境修改 `config/config.js`。

3. 开启 MaintenanceMode。

	在 GuangZhou 上开启

   ```lang-bash
   $ ./sdb_start_maintenance.sh -l GuangZhou
   ```

4. 恢复集群状态。

	检查所有节点状态，若节点状态正常则关闭所有 GroupMode

   ```lang-bash
   $ ./sdb_restore_cluster.sh
   ```

   `restore` 会先检查集群中所有节点是否正常。若存在异常节点，工具会停止恢复并报错；若所有节点正常，则依次关闭全部 MaintenanceMode 和 CriticalMode，并根据 `reelectLevel` 配置执行切主分析。

5. 开启 CriticalMode。

	在 GuangZhou 上开启

   ```lang-bash
   $ ./sdb_start_critical.sh -l GuangZhou
   ```

6. 恢复集群状态。

	检查所有节点状态，若节点状态正常则关闭所有 GroupMode

   ```lang-bash
   $ ./sdb_restore_cluster.sh
   ```

7. 开关 GroupMode 后可通过 `show` 查看集群状态

   ```lang-bash
   $ ./sdb_init_location.sh show
   ```

### 特殊场景处理

#### 编目组无主

除 `init` 和 `restore` 外，工具在连接协调节点时如果遇到编目组无主场景，会尝试执行临时编目升主：

1. 通过本机 `sdbCm` 端口连接 sdbcm。
2. 通过协调节点配置文件获取编目节点信息。
3. 逐个连接编目节点，直到成功连接存活编目节点。
4. 调用 `forceStepUp({Seconds: 60 * maxKeepTime, Enforced: enforce})` 将存活编目节点进行强制升主。

如果 `enforce` 设置为 true，编目强制升主可能造成数据丢失，需谨慎使用。

#### Standalone 节点

工具会检查当前连接是否为 Standalone 节点。Standalone 节点不支持该工具，工具会报错退出。

#### 切主分析

在 sdb shell 中查看切主范围，关注结果中 `RunStatusWeightDesp` 和 `RunStatusWeight` 字段：

```lang-bash
db.snapshot(SDB_SNAP_CONFIGS, new SdbSnapshotOption().options({IgnoreDefault:true, ShowRunStatus:true}));
```

当 `reelectLevel` 取值为 0、1 或 2 时，工具会在 `init`、GroupMode 开关和 `restore` 操作后调用 `dc.reelectAnalyze()`：

| reelectLevel | 切主范围 |
| ------------ | -------- |
| 0 | 针对 Maintenance/Critical 相关运行状态权重执行切主分析 |
| 1 | 向下包含 0 的功能，同时针对 ActiveLocation 相关运行状态权重执行切主分析 |
| 2 | 向下包含 0 和 1 的功能，同时针对所有 RunStatusWeight 执行切主分析 |

如果分析结果显示需要切主，工具会再次调用 `dc.reelectAnalyze(..., true)` 执行切主。

[^_^]:
    本文使用到的所有链接及引用。
[HA_DR]:manual/Distributed_Engine/Maintainance/HA_DR/disaster_recovery.md
