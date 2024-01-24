
指定记录查询参数。

包括指定匹配条件、返回记录字段名、排序情况、索引情况以及对返回结果集的处理等参数。


##语法##

**SdbQueryOption[.cond(\<cond\>)]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;[.sel(\<sel\>)]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;[.sort(\<sort\>)]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;[.hint(\<hint\>)]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;[.skip(\<skipNum\>)]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;[.limit(\<retNum\>)]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;[.update(\<rule\>, [returnNew], [options])]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;[.remove()]**

**SdbQueryOption[.cond(<cond>)][.skip(<skipNum>)][.limit(<retNum>)]**

**SdbQueryOption[.cond(\<cond\>)][.update(\<rule\>, [returnNew], [options])]**

**SdbQueryOption[.cond(\<cond\>)][.remove()]**

##方法##

###cond(\<cond\>)###

设置查询记录时的匹配条件。

| 参数名 			| 参数类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
|cond |	Json 对象 | 为空时，查询所有记录；不为空时，查询符合条件记录。如：{"age":{"$gt":30}}。关于匹配条件的使用，可参考[匹配符](manual/Manual/Operator/Match_Operator/Readme.md)。 | 是 |

###sel(\<sel\>)###
设置需要返回的记录字段名。

| 参数名 			| 参数类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
|sel     |Json 对象 | 为空时，返回记录的所有字段；如果指定的字段名记录中不存在，则按用户设定的内容原样返回。如：{"name":"","age":"","addr":""}。字段值为空字符串即可，数据库只关心字段名。关于选择条件的使用，可参考[选择符](manual/Manual/Operator/Selector_Operator/Readme.md)。 | 是 |

> **Note：**

>* sel 参数是一个json结构，如：{字段名:字段值}，字段值一般指定为空串。sel中指定的字段名在记录中存在，设置字段值不生效；不存在则返回sel中指定的字段名和字段值。
>* 记录中字段值类型为数组的，我们可以在sel中指定该字段名，用"."操作符加上双引号("")来引用数组元素。

###sort(\<sort\>)###

设置结果集的排序规则。

| 参数名 			| 参数类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
|sort |	Json 对象 | 指定结果集按指定字段名排序的情况。字段名的值为1或者-1，如：{"name":1,"age":-1}。1代表升序；-1代表降序。 如果不设定 sort 则表示不对结果集做排序。 | 是 |

###hint(\<hint\>)###

指定索引，用于查询结果集。

| 参数名 			| 参数类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
| hint   |	Json 对象 | 指定查询使用索引的情况。字段名可以为任意不重复的字符串，数据库只关心字段值。  | 是 |

* 不指定`hint`：查询是否使用索引及使用哪个索引将由数据库决定；	
* `hint`为{"":null}：查询走表扫描；
* `hint`为单个索引：如：{"":"myIdx"}，表示查询将使用当前集合中名字为"myIdx"的索引进行；
* `hint`为多个索引：如：{"1":"idx1","2":"idx2","3":"idx3"}，
                        表示查询将使用上述三个索引之一进行。
                        具体使用哪一个，由数据库评估决定。

###skip(\<skip\>)###

设置结果集从哪条记录开始返回。

| 参数名 			| 参数类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
| skipNum | int | 自定义从结果集哪条记录开始返回。默认值为0，表示从第一条记录开始返回。 | 是 |

> **Note：**

>如果不设定 skipNum 的内容或者设定 skipNum 的值为0，相当于返回所有的结果集；如果想从结果集的第3条记录开始返回，可设置 skipNum 的值等于2。

###limit(\<retNum\>)###

设置返回的记录条数。

| 参数名 			| 参数类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
| retNum | int | 自定义返回结果集的记录条数。默认值为-1，表示返回从`skipNum`位置开始到结果集结束位置的所有记录。 | 是 |

> **Note：**

>如果不设定 retNum 的内容，相当于返回所有的结果集记录。如果想返回结果集的前5条记录，可设置 retNum 的值为5。

###remove()###

删除查询后的结果集。

###update(\<rule\>, [returnNew], [options])###

更新查询后的结果集。

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| rule    | Json 对象 | 更新规则，记录按指定规则更新。 | 是 |
| returnNew | bool      | 是否返回更新后的记录。         | 否 |
| options | Json 对象 | 可选项，详见 options 选项说明。| 否 |

####options 选项###

| 参数名          | 参数类型 | 描述                | 默认值 |
| --------------- | -------- | ------------------- | ------ |
| KeepShardingKey | bool     | 是否保留分区键字段。| false  |

> **Note：**

>其中 rule 参数与 [update()](manual/Manual/Sequoiadb_Command/SdbCollection/update.md)的 rule 参数相同，options 参数与 [update()](manual/Manual/Sequoiadb_Command/SdbCollection/update.md)的 options 参数相同。returnNew 参数默认为 false，当为 true 时，返回修改后的记录值。

##返回值##

返回自身，类型为 SdbQueryOption。

##错误##

[错误码](manual/Manual/Sequoiadb_error_code.md)

