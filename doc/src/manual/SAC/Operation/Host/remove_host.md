本文档主要介绍如何在 SAC 中解绑和卸载主机。

> **Note:**  
> 无论是解绑主机还是删除主机都必须先[删除或者移除存储集群][remove_module]，否则解绑会提示主机有服务存在。

解绑主机
----

用户通过解绑主机操作可以将所选主机从 SAC 集群中分离出来，但不会删除 SequoiaDB 巨杉数据库软件。

1. 进入【部署】->【主机】页面
![解绑主机][unbind_host_1]

2. 选择要解绑的主机，可以点击 **全选** 按钮选择全部
![解绑主机][unbind_host_2]

3. 点击【删除主机】->【解绑主机】按钮
![解绑主机][unbind_host_3]

4. 解绑完成，点击 **确定** 按钮
![解绑主机][unbind_host_4]

5. 演示的三台主机均已解绑
![解绑主机][unbind_host_5]

卸载主机
----

用户通过卸载主机操作可以将所选主机从 SAC 集群中分离出来，并删除主机上的 SequoiaDB 软件。

1. 进入【部署】->【主机】页面
![卸载主机][uninstall_host_1]

2. 选择要删除的主机，可以点击 **全选** 按钮选择全部
![卸载主机][uninstall_host_2]

3. 点击【删除主机】->【卸载主机】按钮
![卸载主机][uninstall_host_3]

4. 等待任务完成
![卸载主机][uninstall_host_4]

5. 卸载完成
![卸载主机][uninstall_host_5]


[^_^]:
    本文使用的所有引用和链接
[remove_module]:manual/SAC/Operation/Module/remove_module.md

[unbind_host_1]:images/SAC/Operation/Host/unbind_host_1.png
[unbind_host_2]:images/SAC/Operation/Host/unbind_host_2.png
[unbind_host_3]:images/SAC/Operation/Host/unbind_host_3.png
[unbind_host_4]:images/SAC/Operation/Host/unbind_host_4.png
[unbind_host_5]:images/SAC/Operation/Host/unbind_host_5.png
[uninstall_host_1]:images/SAC/Operation/Host/uninstall_host_1.png
[uninstall_host_2]:images/SAC/Operation/Host/uninstall_host_2.png
[uninstall_host_3]:images/SAC/Operation/Host/uninstall_host_3.png
[uninstall_host_4]:images/SAC/Operation/Host/uninstall_host_4.png
[uninstall_host_5]:images/SAC/Operation/Host/uninstall_host_5.png