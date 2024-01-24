SequoiaDB 巨杉数据库是一款金融级分布式关系型数据库，可以轻松地部署和运行在主流框架的服务器及虚拟化环境。同时作为一款高性能分布式数据库，SequoiaDB 巨杉数据库支持绝大多数的主流硬件网络设备和主流的 Linux 操作系统环境。

##受支持的硬件平台##

| 硬件平台类型     | 硬件平台列表                                |
|------------------|---------------------------------------------|
| x86 架构     |- 通用 x86 硬件平台                          |
| ARM64 架构   |- 华为 TaiShan 服务器（鲲鹏 920 处理器）<br> - 长城擎天服务器（飞腾 2000 处理器）  |
| Power 架构   |- 浪潮(IBM) Open Power |

##受支持的操作系统##

| 硬件平台类型 | 系统类型 | 系统列表                                                   |
|--------------|----------|------------------------------------------------------------|
| x86 系统     | Linux    |- Red Hat Enterprise Linux (RHEL) 6<br> - Red Hat Enterprise Linux (RHEL) 7<br> - Red Hat Enterprise Linux (RHEL) 8<br> - SUSE Linux Enterprise Server (SLES) 11 Service Pack 1 <br>   - SUSE Linux Enterprise Server (SLES) 11 Service Pack 2 <br> 	- SUSE Linux Enterprise Server (SLES) 12 Service Pack 1 <br> 	- Ubuntu 12.x <br> - Ubuntu 14.x <br> - Ubuntu 16.x <br> - CentOS 6.x <br> - CentOS 7.x <br> - CentOS 8.x <br> - 国产统信 UOS <br> - 深度 Deepin <br> - 中标麒麟 <br> - 银河麒麟 <br> - 红旗 Linux         |
| ARM64 架构   | Linux    |- Red Hat Enterprise Linux (RHEL) 7<br> - Red Hat Enterprise Linux (RHEL) 8<br> - Ubuntu 16.x <br> - CentOS 7.x <br> - CentOS 8.x <br> - 国产统信 UOS <br> - 深度 Deepin <br> - 中标麒麟 <br> - 银河麒麟 <br> - 华为 EulerOS(openEuler) |
| Power 架构   | Linux    |- Red Hat Enterprise Linux Server release 7.5 |

> **Note:**
>
>* 操作系统需安装 glibc 2.15 和 libstdc++ 6.0.18，或安装其更高版本。
>* 如果用户需要将未在上述列表中列举的 Linux 操作系统应用于生产环境，建议联系 SequoiaDB 技术支持，以获得更详细的信息。

##服务器要求##

SequoiaDB 巨杉数据库对于开发、测试和生产环境的服务器硬件配置有以下要求和建议：

###最小配置要求###

| 需求项 | 要求                                                                  | 推荐配置                     |
|--------|-----------------------------------------------------------------------|------------------------------|
| CPU    | - x86（Intel Pentium、Intel Xeon 和 AMD）32位 Intel 和 AMD 处理器 <br> - x64（64 位 AMD64 和 Intel EM64T 处理器） <br> - ARM（64位处理器）<br> - PowerPC 7 或者 PowerPC 7+ 处理器         | - x64（64 位 AMD64 和 Intel EM64T 处理器）<br> - ARM（64 位处理器） <br> - PowerPC 7 或者 PowerPC 7+ 处理器        |
| 磁盘   | 10GB+                                                                 | 100GB+                       |
| 内存   | 1GB+                                                                  | 2GB+                         |
| 网卡   | 1 张+                                                                 | 百兆网卡                     |

> **Note:**
>
> * 验证测试环境中的 SequoiaDB 数据库可部署在同一个服务器上。
> * 如进行性能相关的测试，需采取高性能存储和网络硬件配置，防止影响测试结果。

###高性能配置要求###

| 需求项 | 要求                                                                  | 推荐配置                     |
|--------|-----------------------------------------------------------------------|------------------------------|
| CPU    | - x64（64 位 AMD64 和 Intel EM64T 处理器） <br> - ARM（64位处理器）<br> - PowerPC 7 或者 PowerPC 7+ 处理器       | - x64（64 位 AMD64 和 Intel EM64T 处理器）<br> - ARM（64 位处理器） <br> - PowerPC 7 或者 PowerPC 7+ 处理器       |
| 磁盘   | 512GB（至少一块）                                                     | 2TB（至少两块）              |
| 内存   | 32GB+                                                                 | 64GB+                        |
| 网卡   | 1 张+                                                                 | 千兆网卡                     |

> **Note:**
>
> * 高性能配置中的 SequoiaDB 数据库部署在物理机上，如对性能和可靠性有更高的要求，应尽可能避免部署在虚拟机上。
> * 生产环境强烈推荐使用较高性能配置。

###较高性能配置要求###

