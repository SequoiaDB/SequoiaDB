[^_^]:
     集合操作

下述以名为“employee”的集合为例，介绍集合的相关操作。

##创建集合##

用户可根据需求，创建不同属性的集合。

- 在集合空间 sample 中创建名为“employee”的普通集合

    ```lang-javascript
    > db.sample.createCL("employee")
    ```

- 在集合空间 sample 中创建名为“employee”的分区集合，指定分区类型为“hash”，分区键为字段 age，并设置该集合开启自动切分功能

    ```lang-javascript
    > db.sample.createCL("employee", {ShardingType: "hash", ShardingKey: {age: 1}, AutoSplit: true})
    ```

> **Note:**
>
> 创建集合空间的详细参数说明可参考 [createCL()][createCL]。

##CRUD 操作##

用户可通过 [insert()][insert]、[find()][find]、[update()][update] 和 [remove()][remove] 对集合中的数据进行插入、查询、更新和删除操作。 

###插入###

向集合 sample.employee 中插入如下记录：

```lang-javascript
> db.sample.employee.insert({id: 1, name: "Tom", age: 20})
> db.sample.employee.insert({id: 2, name: "Betty", age: 28})
> db.sample.employee.insert({id: 3, name: "Mark", age: 23})
```

###查询###

查询集合 sample.employee 中的记录

```lang-javascript
> db.sample.employee.find()
```

输出结果如下：

```lang-json
{
  "_id": {
    "$oid": "60f93410bf0a61908ef9f364"
  },
  "name": "Tom",
  "age": 20,
  "id": 1
}
{
  "_id": {
    "$oid": "60f93420bf0a61908ef9f365"
  },
  "name": "Betty",
  "age": 28,
  "id": 2
}
{
  "_id": {
    "$oid": "60f93427bf0a61908ef9f366"
  },
  "name": "Mark",
  "age": 23,
  "id": 3
}
Return 3 row(s).
```

###更新###

匹配集合 sample.employee 中 id 为 1 的记录，将该记录的 name 字段值改为"Jack"

```lang-javascript
> db.sample.employee.update({$set: {name: "Jack"}}, {id: {$et: 1}})
```

查看更新后的记录

```lang-javascript
> db.sample.employee.find()
{
  "_id": {
    "$oid": "60f93410bf0a61908ef9f364"
  },
  "age": 20,
  "id": 1,
  "name": "Jack"
}
{
  "_id": {
    "$oid": "60f93420bf0a61908ef9f365"
  },
  "name": "Betty",
  "age": 28,
  "id": 2
}
{
  "_id": {
    "$oid": "60f93427bf0a61908ef9f366"
  },
  "name": "Mark",
  "age": 23,
  "id": 3
}
Return 3 row(s).
```

###删除###

删除集合 sample.employee 中 age 为 23 的记录

```lang-javascript
> db.sample.employee.remove({{age: {$et: 23}})
```

##修改集合属性##

修改集合 sample.employee 的压缩类型为“snappy”

```lang-javascript
> db.sample.employee.setAttributes({CompressionType: "snappy"})
```

> **Note:**
>
> 修改集合属性的详细参数说明可参考 [setAttributes()][setAttributes]。

##删除集合##

删除集合空间 sample 下的集合 employee

```lang-javascript
> db.sample.dropCL({"employee"})
```

> **Note:**
>
> 修改集合的详细参数说明可参考 [dropCL()][dropCL]。

##参考##

更多集合空间操作可参考 [SdbCollection][cl]。





[^_^]:
      本文使用的所有引用及链接
[createCL]:manual/Manual/Sequoiadb_Command/SdbCS/createCL.md
[insert]:manual/Manual/Sequoiadb_Command/SdbCollection/insert.md
[find]:manual/Manual/Sequoiadb_Command/SdbCollection/find.md
[update]:manual/Manual/Sequoiadb_Command/SdbCollection/update.md
[remove]:manual/Manual/Sequoiadb_Command/SdbCollection/remove.md
[setAttributes]:manual/Manual/Sequoiadb_Command/SdbCollection/setAttributes.md
[dropCL]:manual/Manual/Sequoiadb_Command/SdbCS/dropCL.md
[cl]:manual/Manual/Sequoiadb_Command/SdbCollection/Readme.md