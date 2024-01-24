[^_^]:
    安装部署

用户使用 FlinkSQL 连接器前需要对 Flink 和 FlinkSQL 连接器进行安装部署。

##环境需求##

FlinkSQL 连接器组件的环境需求如下：

- Flink 版本： 1.12.0+
- JDK 版本：1.8+
- SequoiaDB 版本：3.6+

##部署 Flink 集群##

Flink 提供了多种集群部署方式，用户可根据实际需求参考 [Flink 官方文档][flink]部署 Flink 集群。

##安装 FlinkSQL 连接器##

1. 从 [SequoiaDB 官网][sequoiadb]下载 FlinkSQL 连接器和 SequoiaDB Java 驱动的 jar 文件

2. 将下载的 jar 文件复制到 Flink 集群所包含机器的 `$FLINK_HOME/lib/` 目录中

3. 重启 Flink 集群使改动生效

##配置 SequoiaDB 主机名映射##

在使用 FlinkSQL 连接器之前，用户需要将 SequoiaDB 集群中所有主机的主机名/IP映射关系，配置到 Flink 节点所在主机的 `/etc/hosts` 文件中。

```lang-bash
$ echo "192.168.20.200 sdbserver1" >> /etc/hosts
$ echo "192.168.20.201 sdbserver2" >> /etc/hosts
$ echo "192.168.20.202 sdbserver3" >> /etc/hosts
```

如果 Flink 通过 Kubernetes 进行部署，用户可以参考以下方式配置主机名映射：

1. 修改 Kubernetes CoreDNS 配置

    ```lang-bash
    $ kubectl edit configmap coredns -n kubesystem
    ```

2. 添加 Sequoiadb 节点所在主机的主机名映射：

    ```lang-yaml
    apiVersion: v1
    kind: ConfigMap
    metadata:
        name: coredns
        namespace: kube-system
    data:
        Corefile: |
            .53: {
                ...
                hosts {
                    192.168.20.200 sdbserver1
                    192.168.20.201 sdbserver2
                    192.168.20.202 sdbserver3
                    fallthrough
                }
                ...
            }
    ```

3. 重新部署 CoreDNS 使上述修改生效

    ```lang-bash
    $ kubectl rollout restart -n kube-system deployment/coredns
    ```

[^_^]:
    本文使用的所有引用及链接
[flink]:https://nightlies.apache.org/flink/flink-docs-release-1.14/zh/docs/deployment/overview/
[sequoiadb]:https://download.sequoiadb.com/cn/driver