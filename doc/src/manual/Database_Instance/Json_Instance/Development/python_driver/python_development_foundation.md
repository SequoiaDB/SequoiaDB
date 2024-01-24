本文档主要介绍如何使用 Python 客户端驱动接口编写使用 SequoiaDB 数据库的程序。完整的示例代码请参考 SequoiaDB 安装目录下的 `samples/Python`

> **Note:** 
> 
> * 在 Python 中构造 BSON 时默认使用 dict，dict 的字段是无序的。如果要求 BSON 中的字段顺序与输入顺序一致（例如，创建索引时索引键的定义），需要使用 collections.OrderedDict。
>
> * 驱动的接口详情请参考 [Python API][api]

##连接数据库##

* 连接 SequoiaDB 数据库，之后即可访问、操作数据库
  
   ```lang-python
   from pysequoiadb import *

   host = 'localhost'
   port = 11810
   user = "admin"
   psw = "admin"

   # 连接数据库
   db = client(host, port, user, psw)
   try:
      # 使用连接对象访问、操作数据库
      # ...
      pass
   finally:
      db.disconnect()
   ```

* 使用多地址连接数据库

   ```lang-python
   host_list = [{'host': 'sdbserver1', 'service': 11810}, {'host': 'sdbserver2', 'service': 11810}]
   db = client( host_list=host_list, policy='random' )
   ```

* 使用密码文件连接数据库

   ```lang-python
   cipher_file="/opt/sequoiadb/cipher"
   db = client(host, port, user, cipher_file=cipher_file)
   ```

   > **Note:**
   >
   > - 密码文件的使用，请参考[密码管理工具](manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbpasswd.md)

##数据库操作##

* 创建集合空间和集合
  
   ```lang-python
   # 创建集合空间
   cs_name = 'sample'
   cs = db.create_collection_space(cs_name)
   # 创建集合
   cl_name = 'employee'
   cl = cs.create_collection(cl_name)
   ```

* 插入数据

   ```lang-python
   # 准备待插入的数据
   record = {"name":"Tom", "age":24}
   # 插入数据
   result = cl.insert_with_flag(record)
   print(result)
   ```
  
* 查询

   ```lang-python
   cr = cl.query()
   # 遍历游标获取查询的结果
   while True:
      try:
         record = cr.next()
         print(record) 
      except SDBEndOfCursor:
         break
   cr.close()
   ```

* 索引

  在 cl 中创建一个以 “name” 为升序、“age” 为降序的索引

   ```lang-python
   index_name = "index_name"
   idx = OrderedDict([('name', 1), ('age', -1)])
   cl.create_index (idx, index_name, False, False) 
   ```
  
* 更新
 
   ```lang-python
   modifier = {"$set":{"age":19}}
   # 更新 cl 中的所有记录
   result = cl.update(modifier)
   print(result)
   ```

* 删除

   ```lang-python
   cond = {"age":19}
   result = cl.delete(condition=cond)
   print(result)
   ```

##集群操作##

集群操作主要涉及复制组与节点。如下以创建复制组、获取节点为例

* 创建复制组

   ```lang-python
   rg = db.create_replica_group("dataGroup")
   # 创建一个数据节点
   rg.create_node('sdbserver', '11820', "/opt/sequoiadb/database/data/11820")
   # 启动复制组
   rg.start ()
   ```

* 获取节点
  
   ```lang-python
   # 获取复制组 dataGroup
   rg = db.get_replica_group_by_name("dataGroup")
   # 获取主节点
   master = rg.get_master() 
   ```

[api]:api/python/html/index.html
