##名称##

split - 切分数据记录

##语法##

**db.collectionspace.collection.split(\<source group\>, \<target group\>, \<percent\>)**

**db.collectionspace.collection.split(\<source group\>, \<target group\>, \<condition\>, [endcondition])**

##类别##

SdbCollection

##描述##

该函数用于将源分区组中的数据记录，按指定条件切分到目标分区组中。源分区组与目标分区组必须属于同一个域。

##参数##

###范围切分###

db.collectionspace.collection.split(\<source group\>, \<target group\>, \<condition\>, [endcondition])

| 参数名 | 类型 | 描述   | 是否必填 |
| ------ | -------- | ------ | -------- |
| source group | string | 源分区组 | 是 |
| target group | string | 目标分区组 | 是 |
| condition | object | 范围切分条件| 是 |
| endcondition | object | 结束范围条件| 可选 |

> **Note:**
>
> - Range 分区使用精确条件，而 Hash 分区使用 Partition（分区数）条件。结束条件不选时默认为切分源当前包含的最大数据范围。  
> - 如果指定分区键字段为降序时，如：{groupingKey: {<字段1>: &lt; -1&gt;}，condition（或 Partition）中的起始条件中的范围应该大于终止条件中的范围。Hash 分区使用的 Partition（分区数）必须为整型，不能为其他的类型。

###百分比切分###

db.collectionspace.collection.split(\<source group\>, \<target group\>, \<percent\>)

| 参数名 | 类型 | 描述   | 是否必填 |
| ------ | -------- | ------ | -------- |
| source group | string | 源分区组 | 是 |
| target group | string | 目标分区组 | 是 |
| percent | double | 百分比切分条件，取值：(0, 100] | 是 |

> **Note:**
>
> - Range 分区需要保证源分区组中含有数据，即集合不为空。
> - 百分比不能为 0。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

* Hash 分区范围切分

    ```lang-javascript
    > db.sample.employee.split("group1", "group2", {Partition: 10}, {Partition: 20})
    ```

* Range 分区范围切分

    ```lang-javascript
    > db.sample.employee.split("group1", "group2", {a: 10}, {a: 10000})
    ```

* 百分比切分

    ```lang-javascript
    > db.sample.employee.split("group1", "group2", 50) 
    ```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
