本文档主要介绍如何在 SAC 中移除和删除存储集群。

> **Note:** 
> 
> 如果需要移除的储集群跟其他服务有关联，需要先解除关联。

移除存储集群
----

用户通过移除存储集群操作可以将服务从 SAC 中分离出来，不卸载相关服务。

1. 进入【部署】页面，点击【删除存储集群】->【移除存储集群】按钮

   ![解绑服务][unbind_1]

2. 选择要移除的集群后点击 **确定** 按钮

   ![卸载服务][unbind_2]

3. 移除成功，点击 **确定** 按钮

   ![卸载服务][unbind_3]

4. 显示服务已经不在 SAC 中

   ![卸载服务][unbind_4]

删除存储集群
----

用户通过卸载存储集群操作可以将服务从 SAC 中分离出来，并卸载协调节点、编目节点及数据节点。

1. 进入【部署】页面，点击【删除存储集群】->【删除存储集群】按钮

   ![卸载服务][uninstall_1]

2. 在窗口选择要删除的集群，并选择是否需要删除所有数据，如果选择“否”，则 SAC 会检查服务是否含有数据，有数据则不会卸载服务并报错；如果选择“是”，SAC 忽略服务数据，直接卸载服务

   ![卸载服务][uninstall_2]

3. 等待任务完成

   ![卸载服务][uninstall_3]

4. 删除完成，点击 **完成** 按钮

   ![卸载服务][uninstall_4]



[^_^]:
     本文使用的所有引用及链接
[unbind_1]:images/SAC/Operation/Module/unbind_1.png
[unbind_2]:images/SAC/Operation/Module/unbind_2.png
[unbind_3]:images/SAC/Operation/Module/unbind_3.png
[unbind_4]:images/SAC/Operation/Module/unbind_4.png
[uninstall_1]:images/SAC/Operation/Module/uninstall_1.png
[uninstall_2]:images/SAC/Operation/Module/uninstall_2.png
[uninstall_3]:images/SAC/Operation/Module/uninstall_3.png
[uninstall_4]:images/SAC/Operation/Module/uninstall_4.png