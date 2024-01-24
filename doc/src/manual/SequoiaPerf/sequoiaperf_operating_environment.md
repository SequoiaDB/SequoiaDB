本文档主要从资源、环境要求、端口、浏览器、分辨率介绍的 sequoiaPerf 运行环境。


##资源##

每个 SequoiaPerf 实例均需要 8GB 的内存和 2GB 的磁盘。

##环境要求##

为了准确地收集和展示性能数据，SequoiaPerf 对部署环境存在以下需求：

- 需要完成 SequoiaDB [集群部署][cluster]。
- 所监控的 SequoiaDB 集群包含的每台物理机器均需要使用 [NTP 同步时钟][ntp]。
- 部署 SequoiaPerf 的服务器也要求使用 NTP 与 SequoiaDB 集群服务器进行时钟同步，以保证时序数据库中信息的连续性和准确性。
- SequoiaPerf 通过连接 SequoiaDB 集群的[协调节点][coord]获取监控数据。为保障性能，建议 SequoiaPerf 与实际业务分别连接不同的协调节点。

##端口##

SequoiaPerf 网页使用的默认端口为 14000。


##浏览器##

SequoiaPerf 可以使用以下浏览器：

* Chrome，支持 Chrome 19.0 以上版本，建议使用当前较新的版本。
* Firefox，支持 Firefox 22.0 以上版本，建议使用当前较新的版本。
* IE7/8/9+，为了更好使用体验， 建议使用 IE9 或更高版本。
* Microsoft Edge，建议使用较新版本。
* 其他主流浏览器，建议使用较新的版本；国内浏览器通常是多内核，在不能正常使用可以尝试切换内核。


##分辨率和缩放##

* SequoiaPerf 页面需要分辨率不小于 1024*768，为了更好使用体验，建议分辨率在 1366*768 或更高。
* SequoiaPerf 页面支持缩放，支持 80%、90%、100%、125% 的缩放，默认是 100%。


##浏览器设置要求##

* IE 浏览器设置了较高安全级别，需要把网站添加到可信站点。
* 部分浏览器需要关闭特殊模式，如：IE 要关闭兼容模式（兼容模式默认关闭）。
* 浏览器必须允许网站运行 Javascript （默认允许）。


[^_^]:
    本文使用的所有引用及链接

[cluster]:manual/Deployment/cluster_deployment.md
[ntp]:https://www.pool.ntp.org/en/use.html
[coord]:manual/Distributed_Engine/Architecture/Node/coord_node.md


