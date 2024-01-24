
##NAME##

setUserConfigs - set the configuration of operating system user

##SYNOPSIS##

**System.setUserConfigs(\<options\>)**

##CATEGORY##

System

##DESCRIPTION##

This function is used to modify the user group, additional group, user directory and other configurations of the operating system user.

##PARAMETERS##

options ( *object, required* )

The user's attributes can be modified through the options parameter:

- name ( *string* ): Specify the user who needs to be modified. This parameter is required.

    Format: `name: "username"`

- gid ( *string* ): Specify the name or ID of the user primary group.

    This parameter must be specified as an existing user group. If not specified, a user group with the same name as the parameter name will be created by default.

    Format: `gid: "groupName"` or `gid: "2003"`

- groups ( *string* ): Specify the name or ID list of the user supplementary groups.

    This parameter must be specified as an existing user group, each supplementary groups is separated by a comma.

    Format: `groups: "groupName1,groupName2,groupName3"` or `groups: "2004,2005,2006"`

- isAppend ( *boolean* ): Specify whether to append additional groups, the default is false.

    This parameter needs to be used with the groups parameter. When groups is specified and isAppend is set to true, the supplementary group of the user will be added. When groups is specified and isAppend is set to false, the original group will be replaced.

    Format: `isAppend: true`

- isMove ( *boolean* ): Whether to move the newly specified directory, the default is false.

    When this parameter is true, the value of parameter dir must be specified.

    Format: `isMove: true`

- dir ( *string* ): Specify a new user directory, which only takes effect when the parameter isMove is true.

    This parameter cannot specify an existing directory. The specified new directory will retain the data of the original user directory, and the original user directory will be deleted.

    Format: `dir: "userHomeDir"`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##EXAMPLES##

Modify the home directory of user `newUser` in the specified user group.

```lang-javascript
> System.setUserConfigs({name: "newUser", gid: "groupName", dir: "/home/userName", isMove: true})
```



[^_^]:
    links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md