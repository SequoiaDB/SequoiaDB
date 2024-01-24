
序列列表可以列出当前数据库的全部序列信息。

> **Note:** 
>
> 该列表只支持 coord 节点上使用。

##标识##

$LIST_SEQUENCES

##字段信息##

| 字段名         | 类型   | 描述                        |
| -------------- | ------ | --------------------------- |
| Name           | string | 序列名                      |

##示例##

查看序列列表

```lang-javascript
> db.exec( "select * from $LIST_SEQUENCES" )
```

输出结果如下：

```lang-json
{
  "Name": "SYS_2469606195201_studentID_SEQ"
}
```
