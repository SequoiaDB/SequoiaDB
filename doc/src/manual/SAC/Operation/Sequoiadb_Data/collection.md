本文档主要介绍在 SAC 中如何创建集合、删除集合、挂载集合、分离集合、切分数据和查看集合属性。

点击左侧导航【数据】进入所选服务的【集合】分页
![集合][attach_cl_1]

创建集合
----

1. 点击 **创建集合** 按钮，填写参数后点击 **确定** 按钮
![创建集合][create_cl_2]

2. 创建集合完成
![创建集合][create_cl_3]

集合属性
----

1. 点击需要查看的集合名，即可切换查看右边的集合信息
![集合属性][attribute2]

2. 集群模式下，集合有【Group】属性，可以切换复制组查看单个组的信息
![集合属性][attribute3]

3. 集群模式下，集合的分区类型是水平分区或者垂直分区，集合有【Partitions】属性，点击 **显示** 按钮可以查看相关的分区信息

   - 水平散列分区的集合分区信息
   ![集合属性][attribute4]

   - 垂直分区的集合分区信息
   ![集合属性][attribute5]


## 挂载集合

1. 创建一个垂直分区的集合 sample.main，分区键输入 name
![挂载集合][attach_cl_2]

2. 创建一个普通集合 sample.employee，该集合可以是水平范围分区也可以是水平散列分区
![挂载集合][attach_cl_3]

3. 创建完成
![挂载集合][attach_cl_4]

4. 点击 **挂载集合** 按钮，集合选垂直分区 sample.main，分区选 sample.employee，分区范围字段名输入 main 的分区键，范围取 1~100，点击 **确定** 按钮
![挂载集合][attach_cl_5]

   > **Note:**  
   > 图中红色方框的参数指的是分区键的数据类型，选择“auto”表示自动分配。

5. 挂载完成
![挂载集合][attach_cl_6]

6. 点击集合列中的集合名 sample.main，在集合属性【Partitions】点击 **显示** 按钮查看范围
![挂载集合][attach_cl_7]

## 分离集合

1. 存在垂直分区的集合 sample.main 和普通集合 sample.employee，并且把 sample.employee 挂载到 sample.main
![分离集合][detach_cl_1]

2. 点击 **分离集合** 按钮，集合选垂直分区 sample.main，分区选 sample.employee，点击 **确定** 按钮
![分离集合][detach_cl_2]

3. 分离完成
![分离集合][detach_cl_3]

4. 点击集合列中的集合名 sample.main，在集合属性【Partitions】点击 **显示** 按钮查看是否分离成功
![分离集合][detach_cl_4]

切分数据
----
相关文档：
- [分区类型][sharding_overview]            
- [分区键][sharding_shardingkey]           
- [分区索引][sharding_index]     
- [数据切分][data_split]         

1. 点击 **创建集合** 按钮，输入参数后点击 **确定** 按钮
![切分数据][split_2]

2. 创建集合完成
![切分数据][split_3]

3. 点击集合属性 Partitions 的 **显示** 按钮，查看当前集合所处复制组
![切分数据][split_4]

4. 点击 **切分数据** 按钮，选择切分的集合，源分区组选 group1，目标分区组演示中选择 group2，点击 **确定** 按钮
![切分数据][split_5]

5. 完成后，在点击集合属性【Partitions】的 **显示** 按钮，查看当前 group1 和 group2 的数据切分情况
![切分数据][split_6]

> **Note:**
> 
> - 文档仅仅演示切分数据的步骤。  
> - 切分数据需要根据实际服务来调整参数。

删除集合
----

用户执行删除集合操作会把该集合下的数据删除（包括记录和 Lob）。

点击 **删除集合** 按钮，选择需要删除的集合后点击 **确定** 按钮
![删除集合][drop_cl_2]


[^_^]:
    本文使用的所有引用及链接
[sharding_overview]:manual/Distributed_Engine/Architecture/Sharding/Readme.md
[sharding_shardingkey]:manual/Distributed_Engine/Architecture/Sharding/sharding_keys.md
[sharding_index]:manual/SAC/Operation/Sequoiadb_Data/collection_space.md
[data_split]:manual/Distributed_Engine/Architecture/domain.md

[create_cl_2]:images/SAC/Operation/Sequoiadb_Data/create_cl_2.png
[create_cl_3]:images/SAC/Operation/Sequoiadb_Data/create_cl_3.png
[attribute2]:images/SAC/Operation/Sequoiadb_Data/attribute2.png
[attribute3]:images/SAC/Operation/Sequoiadb_Data/attribute3.png
[attribute4]:images/SAC/Operation/Sequoiadb_Data/attribute4.png
[attribute5]:images/SAC/Operation/Sequoiadb_Data/attribute5.png
[attach_cl_1]:images/SAC/Operation/Sequoiadb_Data/attach_cl_1.png
[attach_cl_2]:images/SAC/Operation/Sequoiadb_Data/attach_cl_2.png
[attach_cl_3]:images/SAC/Operation/Sequoiadb_Data/attach_cl_3.png
[attach_cl_4]:images/SAC/Operation/Sequoiadb_Data/attach_cl_4.png
[attach_cl_5]:images/SAC/Operation/Sequoiadb_Data/attach_cl_5.png
[attach_cl_6]:images/SAC/Operation/Sequoiadb_Data/attach_cl_6.png
[attach_cl_7]:images/SAC/Operation/Sequoiadb_Data/attach_cl_7.png
[detach_cl_1]:images/SAC/Operation/Sequoiadb_Data/detach_cl_1.png
[detach_cl_2]:images/SAC/Operation/Sequoiadb_Data/detach_cl_2.png
[detach_cl_3]:images/SAC/Operation/Sequoiadb_Data/detach_cl_3.png
[detach_cl_4]:images/SAC/Operation/Sequoiadb_Data/detach_cl_4.png
[split_2]:images/SAC/Operation/Sequoiadb_Data/split_2.png
[split_3]:images/SAC/Operation/Sequoiadb_Data/split_3.png
[split_4]:images/SAC/Operation/Sequoiadb_Data/split_4.png
[split_5]:images/SAC/Operation/Sequoiadb_Data/split_5.png
[split_6]:images/SAC/Operation/Sequoiadb_Data/split_6.png
[drop_cl_2]:images/SAC/Operation/Sequoiadb_Data/drop_cl_2.png
