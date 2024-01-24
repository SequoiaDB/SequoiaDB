
create collection 用于创建集合，必须指定集合所在的集合空间。

##语法##
***create collection \<cs_name\>.\<cl_name\> [ partition by (\<field1\>,...) ]***

##参数##
| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| cs_name | string | 集合空间名  | 是 |
| cl_name | string | 集合名  | 是 |
| field1  | string | 集合分区键 | 否 |
>**Note:**
>
> * 集合空间名、集合名长度不能超过 127Byte，并且不能为空。
> * 在同一个集合空间中不能存在相同的集合名。

##返回值##
无。

## 示例##

在集合空间 sample 下创建集合 employee 

```lang-javascript
//等价于db.sample.createCL("employee")
> db.execUpdate("create collection sample.employee") 
```