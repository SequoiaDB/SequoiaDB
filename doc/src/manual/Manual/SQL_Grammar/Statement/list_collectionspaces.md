
list collectionspaces 枚举数据库中所有的集合空间。

##语法##
***list collectionspaces***

##参数##
无

##返回值##
数据库中所有的集合空间。

##示例##

返回数据库所有的集合空间

```lang-javascript
> db.exec("list collectionspaces") 
{ "Name": "sample" }
Return 1 row(s).
```
