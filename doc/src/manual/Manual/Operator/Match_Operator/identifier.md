
##语法##

```lang-json
{ <字段名.$+标识符>: <value> }
```


##描述##

$+标识符是一种特殊的命令符，只作用于数组对象，用于代替数组元素的索引，并且可以把匹配到的第一个索引值传递到 [update][up] 的 rule 参数中。

标识符相当于临时的存储，会将匹配成功的数组元素索引进行存储。标识符须为整数，错误的书写格式如 $5.4、$a2、$3c、$MA 等。

##示例##

###数组元素为普通值###

集合 sample.employee 存在如下记录：

```lang-javascript
{a:[1,2,2,4,5]}
{a:[1,4,5]}
{a:[4,2,1]}
```

* 查询 a 字段中存在元素 5 的记录

   ```lang-javascript
   > db.sample.employee.find({"a.$1":5},{"a":1})
   {
       "a": [
         1,
         2,
         2,
         4,
         5
       ]
   }
   {
       "a": [
         1,
         4,
         5
       ]
   }
   Return 2 row(s).
   ```

* 操作 a 字段，将值为 4 的元素修改为 100

   ```lang-javascript
   > db.sample.employee.update({$set:{"a.$1":100}},{"a.$1":4})
   ```

   此操作后，记录更新结果如下：
  
   ```lang-json
   {a:[1,2,2,100,5]}
   {a:[1,100,5]}
   {a:[100,2,1]}
   ```

* 操作 a 字段，将值为 2 的元素修改为 2000

   ```lang-javascript
   > db.sample.employee.update({$set:{"a.$1":2000}},{"a.$1":2})
   ```

   此操作后，记录更新结果如下：
   
   ```lang-json
   {a:[1,2000,2,4,5]}
   {a:[1,4,5]}
   {a:[4,2000,1]}
   ```

   > **Note:**
   >
   > 如果同一数组记录中存在多个符合匹配条件的元素，则操作仅对匹配到的第一个元素有效。

*  操作 a 字段，将值为 4 的元素修改为 600，将值为 1 的元素修改为 200

   ```lang-javascript
   > db.sample.employee.update({$set:{"a.$1":600,"a.$2":200}},{"a.$1":4,"a.$2":1})
   ```

   此操作后，记录更新结果如下：

   ```lang-json
   {a:[200,2,2,600,5]}
   {a:[200,600,5]}
   {a:[600,2,200]}
   ```

###数组元素为嵌套对象###

集合 sample.employee 存在如下记录：
  
```lang-json
{a:[{id:2}]}
{a:[{id:1},{id:3,num:3}, {id:4,num:4}]}
{a:[{id:1,num:1}, {id:2,num:2}, {id:3,num:3}, {id:4,num:4}, {id:2}]}
```

- 查询 a 字段中存在元素{id:2}的记录

   ```lang-javascript
   > db.sample.employee.find({"a.$1":{"id":2}})
   {
     "_id": {
       "$oid": "601136504328ca603a7fffe6"
     },
     "a": [
       {
         "id": 2
       }
     ]
   }
   {
     "_id": {
       "$oid": "6011367d4328ca603a7fffe8"
     },
     "a": [
       {
         "id": 1,
         "num": 1
       },
       {
         "id": 2,
         "num": 2
       },
       {
         "id": 3,
         "num": 3
       },
       {
         "id": 4,
         "num": 4
       },
       {
         "id": 2
       }
     ]
   }
   Return 2 row(s).
   ```

- 操作 a 字段，使用 [$elemMatch][match] 匹配数组中存在{id:2}的记录，并将该记录的 id 修改为 100

   ```lang-javascript
   > db.sample.employee.update({$set:{"a.$1.id":100}},{"a.$1":{$elemMatch:{"id":2}}})
   ```

   此操作后，记录更新结果如下：

   ```lang-json
   {a:[{id:100}]}
   {a:[{id:1},{id:3,num:3}, {id:4,num:4}]}
   {a:[{id:1,num:1}, {id:100,num:2}, {id:3,num:3}, {id:4,num:4}, {id:2}]}
   ```

   > **Note:**
   >
   > 如果同一数组记录中存在多个符合匹配条件的元素，则操作仅对匹配到的第一个元素有效。

- 操作 a 字段，匹配数组中存在{"id":1,"num":1}的记录，并将该记录的 id 修改为 500，num 修改为 300

   ```lang-javascript
   > db.sample.employee.update({$set:{"a.$1.id":500,"a.$1.num":300}},{"a.$1":{"id":1,"num":1}})
   ```

   此操作后，记录更新结果如下：

   ```lang-json
   {a:[{id:2}]}
   {a:[{id:1},{id:3,num:3}, {id:4,num:4}]}
   {a:[{id:500,num:300}, {id:2,num:2}, {id:3,num:3}, {id:4,num:4}, {id:2}]}
   ```

- 操作 a 字段，匹配数组中存在{"id":3,"num":3}的记录，将该记录的 id 修改为 30；匹配数组中存在{"id":4,"num":4}的记录，将该记录的 num 修改为 40

   ```lang-javascript
   > db.sample.employee.update({$set:{"a.$1.id":30,"a.$2.num":40}},{"a.$1":{"id":3,"num":3},"a.$2":{"id":4,"num":4}})
   ```

   此操作后，记录更新结果如下：

   ```lang-json
   {a:[{id:2}]}
   {a:[{id:1},{id:30,num:3}, {id:4,num:40}]}
   {a:[{id:1,num:1}, {id:2,num:2}, {id:30,num:3}, {id:4,num:40}, {id:2}]}
   ```





[^_^]:
     本文使用的所有引用及链接
[up]:manual/Manual/Sequoiadb_Command/SdbCollection/update.md
[match]:manual/Manual/Operator/Match_Operator/elemMatch.md