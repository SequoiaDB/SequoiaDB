[^_^]:
    linux推荐配置

本文档将介绍在安装 SequoiaDB 巨杉数据库产品前，用户应如何调整 Linux 系统的环境配置，以保障系统稳定且高效地运行。

Linux 系统配置包括：

* 调整 ulimit
* 调整内核参数
* 调整文件系统挂载参数
* 关闭 transparent_hugepage
* 关闭 NUMA

> **Note:**
>
> 集群中包含的每台服务器都需要进行配置。

##调整 ulimit##
    
SequoiaDB 相关进程以数据库管理用户（安装 SequoiaDB 时指定，默认为 sdbadmin ）权限运行。因此，建议调整数据库管理用户的 ulimit 配置。  

1. 修改配置文件 `/etc/security/limits.conf`，配置如下内容：

    ```lang-text
    #<domain>      <type>    <item>        <value>
    sdbadmin       soft      core          0
    sdbadmin       soft      data          unlimited
    sdbadmin       soft      fsize         unlimited
    sdbadmin       soft      rss           unlimited
    sdbadmin       soft      as            unlimited
    sdbadmin       soft      nofile        65535
    sdbadmin       hard      nofile        65535
    ```
   
    - core：数据库出现故障时，是否产生 core 文件用于故障诊断，生产系统建议关闭；
    - data：数据库进程所允许分配的数据内存大小；
    - fsize：数据库进程所允许寻址的文件大小；
    - rss：数据库进程所允许的最大 resident set 大小；
    - as：数据库进程所允许的最大虚拟内存寻址空间限制；
    - nofile：数据库进程所允许打开的最大文件数。

2. 修改配置文件 `/etc/security/limits.d/90-nproc.conf`，配置如下内容：

    ```lang-text
    #<domain>      <type>    <item>     <value>
    sdbadmin       soft      nproc      unlimited
    sdbadmin       hard      nproc      unlimited
    ```
   
    nproc：数据库所允许的最大线程数限制。

    > **Note:**   
    >
    >  对于 Red Hat 7/CentOS 7 系统，需要在 `/etc/security/limits.d/20-nproc.conf` 文件中配置 nproc，而不在 `/etc/security/limits.d/90-nproc.conf` 文件中配置。

3. 重新登陆服务器使配置生效

##调整内核参数##

1. 备份系统原始的 vm 参数

    ```lang-bash
    # bak_time=`date +%Y%m%d%H%M%S`
    # cat /proc/sys/vm/swappiness             > swappiness_bak_$bak_time
    # cat /proc/sys/vm/dirty_ratio            > dirty_ratio_bak_$bak_time
    # cat /proc/sys/vm/dirty_background_ratio > dirty_background_ratio_bak_$bak_time
    # cat /proc/sys/vm/dirty_expire_centisecs > dirty_expire_centisecs_bak_$bak_time
    # cat /proc/sys/vm/vfs_cache_pressure     > vfs_cache_pressure_bak_$bak_time
    # cat /proc/sys/vm/min_free_kbytes        > min_free_kbytes_bak_$bak_time
    # cat /proc/sys/vm/overcommit_memory      > overcommit_memory_bak_$bak_time
    # cat /proc/sys/vm/overcommit_ratio       > overcommit_ratio_bak_$bak_time
    # cat /proc/sys/vm/max_map_count          > max_map_count_bak_$bak_time
    ```

2. 修改文件 `/etc/sysctl.conf`，添加如下内容：

    ```lang-ini
    vm.swappiness = 0
    vm.dirty_ratio = 100
    vm.dirty_background_ratio = 40
    vm.dirty_expire_centisecs = 3000
    vm.vfs_cache_pressure = 200
    vm.min_free_kbytes = <物理内存大小的 8%，单位 KB (kbytes)，最大不超过 1GB (即 1048576KB)>
    vm.overcommit_memory = 2
    vm.overcommit_ratio = 85
    vm.max_map_count = 262144
    ```
    
    > **Note:**
    > 
    > * 当数据库可用物理内存不足 8GB 时，不需要设置 vm.swappiness = 0。
    > * 上述 dirty 类参数（控制系统的 flush 进程只采用脏页超时机制刷新脏页，而不采用脏页比例超支刷新脏页）只是建议值，具体参数值可根据实际情况进行设置。 
    > * 如果用户采用 SSD 盘，建议设置 vm.dirty_expire_centisecs = 1000。

