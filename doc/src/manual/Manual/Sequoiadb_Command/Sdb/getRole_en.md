##NAME##

getRole - Get role information

##SYNOPSIS##
**db.getRole(\<rolename\>, [options])**

## CATEGORY ##

Sdb

## DESCRIPTION ##

This function is used to retrieve information about a specified custom role [Custom Roles][user_defined_roles] and [Built-in Roles][builtin_roles].

## PARAMETERS ##

* rolename (*string, required*) - Specifies the role by its name.

* options (*object, optional*) - Additional parameters.
  * ShowPrivileges (*boolean*) - Show the role's privileges. Default value is `false`.

## RETURN VALUE ##

Upon successful execution, this function will return a BSONObj through which the role information can be obtained.

Upon failure, it throws an exception and outputs an error message.

## ERRORS ##

Common exceptions are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -409 | SDB_AUTH_ROLE_NOT_EXIST | The specified role does not exist | |

When an exception is thrown, you can retrieve the error message using [getLastErrMsg()][getLastErrMsg] or the [error code][error_code] using [getLastError()][getLastError]. For more error handling, refer to the [Common Error Handling Guide][faq].

## VERSION ##

v5.8 and above

## EXAMPLES ##

- Get information about the role named `foo_developer` in the cluster, without enabling the `ShowPrivileges` option.

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

- Get information about the role named `foo_developer` in the cluster, with the `ShowPrivileges` option enabled.

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

- Get information about the built-in role `_foo.readWrite` in the cluster, with the `ShowPrivileges` option enabled.

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
    All references and links used in this document
[getLastErrMsg]: manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]: manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]: manual/FAQ/faq_sdb.md
[error_code]: manual/Manual/Sequoiadb_error_code.md
[user_defined_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/user_defined_roles.md
[builtin_roles]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/builtin_roles.md