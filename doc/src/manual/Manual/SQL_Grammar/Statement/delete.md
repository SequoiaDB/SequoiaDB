
delete 用于删除集合中的记录。

##语法##
***delete from \<cs_name\>.\<cl_name\> [where \<condition\>]***

##参数###
| 参数名 | 参数类型 | 描述 | 是否必填 |
|--------|----------|------|----------|
| cs_name | string | 集合空间名  | 是 |
| cl_name | string | 集合名  | 是 |
| condition| expression | 匹配条件  | 是 |

##返回值##
无 

##示例##

集合 sample.employee 中原始记录如下所示：

```lang-json
{a:10, b:10}
{a:20, b:20}
{a:30, b:30}
```

删除集合 sample.employee 中的数据 

```lang-javascript
// 删除符合条件a < 10的记录
> db.execUpdate("delete from sample.employee where a < 10")

// 删除集合 sample.employee 中的所有记录
> db.execUpdate("delete from sample.employee")
```