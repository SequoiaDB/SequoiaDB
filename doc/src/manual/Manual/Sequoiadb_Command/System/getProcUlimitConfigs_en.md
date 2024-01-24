
##NAME##

getProcUlimitConfigs - Acquire the values of the process resource limit

##SYNOPSIS##

***System.getProcUlimitConfigs()***

##CATEGORY##

System

##DESCRIPTION##

Acquire the values of the process resource limit

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return values of the process resource limit.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Acquire the values of the process resource limit

```lang-javascript
> System.getProcUlimitConfigs()
{
  "core_file_size": 0,
  "data_seg_size": -1,
  "scheduling_priority": 0,
  "file_size": -1,
  "pending_signals": 23711,
  "max_locked_memory": 65536,
  "max_memory_size": -1,
  "open_files": 1024,
  "POSIX_message_queues": 819200,
  "realtime_priority": 0,
  "stack_size": 8388608,
  "cpu_time": -1,
  "max_user_processes": 23711,
  "virtual_memory": -1,
  "file_locks": -1
}
```