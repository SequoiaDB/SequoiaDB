[^_^]:
    高可用与容灾部署方案

SequoiaDB 巨杉数据库采用复制组多副本架构，保证数据库的高可用性。同时，SequoiaDB 支持单机、机房和城市级别的容灾，提供同城双中心、同城三中心、两地三中心和三地五中心的部署方案。用户可根据机房配置和容灾需求，选择合适的方案进行[集群部署][cluster_deployment]。

##同城双中心##

###特点###

- 运维成本低、建设速度快、管理相对简单。
- 同城“双活”，充分利用资源，避免因一个数据中心处于闲置状态而造成资源浪费。
- 能够应对单机级别的灾难，但无法应对机房和城市级别的灾难。

###部署示意图###

同城部署一个三副本集群，其中主中心包含两个副本，灾备中心为单副本。示意图如下：

![同城双中心集群部署][twodatacenter_cluster]

##同城三中心##

###特点###

- 相较于“同城双中心”，在运维成本增幅不大的情况下，进一步提升数据库的高可用性。
- 同城“多活”，充分利用资源，避免因一个数据中心处于闲置状态而造成的资源浪费。
- 能够应对机房级别的灾难，但无法应对城市级别的灾难。

###部署示意图###

同城部署一个三副本集群，其中主中心和灾备中心均为单副本。示意图如下：

![同城三中心集群部署][threedatacenter]

##两地三中心##

###特点###

- 异地灾备中心仅做数据备份，不提供业务服务。
- 存在资源浪费的情况，但提升了数据库的容灾能力。
- 能够应对城市级别的灾难，降低城市灾难带来的影响。

###部署示意图###

在同城双中心部署基础上，异地部署一个单副本集群。示意图如下：

![两地三中心集群部署][twocity_threedatacenter]

##三地五中心##

###特点###

- 运维成本高、建设速度慢、管理相对困难。
- 网络延迟大，但最大程度保障了数据库的容灾能力。
- 能够应对城市级别的灾难，当其中一个城市发生灾难，其他城市能够接管关键或全部业务，实现故障无感知。

###部署示意图###

三个城市组成一个五副本集群，每个数据中心都为单副本。示意图如下：

![三地五中心集群部署][threecity_fivedatacenter]

[^_^]:
    本文使用到的所有链接
[twodatacenter_cluster]:images/Deployment/twodatacenter_cluster.png
[threedatacenter]:images/Deployment/threedatacenter.png
[twocity_threedatacenter]:images/Deployment/twocity_threedatacenter.png
[threecity_fivedatacenter]:images/Deployment/threecity_fivedatacenter.png
[cluster_deployment]:manual/Deployment/cluster_deployment.md