3. 执行如下命令使配置生效：

    ```lang-bash
    # /sbin/sysctl -p  
    ```

##调整文件系统挂载参数##

SequoiaDB 推荐使用 ext4 格式的文件系统。同时，建议在 `/etc/fstab` 文件中添加 noatime 挂载参数，以提升文件系统性能。

下述以块设备 `/dev/sdb`（假设其 UUID 为 993c5bba-494f-44ae-b543-a109f3598777，挂载目录为 `/data/disk_ssd1`）为例，介绍具体操作步骤。如果数据盘需要使用多个块设备，需对所有块设备进行设置。

###检查文件系统状态###

检查目标磁盘的文件系统状态

```lang-bash
# mount -t ext4
```

如果输出结果中显示文件系统为 ext4，并且挂载参数中包含 noatime，则表示已完成设置；如果输出为其他参数，则需要通过后续步骤进行配置

```lang-text
/dev/sdb1 on /data/disk_ssd1 type ext4 (rw,noatime)
```

###对已挂载的块设备进行设置###

如果块设备已创建分区且文件系统为 ext4，但其挂载参数不包含 noatime，可通过如下操作步骤进行设置：

1. 卸载已挂载的数据盘

    ```lang-bash
    # cd /
    # umount /dev/sdb1
    ```

2. 编辑 `/etc/fstab` 文件

    ```lang-bash
    # vi /etc/fstab
    ```
    
    将块设备 `/dev/sdb` 挂载参数调整为如下内容：
 
    ```lang-ini
    UUID=993c5bba-494f-44ae-b543-a109f3598777 /data/disk_ssd1 ext4 defaults,noatime 0 2
    ```

3. 挂载块设备

    ```lang-bash
    # mount -a
    ```
    
4. 检查挂载参数是否生效

    ```lang-bash
    # mount -t ext4
    ```

    
###对新块设备进行设置###

对于新增的块设备，可以进行如下设置：

1. 查看块设备情况

    ```lang-bash
    # fdisk -l /dev/sdb
    ```

2. 创建分区

    ```lang-bash
    # parted -s /dev/sdb mklabel gpt mkpart primary ext4 0% 100%
    ```

3. 将文件系统格式化为 ext4

    ```lang-bash
    # mkfs.ext4 /dev/sdb1
    ```

4. 查看数据盘分区的 UUID 

    ```lang-bash
    # lsblk -f /dev/sdb
    ```

    输出结果如下：

    ```lang-text
    NAME           FSTYPE   LABEL    UUID                                   MOUNTPOINT
    sdb                 
      └─sdb1       ext4              993c5bba-494f-44ae-b543-a109f3598777   
    ```

5. 创建需要挂载的数据目录

    ```lang-bash
    # mkdir /data/disk_ssd1
    ```

6. 编辑 `/etc/fstab` 文件

    ```lang-bash
    # vi /etc/fstab
    ```
    
    添加如下内容：
 
    ```lang-ini
    UUID=993c5bba-494f-44ae-b543-a109f3598777 /data/disk_ssd1 ext4 defaults,noatime 0 2
    ```

7. 挂载数据盘

    ```lang-bash
    # mount -a
    ```

8. 检查挂载参数是否生效


    ```lang-bash
    # mount -t ext4
    ```

##禁用 transparent_hugepage##

transparent_hugepage 在运行期间动态分配内存。该分配方式会导致内存分配延迟，程序性能下降。因此，建议用户使用 SequoiaDB 时禁用 transparent_hugepage。

###检查 transparent_hugepage 状态###

1. 执行如下命令

    ```lang-bash
    # cat /sys/kernel/mm/transparent_hugepage/enabled
    # cat /sys/kernel/mm/transparent_hugepage/defrag
    ```

