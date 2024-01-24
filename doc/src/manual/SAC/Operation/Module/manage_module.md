本文档主要介绍存储集群的扩容、减容、修改配置、同步配置及鉴权操作。

##扩容##

服务扩容有两种方式：
- 添加分区组：增加分区组  
- 添加副本数：在原有的组上增加节点，如果原有的组节点数量大于或等于设置的值，该组不增加节点

1. 进入【部署】页面，点击【存储集群操作】->【扩容】按钮
![服务选择][extend_1]

2. 选择需要扩容的集群，点击 **确定** 按钮
![服务选择][extend_2]

3. 根据实际需求选择【扩容模式】，设置完毕后点击 **下一步** 按钮

   * 添加分区组，设置【分区组数】和【副本数】

     ![水平扩容][extend_4]

   * 添加副本数，设置每个分区组需要添加的副本数

     ![垂直扩容][extend_5.png]

4. 在【修改服务】页面可以根据需要修改新增节点的配置，修改完毕后点击 **下一步** 按钮
![修改服务][extend_6]

5. 扩容完成
![修改服务][extend_8]

> **Note:**
>
> - 如果要开启事务，同一个分区组下的节点都必须开启。  
> - 批量修改节点配置数据路径和服务名支持特殊规则来简化修改，规则可以点击页面【提示】->【帮助】。  
> - 批量修改节点配置时，如果值为空，那么代表该参数的值不修改。

##减容##

减容可以删除服务中的一个或多个节点。

1. 进入【部署】页面，点击【存储集群操作】->【减容】按钮
![服务选择][shrink_1]

2. 选择需要减容的集群后点击 **确定** 按钮
![节点选择][shrink_2]

3. 显示当前数据组环境
![节点选择][shrink_4]

4. 根据需要进行节点删除，如删除整个数据分区组 group2，可以把 group2 的节点都选上，也可以直接点击左边分区组列表的 group2，选择完毕后点击 **下一步** 按钮
![节点选择][shrink_5]

5. 减容完成
![节点选择][shrink_7]

   > **Note:**  
   > - 服务减容功能仅支持 SequoiaDB 集群模式；  
   > - 如果删除整个数据组，要确保该组没有数据，否则该数据组将无法全部删除，会留下一个节点；
   > - 为了保证服务正常运行，不能删除全部 Coord 和 Catalog 节点。

##修改配置##

用户在 SequoiaDB 存储集群修改配置页面可以查看节点配置，并在线修改节点配置。

###集群模式###

1. 通过左侧导航【配置】选择 SequoiaDB 服务，点击进入配置页面，当前页面可以查看该服务的节点列表
![进入配置页面][sdb_config_1]

2. 修改节点配置有单节点修改和批量修改两种方式：

   - 批量修改 

      在表格中勾选需要修改配置的节点，选择后点击 **批量修改配置** 按钮，进入配置修改页面
       ![批量修改配置][sdb_config_2]

   - 单节点修改

      在表格中选择需要修改配置的节点，直接点击选中的列表，进入配置修改页面
       ![单节点修改配置][sdb_config_3]

3. 进入【批量节点配置】页面中可以进行修改配置、重启节点和查看详细配置操作
![配置][sdb_config_4]

   - 查看节点配置

     在该页面默认可以看到常用的配置项，点击 **查看详细配置** 后点击 **选择显示列** 按钮可以选择想要显示的配置项

   - 修改配置

       1. 点击 **修改配置** 按钮，在弹窗选择需要修改的配置项进行修改，配置项留空则表示默认值
   ![修改配置弹窗][sdb_config_5]

       2. 填写参数后点击 **确定** 按钮，如果修改的配置项中有需要重启生效的项，列表中会将需要重启生效的配置项标红显示，可以点击上方的 **重启节点** 按钮进行重启服务
   ![修改配置][sdb_config_6]

4. 修改完配置后回到【配置】页面，可以在节点列表中看到哪些节点的配置项发生了变化，有变化的节点前面会显示【变化】标识。
![修改完成][sdb_config_7]

###单机模式###

通过左侧导航【配置】选择 SequoiaDB 服务，点击进入配置页面，可以查看节点的常用配置项，以及进行修改配置 、重启节点、查看详细配置操作。
![进入配置页面][sdb_config_8]

