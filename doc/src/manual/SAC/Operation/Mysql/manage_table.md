
点击左侧导航栏 **数据**，选择指定的数据库实例查看当前用户表，点击所选表名可以查看【表信息】页面。用户通过该页面可以查看当前数据表的字段结构及进行添加字段、删除字段、修改字段名、修改字段类型、设置默认值、删除默认值、设置主键、移除主键等操作。

添加字段 
----

点击 **添加字段** 按钮，填写参数后点击 **确定** 按钮添加字段

> **Note:**
>
> 用户点击窗口右侧的【+】符号可以添加多个字段。

![添加字段][add_field]

删除字段
----

从字段列表中选择需要删除的字段点击【X】符号，确认无误后点击 **确定** 按钮删除字段
![删除字段][drop_field]

修改字段
----

从字段列表中选择需要修改的字段，点击【编辑图标】->【修改字段】按钮，填写参数后点击 **确定** 按钮修改字段
![修改字段][rename_field]

设置默认值
----

从字段列表中选择需要设置默认值的字段，点击【编辑图标】->【设置默认值】按钮，填写默认值后点击 **确定** 按钮设置默认值
![设置默认值][set_default]

删除默认值
----

从字段列表中选择需要删除默认值的字段，点击【编辑图标】->【删除默认值】按钮，确认无误后点击 **确定** 按钮删除默认值
![删除默认值][drop_default]

创建索引
----

点击【索引操作】->【创建索引】按钮，填写参数后点击 **确定** 按钮创建索引
![创建索引][create_index]

删除索引
----

点击【索引操作】->【删除索引】按钮，选择需要删除的索引后点击 **确定** 按钮删除索引
![删除索引][drop_index]

设置主键
----

点击【索引操作】->【设置主键】按钮，选择需要设置为主键的字段后点击 **确定** 按钮设置主键
![设置主键][set_primary]

> **Note:**
>  
> 如果建表时设置了分区键，设置的主键必须包含分区键的字段。

移除主键
----

点击【索引操作】->【删除主键】按钮，确认无误后点击 **确定** 按钮移除主键
![移除主键][drop_primary]



[^_^]:
     本文使用的所有引用和链接
[add_field]:images/SAC/Operation/Mysql/add_field.png
[drop_field]:images/SAC/Operation/Mysql/drop_field.png
[rename_field]:images/SAC/Operation/Mysql/rename_field.png
[set_default]:images/SAC/Operation/Mysql/set_default.png
[drop_default]:images/SAC/Operation/Mysql/drop_default.png
[create_index]:images/SAC/Operation/Mysql/create_index.png
[drop_index]:images/SAC/Operation/Mysql/drop_index.png
[set_primary]:images/SAC/Operation/Mysql/set_primary.png
[drop_primary]:images/SAC/Operation/Mysql/drop_primary.png
