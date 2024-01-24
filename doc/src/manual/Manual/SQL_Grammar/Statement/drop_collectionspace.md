
drop collectionspace 用于删除数据库中指定的集合空间。

##语法##
***drop collectionspace \<cs_name\>***

##参数##
| 参数名| 参数类型 | 描述 | 是否必填 |
|-------|----------|------|----------|
| cs_name | string | 集合空间名  | 是 |
> **Note:**
>
> 该集合空间必须在数据库中存在。

##返回值##
无

##示例##

在数据库中删除名为"sample"的集合空间

```lang-javascript
//等价于 db.dropCS("sample")
> db.execUpdate("drop collectionspace sample") 
```