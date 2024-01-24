
SAC 监控可以查看当前服务和主机的运行状态。

完成[添加主机][add_host]及[创建存储集群][install_module]后，即可使用 SequoiaDB 监控服务。

- 点击左侧导航【监控】，选择需要查看的服务，进入监控总览页面

  ![总览][overview]

  页面左侧显示主机数量、磁盘数量以及 CPU、内存、磁盘的使用情况。

  页面右侧显示服务版本，会话、域、分区组、节点、集合、记录及 Lob 的数量，图表则可以查看当前 insert、update、delete 和 read 的实时速率。

  如果当前服务有异常时，页面下方将显示当前服务的警告以及错误信息。


- 点击页面上方的【节点】、【资源】或者【主机】可进入对应的页面，并查看详细的监控信息

  ![总览][overview_2]

  


[^_^]:
    本文使用的所有引用及链接
[add_host]:manual/SAC/Deployment/Deployment_Bystep/add_host.md
[install_module]:manual/SAC/Deployment/Deployment_Bystep/add_sdb_module.md

[overview]:images/SAC/Monitor/overview.png
[overview_2]:images/SAC/Monitor/overview_2.png