- 查看节点详细配置

   在该页面默认可以看到常用的配置项，点击 **查看详细配置** 之后可以查看所有配置项

- 修改配置
  
   1. 点击 **修改配置** 按钮打开修改配置弹窗，选择需要修改的配置项进行修改
    ![修改配置弹窗][sdb_config_9]

   2. 填写好修改的配置项之后点击 **确定** 按钮，如果修改的配置项中有需要重启生效的项，列表中会将需要重启生效的配置项标红显示，可以点击上方的 **重启节点** 按钮进行重启服务
    ![修改完成][sdb_config_10]

##同步配置##

同步配置可以让修改后的 SequoiaDB 服务配置更新到 OM 中。

1. 进入【部署】页面，点击【存储集群操作】->【同步】按钮
![同步服务][sync_1]

2. 选择需要同步的存储集群
![同步服务][sync_2]

3. 同步完成
![同步服务][sync_4]

   > **Note:**
   >  
   > 同步完成后，配置发生变化的节点会在表格第一列显示修改、删除或新增的标识。

##设置鉴权##

用户可以通过 SDB Shell 或 SAC 设置服务鉴权，当用户通过 SDB Shell 设置服务鉴权后需要在 SAC 同步鉴权信息。

1. 进入 SDB Shell

   ```lang-bash
   $ /opt/sequoiadb/bin/sdb
   ```
2. 连接协调节点

   ```lang-javascript 
   > db = new Sdb( "localhost", 11810 )
   ```

3. 创建 SequoiaDB 鉴权，设置用户名为“root”，密码为“123”
   
   ```lang-javascript   
   > db.createUsr( "root", "123" )
   ```

4. 进入【部署】->【分布式存储】页面，集群报错 Authority is forbidden，错误码 -179
   ![设置鉴权][sdb_auth_1]

5. 点击存储集群的 **鉴权** 按钮，输入【用户名】和【密码】后点击 **确定** 按钮
   ![设置鉴权][sdb_auth_2]

6. SAC 显示恢复正常
   ![设置鉴权][sdb_auth_3]





[^_^]:
     本文使用的所有引用和链接
[extend_1]:images/SAC/Operation/Module/extend_1.png
[extend_2]:images/SAC/Operation/Module/extend_2.png
[extend_4]:images/SAC/Operation/Module/extend_4.png
[extend_5.png]:images/SAC/Operation/Module/extend_5.png
[extend_6]:images/SAC/Operation/Module/extend_6.png
[extend_8]:images/SAC/Operation/Module/extend_8.png
[shrink_1]:images/SAC/Operation/Module/shrink_1.png
[shrink_2]:images/SAC/Operation/Module/shrink_2.png
[shrink_4]:images/SAC/Operation/Module/shrink_4.png
[shrink_5]:images/SAC/Operation/Module/shrink_5.png
[shrink_7]:images/SAC/Operation/Module/shrink_7.png
[sync_1]:images/SAC/Operation/Module/sync_1.png
[sync_2]:images/SAC/Operation/Module/sync_2.png
[sync_4]:images/SAC/Operation/Module/sync_4.png
[sdb_auth_1]:images/SAC/Operation/Module/sdb_auth_1.png
[sdb_auth_2]:images/SAC/Operation/Module/sdb_auth_2.png
[sdb_auth_3]:images/SAC/Operation/Module/sdb_auth_3.png
[sdb_config_1]:images/SAC/Operation/Sequoiadb_Data/sdb_config_1.png
[sdb_config_2]:images/SAC/Operation/Sequoiadb_Data/sdb_config_2.png
[sdb_config_3]:images/SAC/Operation/Sequoiadb_Data/sdb_config_3.png
[sdb_config_4]:images/SAC/Operation/Sequoiadb_Data/sdb_config_4.png
[sdb_config_5]:images/SAC/Operation/Sequoiadb_Data/sdb_config_5.png
[sdb_config_6]:images/SAC/Operation/Sequoiadb_Data/sdb_config_6.png
[sdb_config_7]:images/SAC/Operation/Sequoiadb_Data/sdb_config_7.png
[sdb_config_8]:images/SAC/Operation/Sequoiadb_Data/sdb_config_8.png
[sdb_config_9]:images/SAC/Operation/Sequoiadb_Data/sdb_config_9.png
[sdb_config_10]:images/SAC/Operation/Sequoiadb_Data/sdb_config_10.png