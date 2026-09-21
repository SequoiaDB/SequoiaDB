# 集群复制脚本生成工具

从已有 SequoiaDB 集群读取元数据（组、domain、数据源、集合空间、集合、索引），生成一份可在目标环境执行的 JS 脚本（默认 `copyCluster.js`）。

目录文件：

- `generate_copy_cluster.sh`：bash 入口，负责解析参数并调用 `sdb`
- `generatJs.js`：读取源集群元数据并写出复制脚本

## 环境要求

- 已安装 SequoiaDB，可使用 `sdb` shell
- 能连接源集群的 coord 和 catalog 节点
- 集群开启了鉴权时，需要提供用户名和密码

未指定 `--sdb` 时，按以下顺序查找 `sdb`：

1. `/etc/default/sequoiadb` 中的 `INSTALL_DIR/bin/sdb`
2. 本工具目录上两级的 `../../bin/sdb`（源码树布局）

都找不到时，用 `--sdb` 指定完整路径。

## 生成脚本

进入本目录后执行：

```bash
cd tools/generateJSforcopycluster
chmod +x generate_copy_cluster.sh
```

无鉴权、本机默认端口（coord `11810`，catalog `11820`）：

```bash
./generate_copy_cluster.sh
```

有鉴权，并指定实际 catalog 端口：

```bash
./generate_copy_cluster.sh -u sdbadmin -p sdbadmin --cata-port 11800
```

指定连接信息和输出文件：

```bash
./generate_copy_cluster.sh \
  --coord-host 192.168.1.10 --coord-port 11810 \
  --cata-host 192.168.1.10 --cata-port 11820 \
  -u sdbadmin -p sdbadmin \
  -o /tmp/copyCluster.js
```

只导出部分对象：

```bash
./generate_copy_cluster.sh \
  -u sdbadmin -p sdbadmin --cata-port 11800 \
  --domains domain1,domain2 --cs cs1 --cl t1,t2 \
  -o /tmp/copyCluster.js
```

查看帮助：

```bash
./generate_copy_cluster.sh -h
```

## 参数

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `--coord-host` | `localhost` | 源集群 coord 主机 |
| `--coord-port` | `11810` | coord 端口 |
| `--cata-host` | `localhost` | catalog 主机 |
| `--cata-port` | `11820` | catalog 端口，需与集群实际端口一致 |
| `-u` / `--user` | 空 | 用户名。无鉴权可省略 |
| `-p` / `--password` | 空 | 密码。无鉴权可省略 |
| `-o` / `--output` | `./copyCluster.js` | 生成的 JS 文件路径 |
| `--domains` | 不传表示不过滤 | domain 白名单，逗号分隔 |
| `--ds` | 不传表示不过滤 | 数据源白名单，逗号分隔 |
| `--cs` | 不传表示不过滤 | 集合空间白名单，逗号分隔 |
| `--cl` | 不传表示不过滤 | 集合短名白名单，逗号分隔 |
| `--sdb` | 自动查找 | `sdb` 可执行文件路径 |
| `-h` / `--help` | — | 打印帮助 |

用户名、密码中的 `\` 和 `"` 会在传给 `sdb` 前做转义。

## 过滤规则

`--domains`、`--ds`、`--cs`、`--cl` 都是白名单：

- 不传或为空：导出全部
- 列表中的名字在集群里不存在：跳过，不报错
- `--cl` 只写集合短名，例如 `t1`，不要写 `cs1.t1`
- `--cs` 和 `--cl` 同时指定时取交集：集合短名在 `--cl` 中，且所属集合空间在 `--cs` 中，才会导出

例如 `--cs cs1,cs2 --cl t1,t2`，可能导出 `cs1.t1`、`cs1.t2`、`cs2.t1`、`cs2.t2`，不会导出 `cs3.t1` 或 `cs1.t3`。

只写 `--cl t1`、不写 `--cs` 时，所有集合空间下名为 `t1` 的集合都会进入集合列表。

## 执行生成的脚本

生成文件默认会直接调用 `main()`。执行前用 `-e` 传入目标 coord、账号和步骤号 `step`：

```bash
sdb -e 'var coordHostName="localhost"; var coordPort=11810; var username="sdbadmin"; var password="sdbadmin"; var step=0' -f ./copyCluster.js
```

`step` 含义：

| step | 动作 |
| --- | --- |
| 0 | 按导出的组信息部署节点 |
| 1 | 创建 domain |
| 2 | 创建数据源、集合空间、集合 |
| 3 | 创建索引 |
| 4 | 按需修改 domain 属性（如 AutoSplit） |

按 0 到 4 的顺序分步执行。`step=0` 会按源集群拓扑创建节点，请在空集群或确认无路径、端口冲突后再执行。

无鉴权时 `username`、`password` 可传空字符串，或依赖脚本内默认空值。

## 仍可直接调用 JS

不经过 bash 入口时，仍可用 `sdb -e` 注入变量后执行 `generatJs.js`：

```bash
sdb -e 'var coordHostName="localhost"; var coordPort=11810; var catalogHostName="localhost"; var catalogPort=11820; var username="sdbadmin"; var password="sdbadmin"; var generateFilePath="./copyCluster.js"' -f generatJs.js
```

可选过滤变量（JS 数组）：`inDomainNames`、`inDSNames`、`inCSNames`、`inCLNames`。

## 注意

- catalog 端口以集群实际配置为准。连到数据节点去读 `SYSCAT` 时，常见报错为 `-34`（集合空间不存在）。
- 生成阶段的进度输出来自 `generatJs.js`（如 `getGroupDefine success`）。bash 入口在成功时基本不额外打印，只在参数错误或找不到 `sdb` 时输出错误。
