
当查询的集合有多个索引时，SequoiaDB 需要选取合适的索引，或者全表扫描来执行查询。数据节点上的查询优化器会基于代价对候选的访问计划进行评估，选取合适的访问计划来完成查询。

估算出每个候选访问计划执行的以下指标：

1.  基于规则的估算选取候选访问计划

    1. 索引的选择率 < 0.1（即索引过滤剩下的记录个数为集合记录个数的 10%）
    2. 索引完全匹配排序字段及排序方向
    3. 全表扫描

2.  符合指标 1 的候选访问计划，再基于代价的进行估算，最终选出总代价最小的访问计划执行查询。

**示例**

集合 sample.employee 存在以下索引：

1. "index_a" : ```{ a : 1 }```
2. "index_b" : ```{ b : 1 }```
3. "index_c" : ```{ c : 1 }```

查询 ```db.sample.employee.find( { a : 1, b : 2 } ).sort( { c : 1 } )``` 可以有以下的访问计划：

1.  IXSCAN( "index_a" ) ==> SORT( { c : 1 } )
2.  IXSCAN( "index_b" ) ==> SORT( { c : 1 } )
3.  IXSCAN( "index_c" )
4.  TBSCAN() ==> SORT( { c : 1 } )

根据指标 1 可以确定 4 个都是候选的访问计划，其中访问计划 1 和 2 满足指标 1.1，访问计划 3 满足指标 1.2，访问计划 4 满足指标 1.3。

然后通过代价估算确定总代价最小的访问计划，并选取执行查询。假设估算出 4 个候选访问计划的总代价分别为 1000，800，12000 和 1000，则最终选择访问计划 2 执行查询。

##索引选择率的估算##

索引选择率的估算有[使用统计信息进行估算][statistics]和使用默认值进行估算两种方式。

**使用默认值进行估算**

* 数值

    1. 在 ```[ -99999999.9, 99999999.9 ]``` 的区间中选取
    2. 如 ```{ $lt : 0 }``` 的选择率为：```( 0 - ( -99999999.9 ) ) / ( 99999999.9 - ( -99999999.9 ) ) = 0.5```

* 字符串

    1. 逐个字符计算在 ' ' （空格 ASCII 码：32）至 ASCII 码 127 之间的比例
    2. 计算前 20 个字符

* 其他数据类型

   1. 相等比较：0.005
   2. 大于、小于比较：0.333333
   3. 范围比较：0.05

##访问计划的搜索过程##

用户可以通过 [SdbQuery.explain\(\)][explain] 查看查询的访问计划。

当 SdbQuery.explain() 的 Search 选项为 true 时，将会展示查询优化器搜索过的访问计划，并查看查询优化器选择的结果。
当 SdbQuery.explain() 的 Evaluate 选项为 true 时，将会展示查询优化器估算访问计划总代价的演算过程。

*  [访问计划的搜索过程][search]
*  [TBSCAN的推演公式][TBSCAN]
*  [IXSCAN的推演公式][IXSCAN]
*  [SORT的推演公式][SORT]

>   **Note:**
>
>  - 搜索过的访问计划不在访问计划缓存中，因此 Search 选项不使用缓存，重新估算
>  - 搜索过程将嵌套展示在数据节点每个集合的访问计划中


[^_^]:
     本文使用的所有引用和链接
[statistics]:manual/Distributed_Engine/Maintainance/Access_Plan/statistics.md
[explain]:manual/Manual/Sequoiadb_Command/SdbQuery/explain.md
[search]:manual/Manual/Cost_Estimation/search.md
[TBSCAN]:manual/Manual/Cost_Estimation/TBSCAN.md
[IXSCAN]:manual/Manual/Cost_Estimation/IXSCAN.md
[SORT]:manual/Manual/Cost_Estimation/SORT.md