##示例##

###cond###

查询匹配条件的记录，即设置 cond 参数的内容。如下操作返回集合 employee 中符合
条件 age 字段值大于25且 name 字段值为"Tom"的记录。

```lang-javascript
> var query = new SdbQueryOption().cond( { age: { $gt: 25 }, name: "Tom" } )
> db.sample.employee.find( query )
```

###sel###

指定返回的字段名，即设置 sel 参数的内容。如有记录{ age: 25, type: "system" }
和{ age: 20, name: "Tom", type: "normal" }，如下操作返回记录的age字段和name字段。

```lang-javascript
> var query = new SdbQueryOption().sel( { age: "", name: "" } )
> db.sample.employee.find( query )
{
	"age": 25,
	"name": ""
}
{
	"age": 20,
	"name": "Tom"
}
```

###sort###

返回集合 employee 中 age 字段值大于20的记录（如使用 [$gt](manual/Manual/Operator/Match_Operator/gt.md) 查询），设置只返回记录的 name 和 age 字段，并按 age 字段值的升序排序。

```lang-javascript
> var query = new SdbQueryOption().cond( { age: { $gt: 20 } } ).sel( { age: "", name: "" } ).sort( { age: 1 } )
> db.sample.employee.find( query )
```
> **Note：**

> 通过 [find()](manual/Manual/Sequoiadb_Command/SdbCollection/find.md) 方法，我们能任意选择我们想要返回的字段名，在上例中我们选择了返回记录的 age 和 name 字段，此时用 sort() 方法时，只能对记录的 age 或 name 字段排序。而如果我们选择返回记录的所有字段，即不设置 find 方法的 sel 参数内容时，那么 sort() 能对任意字段排序。

指定一个无效的排序字段。

```lang-javascript
> var query = new SdbQueryOption().cond( { age: { $gt: 20 } } ).sel( { age: "", name: "" } ).sort( { "sex": 1 } )
> db.sample.employee.find( query )
```
> **Note：**

> 因为“sex”字段并不存在于 sel() 选项 {age:"",name:""} 中，所以 sort() 指定的排序字段 {"sex":1} 将被忽略。

###hint###

使用索引 ageIndex 遍历集合 employee 下存在 age 字段的记录，并返回。

```lang-javascript
> var query = new SdbQueryOption().cond( {age: {$exists:1} } ).hint( { "": "ageIndex" } )
> db.sample.test.find( query )
{
 	"_id": {
    	"$oid": "5812feb6c842af52b6000007"
  	},
  	"age": 10
}
{
  	"_id": {
    	"$oid": "5812feb6c842af52b6000008"
  	},
  	"age": 20
}
```

###skip###

选择集合 employee 下 age 字段值大于10的记录（如使用 [$gt](manual/Manual/Operator/Match_Operator/gt.md) 查询），从第5条记录开始返回，即跳过前面的四条记录

```lang-javascript
> var query = new SdbQueryOption().cond( { age: { $gt: 10 } } ).skip( 4 )
> db.sample.employee.find( query )
```
> **Note：**

> 如果结果集的记录数小于5，那么无记录返回；如果结果集的记录数大于5，则从第5条开始返回。

###limit###

选择集合 employee 下 age 字段值大于10的记录（如使用 [$gt](manual/Manual/Operator/Match_Operator/gt.md) 查询），并只返回前面2条记录。

```lang-javascript
> var query = new SdbQueryOption().cond( { age: { $gt: 10 } } ).limit( 2 )
> db.sample.employee.find( query )
{
"_id": {
	"$oid": "5813035cc842af52b6000009"
},
"name": "Tom",
"age": 11
}
{
"_id": {
  "$oid": "58130372c842af52b600000a"
},
"name": "Jack",
  "age": 12
}
```
> **Note：**

> 如果结果集的记录数小于2，按实际的记录数返回，如果结果集的记录数大于2，则只返回前2条记录。

###update###

查询集合 employee 下 age 字段值大于10的记录，并将符合条件的记录的 age 字段加1。

```lang-javascript
> var query = new SdbQueryOption().cond( { age: { $gt: 10 } } ).update( { $inc: { age: 1 } } )
> db.sample.employee.find( query )
```
> **Note：**

> 1. 不能与 remove()同时使用。  
> 2. 与 sort()同时使用时，在单个节点上排序必须使用索引。  
> 3. 在集群中与 limit()或 skip()同时使用时，要保证查询条件会在单个节点或单个子表上执行。

###remove###

查询集合 employee 下 age 字段值大于10的记录，并将符合条件的记录删除。

```lang-javascript
> var query = new SdbQueryOption().cond( { age: { $gt: 10 } } ).remove()
> db.sample.employee.find( query )
```
> **Note：**

> 1. 不能与 update() 同时使用。  
> 2. 与 sort() 同时使用时，在单个节点上排序必须使用索引。  
> 3. 在集群中与 limit() 或 skip() 同时使用时，要保证查询条件会在单个节点或单个子表上执行。