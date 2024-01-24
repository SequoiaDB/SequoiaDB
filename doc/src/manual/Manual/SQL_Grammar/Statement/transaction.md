
事务操作。

##语法##

开启事务： ***begin transaction***

提交事务： ***commit***

回滚事务： ***rollback***

##参数##
无

##返回值##
无 

## 示例##

- 开启事务，插入记录，提交事务 

   ```lang-javascript
   > db.execUpdate( "begin transaction" )
   > db.execUpdate( "insert into sample.employee(name) values(\"Tom\")" )
   > db.execUpdate( "commit" )
   ```

- 开启事务，插入记录，回滚事务 

   ```lang-javascript
   > db.execUpdate( "begin transaction" )
   > db.execUpdate( "insert into sample.employee(name) values('Tom')" )
   > db.execUpdate( "rollback" )
   ```