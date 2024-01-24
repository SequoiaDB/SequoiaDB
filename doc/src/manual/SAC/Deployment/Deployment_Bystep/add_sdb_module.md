本文档主要介绍如何在 SAC 中创建存储集群和添加已有存储集群。

## 创建存储集群

用户可以通过创建存储集群操作在 SAC 中创建一个存储集群。

1. 在【部署】页面点击【添加存储集群】->【创建存储集群】按钮，服务类型选择 SequoiaDB 数据库，点击 **确定** 按钮 
![配置服务][add_module]

2. 根据实际需求配置服务
![配置服务][config_module_1]

3. 点击 **选择安装服务的主机** 按钮，可以指定主机安装服务，默认已经全选，点击 **下一步** 按钮
![配置服务][config_module_2]

4. 进入修改服务页面，根据实际需求配置完成后点击 **下一步** 按钮

   - 修改服务的配置，如设置全部节点开启事务
     ![修改服务][modify_module_1]

     点击 **全选** 按钮
     ![修改服务][modify_module_2]

     点击 **批量修改节点** 按钮，把 transactionon 改为 true 后，点击 **确定** 按钮
     ![修改服务][modify_module_3]

   - v2.8 及以上版本支持 SequoiaDB 集群模式下导入导出配置，支持 JSON 和 XML 格式；点击 **编辑配置** 按钮，出现编辑配置的窗口   
     ![修改服务][modify_module_4]

   > **Note:**  
   >
   > - 如果要开启事务，必须将所有节点都开启。
   > - 批量修改节点配置数据路径和服务名支持特殊规则来简化修改。规则可以点击页面 **提示** 的 **帮助**。  
   > - 批量修改节点配置时，如果值为空，那么代表该参数的值不修改。


5. 完成安装
![安装服务][install_module_2]

## 添加已有存储集群

用户可以通过添加已有存储集群操作，将已经创建的集群添加至 SAC 管理。

1. 进入【部署】页面，点击【添加存储集群】->【添加已有的存储集群】按钮
![发现服务][append_sdb_1]

2. 类型选择 SequoiaDB，点击 **确定** 按钮
![发现服务][append_sdb_2]

3. 填写 SequoiaDB 协调节点地址和协调节点端口号，填写完毕后点击 **确定** 按钮
![发现SequoiaDB][append_sdb_3]

   > **Note:**
   >
   > - 地址支持填写 IP 或主机名形式。
   > - 如果用户为服务创建了鉴权，需要填写相应的数据库用户名和数据库密码。
 
  当服务的所有主机不在 SAC 管理时，会提示“是否前往添加主机”，点击 **是**，进入【安装主机】；安装完成后，再从第一步重新操作

   ![发现服务][append_sdb_6]

4. 添加完成，在当前页面可以查看该服务的信息
![发现SequoiaDB][append_sdb_5]



[^_^]:
     本文使用的所有引用和链接
[add_module]:images/SAC/Deployment/Deployment_Bystep/add_module.png
[config_module_1]:images/SAC/Deployment/Deployment_Bystep/config_module_1.png
[config_module_2]:images/SAC/Deployment/Deployment_Bystep/config_module_2.png
[modify_module_1]:images/SAC/Deployment/Deployment_Bystep/modify_module_1.png
[modify_module_2]:images/SAC/Deployment/Deployment_Bystep/modify_module_2.png
[modify_module_3]:images/SAC/Deployment/Deployment_Bystep/modify_module_3.png
[modify_module_4]:images/SAC/Deployment/Deployment_Bystep/modify_module_4.png
[install_module_2]:images/SAC/Deployment/Deployment_Bystep/install_module_2.png
[append_sdb_1]:images/SAC/Deployment/Deployment_Bystep/append_sdb_1.png
[append_sdb_2]:images/SAC/Deployment/Deployment_Bystep/append_sdb_2.png
[append_sdb_3]:images/SAC/Deployment/Deployment_Bystep/append_sdb_3.png
[append_sdb_6]:images/SAC/Deployment/Deployment_Bystep/append_sdb_6.png
[append_sdb_5]:images/SAC/Deployment/Deployment_Bystep/append_sdb_5.png
