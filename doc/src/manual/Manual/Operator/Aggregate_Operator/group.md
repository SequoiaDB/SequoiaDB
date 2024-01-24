##语法##

```lang-json
{ 
   $group: 
   { 
      _id: "$分组字段名", 
      显示字段名: { 聚集函数: "$字段名" }, [ 显示字段名2: { 聚集函数: "$字段名" }, ... ] 
   } 
}
```

##说明##

$group 实现对结果集的分组，类似 SQL 中的 group by 语句。

首先指定分组键（_id），通过 _id 来标识分组字段，分组字段可以是单个，也可以是多个。

* 单个分组键

   ```lang-json
   { 
      _id: "$field" 
   }
   ```

* 多个分组键

   ```lang-json
   { 
      _id: 
      { 
         field1: "$field1", 
         field2: "$field2", 
         ... 
      } 
   }
   ```

> Note：

> $group 是按照分组键指定的字段对结果集进行分组。如果不指定分组键，结果集不会进行分组，会返回所有的记录。如果分组键指定的字段不存在，默认返回集合的第一条记录。

##示例##

1. 在集合 sample.employee 中插入以下数据：

   ```lang-javascript
   > db.sample.employee.insert( [ { name: "Tom", score: 95, major: "Electronic Commerce", college: "College of Economics Finance" }, { name: "Alice", score: 90, major: "Accounting Profession", college: "College of Economics Finance" }, { name: "Ben", score: 85, major: "Financial Management", college: "College of Economics Finance" }, { name: "Jack", score: 80, major: "Software Engineering", college: "College of Computer Science" }, {name: "Jerry", score: 75, major: "Network Engineering", college: "College of Computer Science" }, { name: "Smith", score: 70, major: "Information Engineering", college: "College of Computer Science" } ] )
   ```

2. 从集合 sample.employee 中读取记录，并按 college 字段进行分组，取每个分组第一条记录的 college 字段，输出字段名为 College；每个分组中的 score 字段值求平均值，输出字段名为“Avg_score”

   ```lang-javascript
   > db.sample.employee.aggregate( { $group: { _id: "$college", Avg_score: { $avg: "$score" }, College: { $first: "$college" } } } )
   ```      

   输出结果如下：

   ```lang-javascript
   {
      "Avg_score": 75,
      "College": "College of Computer Science"
   }
   {
      "Avg_score": 90,
      "College": "College of Economics Finance"
   }
   ```

##$group支持的聚集函数##

|  函数名   |                   描述                                             |
| --------- | ------------------------------------------------------------------ |
| $addtoset | 将字段添加到数组中，相同的字段值只会添加一次                       |
| $first    | 取分组中第一条记录中的字段值                                       |
| $last     | 取分组中最后一条记录中的字段值                                     |
| $max      | 取分组中指定字段的最大值                                           |
| $min      | 取分组中指定字段的最小值                                           |
| $avg      | 取分组中指定字段值的平均值                                         |
| $push     | 将所有字段添加到数组中，即使数组中已经存在相同的字段值，也继续添加 |
| $sum      | 取分组中指定字段值的总和                                           |
| $count    | 对记录分组后，返回表所有的记录条数                                 |


##$addtoset##

记录分组后，使用 $addtoset 将指定字段值添加到数组中，相同的字段值只会添加一次。对嵌套对象使用点操作符（.）引用字段名。

**示例**

从集合 sample.employee 中读取记录，按 college 字段值进行分组，并通过 $first 获取每个分组第一条记录的 college 字段，输出字段名为 College；使用 $addtoset 把 major 字段值放入数组中返回，数组名为“addtoset_major”

```lang-javascript
> db.sample.employee.aggregate( { $group: { _id: "$college", College: { $first: "$college" },  addtoset_major: { $addtoset: "$major" } } } )
```      

输出结果如下：

```lang-json
{
   "College": "College of Computer Science",
   "addtoset_major": [
     "Software Engineering",
     "Network Engineering",
     "Information Engineering"
   ]
}
{
   "College": "College of Economics Finance",
   "addtoset_major": [
     "Electronic Commerce",
     "Accounting Profession",
     "Financial Management"
   ]
}
```

##$count##

记录分组后，用 $count 取出分组所包含的总记录条数。

**示例**

从集合 sample.employee 中读取记录，返回表中包含有 college 字段的所有记录的数量

```lang-javascript
> db.sample.employee.aggregate( { $group: { Total: { $count: "$college" } } } )
```      

输出结果如下：

```lang-json
{
   "Total": 6
}
```

##$first##

记录分组后，取分组中第一条记录指定的字段值，对嵌套对象使用点操作符（.）引用字段名。

**示例**

从集合 sample.employee 中读取记录，对记录按 college 字段分组，取每个分组中第一条记录的 college 字段值和  name 字段值，输出字段名分别为 College 和 Name

```lang-javascript
> db.sample.employee.aggregate( { $group: { _id: "$college", College: { $first: "$college" }, Name: { $first: "$name" } } } )
```      

