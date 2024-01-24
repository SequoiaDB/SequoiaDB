[^_^]:   
    集群缩容
    作者：黄文华   
    时间：20190527
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190716


当机器中存在多个数据节点，减少机器中的数据节点个数，从而达到集群节点缩容的效果。下图为服务器内节点缩容架构图:    
![集群节点缩容架构图][less_node]  

### 节点缩容 ###

将上图中 sdbserver1、sdbserver2 和 sdbserver3 三台机器中的复制组 group2 删除，集群中只保留复制组 group1，且需要保留复制组 group2 中的数据。方案有两种：
- 使用 [sdbexprt][sdbexport] 命令将包含 group2 的所有集合导出，然后删除 group2 及 group2 上的所有集合，重新创建只属于 group1 的同名集合，并将导出的集合使用 [sdbimprt][sdbimport] 命令导入
- 使用数据切割 [split][split] 工具将 group2 的数据切割到 group1 中，无需手工删除集合

以下示例为使用 [split][split] 工具将集合中的数据从 group2 切割到 group1，当确保 group2 没有数据时就能够把此数据组从集群中剔除。  

1. 连接协调节点

   ```lang-javascript
   > db = new Sdb("sdbserver1",11810)  
   ```

2. 获取集群中的所有域

   ```lang-javascript
   > db.listDomains()
   ```

3. 获取使用了 group2 的域

   ```lang-javascript
   > var domain = db.getDomain("domainName")
   ```

4. 查看指定域下的集合

   ```lang-javascript
   > domain.listCollections()
   ```

5. 查看集合 sample.employee 的编目信息快照信息，获取集合 sample.employee 中 group2 的 Partition 范围

   ```lang-javascript
   > db.snapshot(SDB_SNAP_CATALOG,{"Name":"sample.employee"})
   ```

6. 使用 split 工具将集合 sample.employee 中 group2 的数据切割至 group1 

   ``` lang-javascript
   > db.sample.employee.split("group2","group1",{"Partition":2084},{"Partition":4096})  
   ```

   >**Note:**
   >
   > 用户使用 split 工具切割数据时，集群中至少需要存在两个复制组。

7. 删除 group2 中所有节点

   ```lang-javascript
   > var rg2 = db.getRG("group2")
   > rg2.removeNode("sdbserver1", 11830)
   > rg2.removeNode("sdbserver2", 11830)
   > rg2.removeNode("sdbserver3", 11830)
   ```

   >**Note:**
   >
   > 如果用户需要保存节点的数据，在执行 removeNode 删除节点前，应先使用 [sdbexprt][sdbexport] 工具对集合数据进行导出。

   如需要强制删除 group2 中的节点，在参数配置中添加 enforced 值为 true

   ```lang-javascript
   > rg2.removeNode("sdbserver1", 11820, {enforced:true})  
   ```

8. 删除 group2

   ```lang-javascript
   > db.removeRG("group2")
   ```

### 缩容后检查

- 节点缩容后使用 sdblist 命令检查复制组是否已删除

   ```lang-bash 
   sdblist -l -t all
   ```

- 使用快照查看集合信息中 GroupName 是否已不包含 group2

   ```lang-javascript
   > db.snapshot(SDB_SNAP_CATALOG,{"Name":"sample.employee"})
   ```

  

[^_^]:
    本文使用到的所有链接及引用。
[sdbimport]:manual/Distributed_Engine/Maintainance/Migration/csv_import.md
[sdbexport]:manual/Distributed_Engine/Maintainance/Migration/export.md
[collection_space]:manual/Distributed_Engine/Architecture/Data_Model/collection_space.md
[collection]:manual/Distributed_Engine/Architecture/Data_Model/collection.md
[split]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/split_merge.md
[less_node]:images/Distributed_Engine/Maintainance/Expand/less_node.PNG

