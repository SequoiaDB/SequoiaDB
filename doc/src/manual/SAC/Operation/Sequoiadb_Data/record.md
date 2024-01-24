本文档主要介绍在 SAC 中如何对记录进行插入、查询、更新、删除等操作。

显示模式
----

集合 sample.employee 存在以下记录：
![显示模式][display_mode_1]

- 显示模式默认是字符串模式，可以在记录表格的底部工具栏看到【A】符号是点亮的
![显示模式][display_mode_1]

- 点击第二个图标显示 JSON 树状图模式，点击记录是可以展开查看
![显示模式][display_mode_2]

- 点击第三个图标显示表格模式
![显示模式][display_mode_3]

插入
----

1. 存在集合 sample.employee，点击该集合的记录数
![插入][insert_1]

2. 进入记录操作页面
![插入][insert_2]

3. 点击 **插入** 按钮，打开插入记录的窗口
![插入][insert_3]

4. 通过图形化 JSON 编辑界面构造一条记录后点击 **确定** 按钮

   ```lang-json
   {
      "name": "Jack",
      "phone": "136123456",
      "address": "Guangzhou City, Guangdong Province"
   }
   ```

   ![插入][insert_4]

5. 记录插入完成
![插入][insert_5]

6. 点击 **插入** 按钮，打开插入记录的窗口，点击【A】符号，切换到字符串模式，输入一个标准的 JSON 字符串，点击 **确定** 按钮

   ```lang-json
   {
      "name": "Mike",
      "phone": "136456789",
      "address": "Guangzhou City, Guangdong Province"
   }
   ```

   ![插入][insert_6]

7. 记录插入完成
![插入][insert_7]

查询
----

集合 sample.employee 存在以下记录：
![查询][query_1]

1. 点击 **查询** 按钮，在查询窗口输入匹配条件后点击 **确定** 按钮
![查询][query_2]

2. 查询完成
![查询][query_3]

更新
----

集合 sample.employee 存在以下记录：
![更新][update_1]

1. 点击 **更新** 按钮，在更新窗口输入匹配条件和更新操作后点击 **确定** 按钮，如下示例是将字段 name 从“Jack”改为“Tom”：
![更新][update_2]

2. 更新完成
![更新][update_3]

删除
----

集合 sample.employee 存在以下记录：
![删除][delete_1]

1. 点击 **删除** 按钮，在删除窗口输入匹配条件后点击 **确定** 按钮
![删除][delete_2]

2. 删除完成
![删除][delete_3]

[^_^]:
    本文使用的所有引用及链接
[display_mode_1]:images/SAC/Operation/Sequoiadb_Data/display_mode_1.png
[display_mode_2]:images/SAC/Operation/Sequoiadb_Data/display_mode_2.png
[display_mode_3]:images/SAC/Operation/Sequoiadb_Data/display_mode_3.png
[insert_1]:images/SAC/Operation/Sequoiadb_Data/insert_1.png
[insert_2]:images/SAC/Operation/Sequoiadb_Data/insert_2.png
[insert_3]:images/SAC/Operation/Sequoiadb_Data/insert_3.png
[insert_4]:images/SAC/Operation/Sequoiadb_Data/insert_4.png
[insert_5]:images/SAC/Operation/Sequoiadb_Data/insert_5.png
[insert_6]:images/SAC/Operation/Sequoiadb_Data/insert_6.png
[insert_7]:images/SAC/Operation/Sequoiadb_Data/insert_7.png
[query_1]:images/SAC/Operation/Sequoiadb_Data/query_1.png
[query_2]:images/SAC/Operation/Sequoiadb_Data/query_2.png
[query_3]:images/SAC/Operation/Sequoiadb_Data/query_3.png
[update_1]:images/SAC/Operation/Sequoiadb_Data/update_1.png
[update_2]:images/SAC/Operation/Sequoiadb_Data/update_2.png
[update_3]:images/SAC/Operation/Sequoiadb_Data/update_3.png
[delete_1]:images/SAC/Operation/Sequoiadb_Data/delete_1.png
[delete_2]:images/SAC/Operation/Sequoiadb_Data/delete_2.png
[delete_3]:images/SAC/Operation/Sequoiadb_Data/delete_3.png
