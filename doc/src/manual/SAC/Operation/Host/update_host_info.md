当主机信息发生改变时，用户需要在 SAC 进行更新主机信息操作。

1. 在【部署】->【主机】中可以看到主机状态都是正常的
![更新主机信息][update_host_1]

2. 主机 ubuntu-test-03 的 IP 改变导致访问 ubuntu-test-03 主机失败
![更新主机信息][update_host_2]

 >**Note:**
 >
 > 用户将鼠标停留在红色标记的状态上会显示错误信息。

3. ubuntu-test-03 的 IP 从 192.168.3.233 变成 192.168.3.234
![更新主机信息][update_host_3]

4. 选择需要更新 IP 的主机，点击【主机操作】->【更新主机信息】按钮
![更新主机信息][update_host_4]

5. 在窗口修改 ubuntu-test-03 的 IP 地址为 192.168.3.234 后点击 **确定** 按钮
![更新主机信息][update_host_5]

6. 更新完成，关闭窗口
![更新主机信息][update_host_6]

7. 在【部署】->【主机】中可以看到 ubuntu-test-03 主机恢复正常
![更新主机信息][update_host_7]

> **Note:** 
>
> - 更新成功后，如果主机状态仍然无法连接，可能是 dns 缓存没有更新导致。需要在 Linux 终端输入 ```$ service nscd restart``` 指令更新缓存。
> - 如果主机用户 sdbadmin 的密码被修改，会导致更新失败。


[^_^]:
    本文使用的所有引用和链接
[update_host_1]:images/SAC/Operation/Host/update_host_1.png
[update_host_2]:images/SAC/Operation/Host/update_host_2.png
[update_host_3]:images/SAC/Operation/Host/update_host_3.png
[update_host_4]:images/SAC/Operation/Host/update_host_4.png
[update_host_5]:images/SAC/Operation/Host/update_host_5.png
[update_host_6]:images/SAC/Operation/Host/update_host_6.png
[update_host_7]:images/SAC/Operation/Host/update_host_7.png