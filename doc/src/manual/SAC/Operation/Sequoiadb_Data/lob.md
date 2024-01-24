
1. 已存在集合 sample.employee
![Lob][lob_1]

2. 使用 coord 节点的主机，运行 SDB Shell 

   ```lang-bash
   $ /opt/sequoiadb/bin/sdb
   ```

3. 连接协调节点

   ```lang-javascript
   > db = new Sdb( "localhost", 11810 )
   ```

4. 导入 Lob

   ```lang-javascript
   > db.sample.employee.putLob( '/opt/pic.jpg' )
   ```

   输出 Lob 的 OID：

   ```lang-text
   5878b0add9d765d278000000
   ```

5. 导入完成
![Lob][lob_2]


[^_^]:
    本文使用的所有引用及链接
[lob_1]:images/SAC/Operation/Sequoiadb_Data/lob_1.png
[lob_2]:images/SAC/Operation/Sequoiadb_Data/lob_2.png