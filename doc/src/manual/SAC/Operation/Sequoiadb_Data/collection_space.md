本文档主要介绍在 SAC 中如何创建集合空间、查看集合空间属性和删除集合空间。


点击左侧导航【数据】进入所选服务的【集合空间】分页
![集合空间][create_cs_1]


创建集合空间
----

1. 点击 **创建集合空间**，在提示窗口输入集合空间名后点击 **确定** 按钮
![创建集合空间][create_cs_2]

2. 创建集合空间完成
![创建集合空间][create_cs_3]

集合空间属性
----

1. 点击需要查看的集合空间名，即可切换查看右边的集合空间信息
![集合空间属性][attribute_2]

  没有集合的集合空间，属性为空
   ![集合空间属性][attribute_3]

2. 集群模式下，集合空间有【Group】属性，可以切换复制组查看单个组的信息
![集合空间属性][attribute_4]

 ![集合空间属性][attribute_5]

删除集合空间
----
  
用户执行删除集合空间操作会把该集合空间下的集合一并删除，包括集合中的数据。

1. 点击 **删除集合空间** 按钮，选择要删除的集合空间后点击 **确定** 按钮
![删除集合空间][drop_cs_2]

2. 删除集合空间完成
![删除集合空间][drop_cs_3]




[^_^]:
    本文使用的所有引用及链接
[create_cs_1]:images/SAC/Operation/Sequoiadb_Data/create_cs_1.png
[create_cs_2]:images/SAC/Operation/Sequoiadb_Data/create_cs_2.png
[create_cs_3]:images/SAC/Operation/Sequoiadb_Data/create_cs_3.png
[attribute_2]:images/SAC/Operation/Sequoiadb_Data/attribute_2.png
[attribute_3]:images/SAC/Operation/Sequoiadb_Data/attribute_3.png
[attribute_4]:images/SAC/Operation/Sequoiadb_Data/attribute_4.png
[attribute_5]:images/SAC/Operation/Sequoiadb_Data/attribute_5.png
[drop_cs_2]:images/SAC/Operation/Sequoiadb_Data/drop_cs_2.png
[drop_cs_3]:images/SAC/Operation/Sequoiadb_Data/drop_cs_3.png