2. 若返回结果为 [never]，则表示 transparent_hugepage 已被禁用
    
    ```lang-bash
    always madvise [never]
    ````

###关闭 transparent_hugepage###

1. 修改文件 `/etc/rc.local`

    ```lang-bash
    # vi /etc/rc.local
    ```

    >**Note:**
    >
    > 不同操作系统需要修改的文件存在差异，用户需根据实际情况获取确切的文件。

2. 在末尾添加如下内容：
    
    ```lang-bash
    echo never > /sys/kernel/mm/transparent_hugepage/enabled
    echo never > /sys/kernel/mm/transparent_hugepage/defrag
    ```

3. 执行命令使配置生效：
    
    ```lang-bash
    # source /etc/rc.local
    ```

##禁用 NUMA##

Linux 系统默认开启 NUMA。NUMA 默认的内存分配策略是优先从进程所在 CPU 节点的本地内存中分配，该分配策略大大降低了内存利用率。因此，NUMA 不适合数据库这种大规模内存使用的应用场景，建议用户在使用 SequoiaDB 时禁用 NUMA。

用户可通过以下两种方式禁用 NUMA：

* 通过修改 grub 的配置文件禁用 NUMA
* 通过 BIOS 设置禁用 NUMA


###检查 NUMA 状态###
	
1. 安装 numactl 工具

    ```lang-bash
    # yum install -y numactl
    ```
    
    或者
    
    ```lang-bash
    # apt-get install -y numactl
    ```

2. 检查 NUMA 是否已经禁用

    ```lang-bash
    # numastat
    ```

    如果输出结果中只有 node0 ，则表示已禁用 NUMA；如果有 node1 等其他 node 出现，则表示未禁用 NUMA
    
    ```lang-text
                               node0
    numa_hit             15324652334
    numa_miss                      0
    numa_foreign                   0
    interleave_hit             40411
    local_node           15324652334
    other_node                     0
    ```

###通过修改 grub 的配置文件禁用 NUMA###

- **对于 CentOS 6/Red Hat 6**

    1. 修改文件 `/boot/grub/grub.conf` 

        ```lang-bash
        # vi /boot/grub/grub.conf
        ```

    2. 找到“kernel”引导行（不同的版本内容略有差异，但开头有“kernel /vmlinuz-”）

        ```lang-text
        kernel /vmlinuz-2.6.32-358.el6.x86_64 ro root=/dev/mapper/vg_centos64001-lv_root quiet
        ```

    3. 在“kernel”引导行的末尾加上空格和“numa=off”

        ```lang-text
        kernel /vmlinuz-2.6.32-358.el6.x86_64 ro root=/dev/mapper/vg_centos64001-lv_root quiet numa=off
        ```

        >**Note:**
        >
        > 如果该配置文件中存在多个“kernel”引导行，则每个引导行都需要进行修改。

    4. 重启操作系统

        ```lang-bash
        # reboot
        ```

- **对于 CentOS 7/Red Hat 7/CentOS 8/Red Hat 8/Suse 12/openEuler/中标麒麟/Ubuntu 16/统信 UOS**

    1. 修改文件 `/etc/default/grub` 

        ```lang-bash
        # vi /etc/default/grub
        ```

    2. 找到配置项“GRUB_CMDLINE_LINUX”

        ```lang-text
        GRUB_CMDLINE_LINUX="crashkernel=auto rhgb quiet"
        ```

    3. 在配置项“GRUB_CMDLINE_LINUX”最后加上空格和“numa=off”

        ```lang-text
        GRUB_CMDLINE_LINUX="crashkernel=auto rhgb quiet numa=off"
        ```

    4. 重新生成 grub 引导文件

        对于 CentOS 7/Red Hat 7/CentOS 8/Red Hat 8，执行如下命令：
        
        ```lang-bash
        # grub2-mkconfig -o /etc/grub2.cfg
        ```
        对于 Suse12，执行如下命令：
        
        ```lang-bash
        # grub2-mkconfig -o /boot/grub2/grub.cfg
        ```
        
        对于 openEuler/中标麒麟，执行如下命令：
        
        ```lang-bash
        # grub2-mkconfig -o /etc/grub2-efi.cfg
        ```
  
        对于 Ubuntu 16/统信 UOS，执行如下命令：
        
        ```lang-bash
        # update-grub
        ```

    5. 重启操作系统

        ```lang-bash
        # reboot
        ```

###通过 BIOS 设置禁用 NUMA###

如果用户通过修改 grub 配置文件无法禁用 NUMA，可以在开机时根据启动页面介绍进入 BIOS 设置界面关闭 NUMA，保存设置并重启服务器。由于不同品牌的主板或服务器通过 BIOS 禁用 NUMA 的操作方式有所差异，此处不详细介绍操作步骤。