| 需求项 | 要求                                                                  | 推荐配置                     |
|--------|-----------------------------------------------------------------------|------------------------------|
| CPU    | - x64（64 位 AMD64 和 Intel EM64T 处理器） <br> - ARM（64位处理器）<br> - PowerPC 7 或者 PowerPC 7+ 处理器       | - x64（64 位 AMD64 和 Intel EM64T 处理器）<br> - ARM（64 位处理器） <br> - PowerPC 处理器            |
| 磁盘   | 2TB 或 4TB（至少十块）                                                | 4TB（至少十块）              |
| 内存   | 64GB+                                                                 | 64GB+                        |
| 网卡   | 1 张+                                                                 | 万兆网卡                     |

> **Note:**
>
> SequoiaDB 数据库磁盘大小配置建议普通物理磁盘不超过 4TB，单台服务器可配置部分物理SSD盘以提高性能。

##操作系统配置##

###Linux 系统要求###

在安装 SequoiaDB 之前，应该先对 Linux 系统相关的配置进行检查和设置。需要检查和设置的配置包括：

* 配置主机名 
* 配置主机名/IP地址映射
* 配置防火墙
* 配置 SELinux
* 配置时区

配置说明：

* 需要使用 root 用户权限进行配置，应确保 root 用户对相关命令或配置文件具有访问权限。
* 示例中“sdbserver1”为主机名称，用户可以根据需要修改该主机名。
* 主机名、主机名/IP地址映射、防火墙、SELinux 和时区需要在每台物理机器上进行配置。

###配置主机名###

**配置方法**

[^_^]:tab
- SUSE:

    设置主机名

    ```lang-bash
    # hostname sdbserver1
    ```

    将主机名持久化到配置文件

    ```lang-bash
    # echo "sdbserver1" > /etc/HOSTNAME
    ```

- Red Hat:

    - 对于 Red Hat 6/CentOS 6 及以下的系统，执行如下命令设置主机名： 

        ```lang-bash
        # hostname sdbserver1
        ```
        
        将主机名持久化到配置文件
        
        ```lang-bash
        # sed -i "s/HOSTNAME=.*/HOSTNAME=sdbserver1/g" /etc/sysconfig/network
        ```

    - 对于 Red Hat 7/Red Hat 8 和 CentOS 7/CentOS 8，执行如下命令设置主机名： 

        ```lang-bash
        # hostname sdbserver1
        ```
        
        将主机名持久化到配置文件
        
        ```lang-bash
        # echo "sdbserver1" > /etc/hostname
        ```

- Ubuntu:

    设置主机名 

     ```lang-bash
     # hostname sdbserver1
    ```
   
    将主机名持久化到配置文件

    ```lang-bash
    # echo "sdbserver1" > /etc/hostname
    ```

###配置主机名/IP地址映射###

**配置方法**

将服务器节点的主机名与IP映射关系配置到 `/etc/hosts` 文件中：

```lang-bash
# echo "192.168.20.200 sdbserver1" >> /etc/hosts
# echo "192.168.20.201 sdbserver2" >> /etc/hosts
```

**验证方法**

1. ping sdbserver1（本机主机名） 可以 ping 通：

    ```lang-bash
    # ping sdbserver1
    ```
2. ping sdbserver2（远端主机名） 可以 ping 通：

    ```lang-bash
    # ping sdbserver2
    ```

###关闭防火墙###

**配置方法**

[^_^]:tab
- SUSE:

    - 对于 SUSE 11，执行如下命令：

        ```lang-bash
        # SuSEfirewall2 stop    # 临时关闭防火墙
        # chkconfig SuSEfirewall2_init off    # 设置开机禁用防火墙
        # chkconfig SuSEfirewall2_setup off
        ```

    - 对于 SUSE 12，执行如下命令：

        ```lang-bash
        # systemctl stop SuSEfirewall2.service    # 临时关闭防火墙
        # systemctl disable SuSEfirewall2.service    # 设置开机禁用防火墙
        ```

- Red Hat:

    - 对于 Red Hat 6/CentOS 6 及以下系统，执行如下命令：

        ```lang-bash
        # service iptables stop    # 临时关闭防火墙
        # chkconfig iptables off    # 设置开机禁用防火墙
        ```

    - 对于 Red Hat 7/Red Hat 8 和 CentOS 7/CentOS 8，执行如下命令：

        ```lang-bash
        # systemctl stop firewalld.service    # 临时关闭防火墙
        # systemctl disable firewalld.service    # 设置开机禁用防火墙
        ```

- Ubuntu:

     执行如下命令：

     ```lang-bash
     # ufw disable
     ```

**验证方法**

