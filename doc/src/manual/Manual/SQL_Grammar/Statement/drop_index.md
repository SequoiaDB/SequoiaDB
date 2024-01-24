
drop index 用于删除集合中的索引。

##语法##
***drop index \<index_name\> on \<cs_name\>.\<cl_name\>***

##参数###
| 参数名| 参数类型 | 描述 | 是否必填 |
|-------|----------|------|----------|
| index_name | string | 索引名  | 是 |
| cs_name | string | 集合名  | 是 |
| cl_name | string | 集合名  | 是 |

##返回值##
无

##示例##

删除集合 sample.employee 中名为"test_index"的索引

```lang-javascript"
// 等价于 db.sample.employee.dropIndex("test_index")
> db.execUpdate("drop index test_index on sample.employee") 
```
