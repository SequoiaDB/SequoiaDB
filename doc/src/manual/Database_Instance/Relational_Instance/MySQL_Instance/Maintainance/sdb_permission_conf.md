# SequoiaDB权限配置

在 SequoiaDB 中 Role-Based Access Control (RBAC) 是一种权限管理机制，基于角色的概念控制用户对数据库资源的访问权限。

本节介绍 MySQL 实例通过用户的方式连接 SequoiaDB 时，用户需要拥有的对 SequoiaDB 数据库资源的访问权限。

## SequoiaDB基于角色的访问控制
- [SequoiaDB基于角色的访问控制][SequoiaDB_RBAC]

## MySQL实例对接命令
此命令将会赋予 MySQL 实例连接 SequoiaDB 所需要的所有权限(包括实例组)

角色创建
```lang-javascript
> db.createRole({
	 Role: "sequoiasql_developer",
	 Privileges:[
		 {
			 Resource:{Cluster:true},
			 Actions:["trans","snapshot","createCS","dropCS"]
		 }
	 ],
	 Roles:[
		 "_dbAdmin"
	 ]
 })
```


##MySQL实例访问所需权限
###SELECT 所需权限
- trans on { Cluster : true }
- snapshot on { Cluster : true }
- find on { cs: "<cs name>", cl: "<cl name>" }
- testCS on { cs: "<cs name>", cl: "" }
- testCL on { cs: "<cs name>", cl: "<cl name>" }
- getDetail on { cs: "<cs name>", cl: "<cl name>" }

###INSERT 所需权限
- testCS on { cs: "<cs name>", cl: "" }
- testCL on { cs: "<cs name>", cl: "<cl name>" }
- trans on { Cluster : true }
- insert on { cs: "<cs name>", cl: "<cl name>" }
- find on { cs: "<cs name>", cl: "<cl name>" }
- getDetail on { cs: "<cs name>", cl: "<cl name>" }
- update on { cs: "<cs name>", cl: "<cl name>" }

###CREATE 所需权限
- createCS on { Cluster : true }
- createCL on { cs: "<cs name>", cl: "" }
- trans on { Cluster : true }
- find on { cs: "<cs name>", cl: "<cl name>" }
- getDetail on { cs: "<cs name>", cl: "<cl name>" }
- testCS on { cs: "<cs name>", cl: "" }
- testCL on { cs: "<cs name>", cl: "" }
- snapshot on { Cluster : true }
- createIndex on { cs: "<cs name>", cl: "<cl name>" }
- insert on { cs: "<cs name>", cl: "<cl name>" }

###UPDATE 所需权限
- testCS on { cs: "<cs name>", cl: "" }
- testCL on { cs: "<cs name>", cl: "<cl name>" }
- update on { cs: "<cs name>", cl: "<cl name>" }
- trans on { Cluster : true }
- find on { cs: "<cs name>", cl: "<cl name>" }
- getDetail on { cs: "<cs name>", cl: "<cl name>" }

###DELETE 所需权限
- testCS on { cs: "<cs name>", cl: "" }
- testCL on { cs: "<cs name>", cl: "<cl name>" }
- find on { cs: "<cs name>", cl: "<cl name>" }
- getDetail on { cs: "<cs name>", cl: "<cl name>" }
- trans on { Cluster : true }
- remove on { cs: "<cs name>", cl: "<cl name>" }

###ALTER 所需权限
- attachCL on { cs: "<cs name>", cl: "<cl name>" }
- detachCL on { cs: "<cs name>", cl: "<cl name>" }
- alterCL on { cs: "<cs name>", cl: "<cl name>" }
- createCL on { cs: "<cs name>", cl: "" }
- createIndex on { cs: "<cs name>", cl: "<cl name>" }
- remove on { cs: "<cs name>", cl: "<cl name>" }
- dropCL on { cs: "<cs name>", cl: "" } 
- dropIndex on { cs: "<cs name>", cl: "<cl name>" }
- testCS on { cs: "<cs name>", cl: "" }
- testCL on { cs: "<cs name>", cl: "<cl name>" }
- find on { cs: "<cs name>", cl: "<cl name>" }
- getDetail on { cs: "<cs name>", cl: "<cl name>" }
- snapshot on { Cluster : true }
- insert on { cs: "<cs name>", cl: "<cl name>" }
- find on { cs: "<cs name>", cl: "<cl name>" }
- renameCL on { cs: "<cs name>", cl: "" }
- trans on { Cluster : true }
- truncate on { cs: "<cs name>", cl: "<cl name>" }


###实例组相关
- find on { cs: "<cs name>", cl: "<cl name>" }
- getDetail on { cs: "<cs name>", cl: "<cl name>" }
- testCS on { cs: "<cs name>", cl: "" }
- testCL on { cs: "<cs name>", cl: "<cl name>" }
- snapshot on { Cluster : true }
- alterCL on { cs: "<cs name>", cl: "<cl name>" }
- createIndex on { cs: "<cs name>", cl: "<cl name>" }
- analyze on { cs: "<cs name>", cl: "<cl name>" }
- attachCL on { cs: "<cs name>", cl: "<cl name>" }
- createCS on { Cluster : true }
- createCL on { cs: "<cs name>", cl: "" }
- detachCL on main cl
- dropCS on on { Cluster : true }
- dropCL on { cs: "<cs name>", cl: "" }
- list on { Cluster : true }
- insert on { cs: "<cs name>", cl: "<cl name>" }
- find on { cs: "<cs name>", cl: "<cl name>" }
- remove on { cs: "<cs name>", cl: "<cl name>" }
- update on { cs: "<cs name>", cl: "<cl name>" }
- renameCL on { cs: "<cs name>", cl: "" }
- trans on { Cluster : true }
- truncate on { cs: "<cs name>", cl: "<cl name>" }

[^_^]:
    本文使用的所有引用和链接
[SequoiaDB_RBAC]:manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/Readme.md