[^_^]:tab
- SUSE:

    - 对于 SUSE 11，执行命令，若打印以下信息，说明关闭防火墙成功：

        ```lang-bash
        # chkconfig -list | grep fire
        SuSEfirewall2_init       	0:off	1:off	2:off	3:off	4:off	5:off	6:off
        SuSEfirewall2_setup      	0:off	1:off	2:off	3:off	4:off	5:off	6:off
        ```

    - 对于 SUSE 12，执行命令，若打印以下信息，说明关闭防火墙成功：

        ```lang-bash
        # systemctl status SuSEfirewall2.service
        ● SuSEfirewall2.service - SuSEfirewall2 phase 2
              Loaded: loaded (/usr/lib/systemd/system/SuSEfirewall2.service; disabled; vendor preset: disabled)
              Active: inactive (dead)
        ```

- Red Hat:

    - 对于 Red Hat 6/CentOS 6 及以下系统，执行命令，若打印以下信息，说明关闭防火墙成功：

        ```lang-bash
        # chkconfig --list iptables
        iptables       	0:off	1:off	2:off	3:off	4:off	5:off	6:off
        ```

    - 对于 Red Hat 7/Red Hat 8 和 CentOS 7/CentOS 8，执行命令，若打印以下信息，说明关闭防火墙成功：

        ```lang-bash
        # systemctl status firewalld.service
        ● firewalld.service - firewalld - dynamic firewall daemon
              Loaded: loaded (/usr/lib/systemd/system/firewalld.service; disabled; vendor preset: enabled)
              Active: inactive (dead)
                Docs: man:firewalld(1)
        ```

- Ubuntu:

     执行命令，若打印以下信息，说明关闭防火墙成功

     ```lang-bash
     # ufw status
     Status: inactive
     ```

###配置 SELinux###

针对 SELinux 可以配置为关闭或者将模式调整成 permissive，建议关闭 SELinux。

**关闭 SELinux**

- 配置方法

    1. 修改配置文件 `/etc/selinux/config`，将 SELINUX 配置为 disabled

        ```lang-bash
        # sed -i "s/SELINUX=.*/SELINUX=disabled/g" /etc/selinux/config
        ```

    2. 重启操作系统

        ```lang-bash
        # reboot # 需要重启系统
        ```

- 验证方法

    ```lang-bash
    # sestatus
    SELinux status:                 disabled
    ```

**模式设置成 permissive**

- 配置方法

    1. 关闭 SELinux 防火墙

        ```lang-bash
        # setenforce 0
        ```

    2. 修改配置文件 `/etc/selinux/config`，将 SELINUX 配置为 permissive

        ```lang-bash
        # sed -i "s/SELINUX=.*/SELINUX=permissive/g" /etc/selinux/config
        ```

- 验证方法

    ```lang-bash
    # sestatus
    SELinux status:                 enabled
    SELinuxfs mount:                /sys/fs/selinux
    SELinux root directory:         /etc/selinux
    Loaded policy name:             targeted
    Current mode:                   permissive
    Mode from config file:          permissive
    Policy MLS status:              enabled
    Policy deny_unknown status:     allowed
    Max kernel policy version:      28
    ```

###设置时区###

为保证时间数据正确，要求 SequoiaDB 集群中所有机器的时区保持一致。下述以中国上海时区为例，介绍具体步骤。

[^_^]:tab
- SUSE:

    - 设置为 Asia/Shanghai 时区

        ```lang-bash
        # sed -i "s/TIMEZONE=.*/TIMEZONE=Asia\/Shanghai/g" /etc/sysconfig/clock
        ```

    - 更新文件 `/etc/localtime`

        ```lang-bash
        # zic -l Asia/Shanghai
        ```

    - 查看时区，若显示 CST 则说明时区已设置为 Asia/Shanghai：

        ```lang-bash
        # date
          Sat Nov  5 17:57:06 CST 2022
        ```

- Red Hat:

    - 设置为 Asia/Shanghai 时区

        ```lang-bash
        # timedatectl set-timezone Asia/Shanghai
        ```

    - 查看时区

        ```lang-bash
        # timedatectl status
        ...
        Time zone: Asia/Shanghai (CST, +0800)
        ...
        ```

- Ubuntu:

    - 设置为 Asia/Shanghai 时区

        ```lang-bash
        # timedatectl set-timezone Asia/Shanghai
        ```

    - 查看时区

        ```lang-bash
        # timedatectl status
        ...
        Time zone: Asia/Shanghai (CST, +0800)
        ...
        ```

- UOS V20:

    - 设置为 Asia/Shanghai 时区

        ```lang-bash
        # timedatectl set-timezone Asia/Shanghai
        ```

    - 查看时区

        ```lang-bash
        # timedatectl status
        ...
        Time zone: Asia/Shanghai (CST, +0800)
        ...
        ```

- Kylin V10:

    - 设置为 Asia/Shanghai 时区

        ```lang-bash
        # timedatectl set-timezone Asia/Shanghai
        ```

    - 查看时区

        ```lang-bash
        # timedatectl status
        ...
        Time zone: Asia/Shanghai (CST, +0800)
        ...
        ```