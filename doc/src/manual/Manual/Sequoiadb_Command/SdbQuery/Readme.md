SdbQuery 类主要用于指定查询条件和操作查询结果，包含的函数如下：

| 名称 | 描述 |
|------|------|
| [arrayAccess()][arrayAccess] | 将结果集保存到数组中并获取指定下标记录 |
| [close()][close] | 关闭游标 |
| [count()][count] | 查询符合匹配条件的记录条数 |
| [current()][current] | 获取当前游标指向的记录 |
| [explain()][explain] | 获取查询的访问计划 |
| [flags()][flags] | 按指定的标志位遍历结果集 |
| [getQueryMeta()][getQueryMeta] | 获取查询元数据信息 |
| [hint()][hint] | 按指定的索引遍历结果集 |
| [limit()][limit] | 控制查询返回的记录条数 |
| [next()][next] | 获取当前游标指向的下一条记录 |
| [query()][query] | 以下标的方式访问查询结果集 |
| [remove()][remove] | 删除查询后的结果集 |
| [size()][size] | 返回当前游标到最终游标的记录条数 |
| [skip()][skip] | 指定结果集从哪条记录开始返回 |
| [sort()][sort] | 对结果集按指定字段排序 |
| [toArray()][toArray] | 以数组的形式返回结果集 |
| [update()][update] | 更新查询后的结果集 |

[^_^]:
     本文使用的所有引用及链接
[arrayAccess]:manual/Manual/Sequoiadb_Command/SdbQuery/arrayAccess.md
[close]:manual/Manual/Sequoiadb_Command/SdbQuery/close.md
[count]:manual/Manual/Sequoiadb_Command/SdbQuery/count.md
[current]:manual/Manual/Sequoiadb_Command/SdbQuery/current.md
[explain]:manual/Manual/Sequoiadb_Command/SdbQuery/explain.md
[flags]:manual/Manual/Sequoiadb_Command/SdbQuery/flags.md
[getQueryMeta]:manual/Manual/Sequoiadb_Command/SdbQuery/getQueryMeta.md
[hint]:manual/Manual/Sequoiadb_Command/SdbQuery/hint.md
[limit]:manual/Manual/Sequoiadb_Command/SdbQuery/limit.md
[next]:manual/Manual/Sequoiadb_Command/SdbQuery/next.md
[query]:manual/Manual/Sequoiadb_Command/SdbQuery/query.md
[remove]:manual/Manual/Sequoiadb_Command/SdbQuery/remove.md
[size]:manual/Manual/Sequoiadb_Command/SdbQuery/size.md
[skip]:manual/Manual/Sequoiadb_Command/SdbQuery/skip.md
[sort]:manual/Manual/Sequoiadb_Command/SdbQuery/sort.md
[toArray]:manual/Manual/Sequoiadb_Command/SdbQuery/toArray.md
[update]:manual/Manual/Sequoiadb_Command/SdbQuery/update.md