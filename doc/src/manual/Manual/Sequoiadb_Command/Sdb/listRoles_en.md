##NAME##

listRoles - Get Information of All Roles

##SYNOPSIS##

**db.listRoles([options])**

## CATEGORY ##

Sdb

## DESCRIPTION ##

This function is used to get information about all [custom roles][user_defined_roles] and [built-in roles][builtin_roles].

## PARAMETERS ##

| Parameter        | Type       | Required | Description                                                            |
|------------------|------------|----------|------------------------------------------------------------------------|
| options          | _object_   | No       | Additional parameters.                                                 |
| ShowPrivileges   | _boolean_  | No       | Show the privileges of roles. Default value is `false`.               |
| ShowBuiltinRoles | _boolean_  | No       | Additional get built-in roles. Default value is `false`.              |

## RETURN VALUE ##

Upon successful execution, this function will return a SdbCursor object through which the roles information can be obtained.

Upon failure, it throws an exception and outputs an error message.

## ERRORS ##

When an exception is thrown, you can retrieve the error message using [getLastErrMsg()][getLastErrMsg] or the [error code][error_code] using [getLastError()][getLastError]. For more error handling, refer to the [Common Error Handling Guide][faq].

## VERSION ##

v5.8 and above

## EXAMPLES ##

- Print all custom roles in the cluster, without enabling the `ShowPrivileges` option.

    ```lang-javascript
    > db.listRoles()
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

- Print all custom roles in the cluster, enabling the `ShowPrivileges` option.

    ```lang-javascript
    > db.listRoles({ShowPrivileges:true})
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

- Print all custom roles and built-in roles in the cluster, enabling the `ShowBuiltinRoles` option.
    ```lang-javascript
    > db.listRoles({ShowBuiltinRoles:true})
    {
    "Role": "_root",
    "Roles": [],
    "InheritedRoles": []
    }
    {
    "Role": "_clusterAdmin",
    "Roles": [],
    "InheritedRoles": []
    }
    {
    "Role": "_clusterMonitor",
    "Roles": [],
    "InheritedRoles": []
    }
    {
    "Role": "_backup",
    "Roles": [],
    "InheritedRoles": []
    }
    {
    "Role": "_dbAdmin",
    "Roles": [],
    "InheritedRoles": []
    }
    {
    "Role": "_userAdmin",
    "Roles": [],
    "InheritedRoles": []
    }
    {
    "Role": "_exact.read",
    "Roles": [],
    "InheritedRoles": []
    }
    {
    "Role": "_exact.readWrite",
    "Roles": [],
    "InheritedRoles": []
    }
    {
    "Role": "_exact.admin",
    "Roles": [],
    "InheritedRoles": []
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