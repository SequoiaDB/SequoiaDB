##名称##

getSystemConfigs - 获取系统配置信息

##语法##

**System.getSystemConfigs( \[types\] )**

##类别##

System

##描述##

获取系统配置信息

##参数##

| 参数名  | 参数类型 | 默认值       | 描述             | 是否必填 |
| ------- | -------- | ------------ | ---------------- | -------- |
| types     | string   | all          | 系统模块类别       | 否       |

types 参数可选值如下表：

| 可选值 | 描述            |
| ------ | ---------------- |
|   kernel | 内核模块信息 |
|   vm    | 虚拟内存信息 |
|   fs    | 文件系统信息 |
|   debug  | 调试信息 |
|   dev    | 设备信息 |
|   abi    | 应用程序二进制接口信息 |
|   net    | 网络信息 |
|   all    | 所有信息 |

##返回值##

返回系统配置信息

##错误##

如果出错则抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##版本##

v3.2 及以上版本

##示例##

获取系统虚拟内存信息

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