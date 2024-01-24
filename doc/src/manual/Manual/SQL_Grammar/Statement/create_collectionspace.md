
create collectionspace 用于创建数据库中的集合空间。

##语法##
***create collectionspace \<cs_name\>***

##参数##
| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| cs_name | string | 集合空间名  | 是 |
>**Note:**
>
> 集合空间名、集合名长度不能超过 127Byte，并且不能为空。

##返回值##
无。

## 示例##

创建集合空间 sample

```lang-javascript
//等价于db.createCS("sample")
> db.execUpdate("create collectionspace sample")
```