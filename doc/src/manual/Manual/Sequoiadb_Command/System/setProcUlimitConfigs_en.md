
##NAME##

setProcUlimitConfigs - Modify process resource limits

##SYNOPSIS##

***System.setProcUlimitConfigs( \<configs\> )***

##CATEGORY##

System

##DESCRIPTION##

Modify process resource limits

##PARAMETERS##

| Name      | Type     | Default | Description         | Required or not |
| ------- | -------- | ------------ | ---------------- | -------- |
| configs  | JSON   | ---  | new resource limits   | yes       |

Fields that can be modified in the configs parameter can be found in the example in getProcUlimitConfigs

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Modify the maximum memory size of the process

```lang-javascript
> System.setProcUlimitConfigs( { "max_memory_size": -1 } )
```