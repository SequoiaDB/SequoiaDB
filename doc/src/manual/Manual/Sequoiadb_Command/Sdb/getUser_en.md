##NAME##

getUser - Get User Information

##SYNOPSIS##

**db.getUser(<username>, [options])**

## CATEGORY ##

Sdb

## DESCRIPTION ##

This function is used to retrieve information about a specified user.

## PARAMETERS ##

| Parameter | Type      | Required | Description                             |
|-----------|-----------|----------|-----------------------------------------|
| username  | _string_  | Yes      | Specifies the user by their name.       |
| options   | _object_  | No       | Additional parameters.                  |
|           |           |          |   ShowPrivileges (_boolean_) - Show the user's privileges. Default value is `false`.|

## RETURN VALUE ##

Upon successful execution, this function will return a BSONObj through which the user information can be obtained.

Upon failure, it throws an exception and outputs an error message.

## ERRORS ##

Common exceptions are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -300 | SDB_AUTH_USER_NOT_EXIST | The specified user does not exist | |

When an exception is thrown, you can retrieve the error message using [getLastErrMsg()][getLastErrMsg] or the [error code][error_code] using [getLastError()][getLastError]. For more error handling, refer to the [Common Error Handling Guide][faq].

## VERSION ##

v5.8 and above

## EXAMPLES ##

- Get information about the user named `myuser` in the cluster, without enabling the `ShowPrivileges` option.

    ```lang-javascript
    > db.getUser("myuser")
    {
      "User": "myuser",
      "Roles": [
          "_foo.read"
      ],
      "Options": {}
    }
    ```

- Get information about the user named `myuser` in the cluster, with the `ShowPrivileges` option enabled.

    ```lang-javascript
    > db.getUser("myuser",{ShowPrivileges:true})
      {
          "User": "myuser",
          "Roles": [
              "_foo.read"
          ],
          "Options": {},
          "InheritedRoles": [
              "_foo.read"
          ],
          "InheritedPrivileges": [
              {
              "Resource": {
                  "cs": "foo",
                  "cl": ""
              },
              "Actions": [
                  "find",
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