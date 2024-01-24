
##NAME##

getSystemConfigs - Acquire the information of system configuration

##SYNOPSIS##

***System.getSystemConfigs( \[types\] )***

##CATEGORY##

System

##DESCRIPTION##

Acquire the information of system configuration

##PARAMETERS##

| Name      | Type     | Default | Description         | Required or not |
| ------- | -------- | ------------ | ---------------- | -------- |
| types     | string   | all     | system module type   | not       |

The detail description of 'types' parameter is as follow:

| Attributes |  Description  |
| ---------- | ------- |
|   kernel | kernel module |
|   vm    | virtual memory module |
|   fs    | file system module |
|   debug  | debug module |
|   dev    | device module |
|   abi    | application biary interface module |
|   net    | network module |
|   all    | all modules |

##RETURN VALUE##

On success, return the information of operating system configuration.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

Acquire the information of operating system configuration about virtual memory

```lang-javascript
> System.getSystemConfigs( "vm" )
{
  "vm.admin_reserve_kbytes": "8192",
  "vm.block_dump": "0",
  "vm.compact_unevictable_allowed": "1",
  "vm.dirty_background_bytes": "0",
  "vm.dirty_background_ratio": "10",
  "vm.dirty_bytes": "0",
  "vm.dirty_expire_centisecs": "3000",
  "vm.dirty_ratio": "20",
  "vm.dirty_writeback_centisecs": "500",
  "vm.dirtytime_expire_seconds": "43200",
  "vm.drop_caches": "0",
  "vm.extfrag_threshold": "500",
  "vm.hugepages_treat_as_movable": "0",
  "vm.hugetlb_shm_group": "0",
  "vm.laptop_mode": "0",
  "vm.legacy_va_layout": "0",
  "vm.lowmem_reserve_ratio": "256    256    32    1",
  "vm.max_map_count": "65530",
  "vm.memory_failure_early_kill": "0",
  "vm.memory_failure_recovery": "1",
  "vm.min_free_kbytes": "67584",
  "vm.min_slab_ratio": "5",
  "vm.min_unmapped_ratio": "1",
  "vm.mmap_min_addr": "65536",
  "vm.nr_hugepages": "0",
  "vm.nr_hugepages_mempolicy": "0",
  "vm.nr_overcommit_hugepages": "0",
  "vm.nr_pdflush_threads": "0",
  "vm.numa_zonelist_order": "default",
  "vm.oom_dump_tasks": "1",
  "vm.oom_kill_allocating_task": "0",
  "vm.overcommit_kbytes": "0",
  "vm.overcommit_memory": "0",
  "vm.overcommit_ratio": "50",
  "vm.page-cluster": "3",
  "vm.panic_on_oom": "0",
  "vm.percpu_pagelist_fraction": "0",
  "vm.stat_interval": "1",
  "vm.swappiness": "60",
  "vm.user_reserve_kbytes": "131072",
  "vm.vfs_cache_pressure": "100",
  "vm.zone_reclaim_mode": "0"
}
```