输出结果如下：

```lang-json
{
   "College": "College of Computer Science",
   "Name": "Jack"
}
{
   "College": "College of Economics Finance",
   "Name": "Tom"
}
```

##$avg##

记录分组后，取分组中指定字段的平均值返回，对嵌套对象使用点操作符（.）引用字段名。

**示例**

从集合 sample.employee 中读取记录，并按 college 字段进行分组，取每个分组的第一条记录的 college 字段，输出字段名为 College；每个分组中的 score 字段值求平均值，输出字段名为“Avg_score”

```lang-javascript
> db.sample.employee.aggregate( { $group: { _id: "$college", Avg_score: { $avg: "$score" }, College: { $first: "$college" } } } )
```      

输出结果如下：

```lang-json
{
   "Avg_score": 75,
   "College": "College of Computer Science"
}
{
   "Avg_score": 90,
   "College": "College of Economics Finance"
}
```

##$max##

记录分组后，取分组中指定字段的最大值返回，对嵌套对象使用点操作符（.）引用字段名。

**示例**

从集合 sample.employee 中读取记录，按 college 字段分组，使用 $max 返回每个分组中 score 字段值的最大值，输出字段名为“Max_score”，并输出第一条记录的 name 字段值和 college 字段值，输出字段名为“Name”和“College”

```lang-javascript
> db.sample.employee.aggregate( { $group: { _id: "$college", max_score: { $max: "$score" }, College: { $first: "$college" }, Name: { $first: "$name" } } } )
```      

输出结果如下：

```lang-json
{
   "Max_score": 80,
   "College": "College of Computer Science",
   "Name": "Jack"
}
{
   "Max_score": 95,
   "College": "College of Economics Finance",
   "Name": "Tom"
}
```

##$min##

记录分组后，取分组中指定字段的最小值返回，对嵌套对象使用点操作符（.）引用字段名。

**示例**

从集合 sample.employee 中读取记录，按 college 字段分组，使用 $min 返回每个分组中 score 字段值的最小值，输出字段名为“Min_score”，并输出第一条记录的 name 字段值和 college 字段值，输出字段名为“Name”和“College”

```lang-javascript
> db.sample.employee.aggregate( { $group: { _id: "$college", Min_score: { $min: "$score" }, College: { $first: "$college" }, Name: { $last: "$name" } } } )
```      

输出结果如下：

```lang-json
{
   "Min_score": 70,
   "College": "College of Computer Science",
   "Name": "Smith"
}
{
   "Min_score": 85,
   "College": "College of Economics Finance",
   "Name": "Ben"
}
```

##$last##

记录分组后，取分组中最后一条记录指定的字段值，对嵌套对象使用点操作符（.）引用字段名。

**示例**

从集合 sample.employee 中读取记录，对记录按 college 字段分组，使用 $last 取每个分组中最后一条记录的 name 字段值，输出字段名为“Name”，使用 $addtoset 把 major 字段值放入数组中返回，数组名为“Major”

```lang-javascript
> db.sample.employee.aggregate( { $group: { _id: "$college", Major: { $addtoset: "$major" }, Name: { $last: "$name" } } } )
```      

输出结果如下：

```lang-javascript
{
   "Major": [
     "Software Engineering",
     "Network Engineering",
     "Information Engineering"
   ],
   "Name": "Smith"
}
{
   "Major": [
     "Electronic Commerce",
     "Accounting Profession",
     "Financial Management"
   ],
   "Name": "Ben"
}
```

##$push##

记录分组后，使用 $push 将指定字段值添加到数组中，即使数组中已经存在相同的值，也可以继续添加。对嵌套对象使用点操作符（.）引用字段名。

**示例**

从集合 sample.employee 中读取记录，对记录按 college 字段值进行分组，每个分组 score 字段值使用 $push 放入数组中返回，数组名为“push_score”

```lang-javascript
> db.sample.employee.aggregate( { $group: { _id: "$college", College: { $first: "$college" }, push_score: { $push: "$score" } } } )
```      

输出结果如下：

```lang-json
{
   "College": "College of Computer Science",
   "push_score": [
     80,
     75,
     70
   ]
}
{
   "College": "College of Economics Finance",
   "push_score": [
     95,
     90,
     85
   ]
}
```

##$sum##

记录分组后，返回每个分组中指定字段值的总和，对嵌套对象使用点操作符（.）引用字段名。

**示例**

从集合 sample.employee 中读取记录，对记录按 college 字段分组，使用 $sum 返回每个分组中 score 字段值的总和，输出字段名为“sum_score”；使用 $first 取每个分组中第一条记录的 college 字段值，输出字段名为“College”

```lang-javascript
> db.sample.employee.aggregate( { $group: { _id: "$college", sum_score: { $sum: "$score" }, College: { $first: "$college" } } } )
```      

输出结果如下：

```lang-json
{
   "sum_score": 225,
   "College": "College of Computer Science"
}
{
   "sum_score": 270,
   "College": "College of Economics Finance"
}
```

