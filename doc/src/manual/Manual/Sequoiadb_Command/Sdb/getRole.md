##名称##

getRole - 获取角色的信息

##语法##

**db.getRole(\<rolename\>, [options] )**

##类别##

Sdb

##描述##

该函数用于获取指定自定义角色[自定义角色][user_defined_roles]和[内建角色][builtin_roles]的信息

##参数##

* rolename （ *string，必填* ） 通过 rolename 指定角色

* options （ *object，选填* ） 额外参数
  * ShowPrivileges （ *boolean* ） 展示角色的权限，默认值为 `false`

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取角色的详细信息。

函数执行失败时，将抛异常并输出错误信息。

##错误##

常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -409 | SDB_AUTH_ROLE_NOT_EXIST | 指定角色不存在 | |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.8 及以上版本

##示例##

- 在集群中获取名为 `foo_developer` 的角色，不开启 `ShowPrivileges` 选项

    ```lang-javascript
    > db.getRole("foo_developer")
    {
        "_id": {
            "$oid": "64c0eb5e8c7c328f60bc6d71"
        },
        "Role": "foo_developer",
        "Roles": [
            "_foo.readWrite"
        ],
        "InheritedRoles": [
            "_foo.readWrite"
        ]
    }
    ```

- 在集群中获取名为 `foo_developer` 的角色，开启 `ShowPrivileges` 选项

    ```lang-javascript
    > db.getRole("foo_developer", {ShowPrivileges:true})
    {
        "_id": {
            "$oid": "64c0eb5e8c7c328f60bc6d71"
        },
        "Role": "foo_developer",
        "Privileges": [
            {
            "Resource": {
                "Cluster": true
            },
            "Actions": [
                "snapshot"
            ]
            }
        ],
        "Roles": [
            "_foo.readWrite"
        ],
        "InheritedRoles": [
            "_foo.readWrite"
        ],
        "InheritedPrivileges": [
            {
            "Resource": {
                "Cluster": true
            },
            "Actions": [
                "snapshot"
            ]
            },
            {
            "Resource": {
                "cs": "foo",
                "cl": ""
            },
            "Actions": [
                "find",
                "insert",
                "update",
                "remove",
                "getDetail"
            ]
            }
        ]
    }
    ```

- 在集群中获取名为 `_foo.readWrite` 的内建角色，开启 `ShowPrivileges` 选项

    ```lang-javascript
    > db.getRole("_foo.readWrite", {ShowPrivileges:true})
    {
        "Role": "_foo.readWrite",
        "Roles": [],
        "InheritedRoles": [],
        "Privileges": [
            {
            "Resource": {
                "cs": "foo",
                "cl": ""
            },
            "Actions": [
                "find",
                "insert",
                "update",
                "remove",
                "getDetail"
            ]
            }
        ],
        "InheritedPrivileges": [
            {
            "Resource": {
                "cs": "foo",
                "cl": ""
            },
            "Actions": [
                "find",
                "insert",
                "update",
                "remove",
                "getDetail"
            ]
            }
        ]
    }
    ```

[^_^]: 
    本文使用的所有引用及链接
[getLastErrMsg]: manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]: manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]: manual/FAQ/faq_sdb.md
[error_code]: manual/Manual/Sequoiadb_error_code.md
[user_defined_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/user_defined_roles.md
[builtin_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/builtin_roles.md
