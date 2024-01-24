本文档主要介绍如何获取驱动开发包和配置开发环境。

##获取驱动开发包##

用户可以从 [SequoiaDB 巨杉数据库官网](http://download.sequoiadb.com/cn/index-cat_id-2)下载对应操作系统版本的 SequoiaDB 驱动开发包。

##支持版本##

解压驱动开发包，选择 libsdbphp-x.x.x.so（x.x.x 为版本号，请根据 PHP 版本选择，前两位需相同版本，第三位版本要小于或等于 PHP   的版本）。

**x86 Linux 和 Power Linux 支持版本**

| PHP 版本 |
| ------- |
| 5.3.3、5.3.8、5.3.10、5.3.15 |
| 5.4.6 或更高 |
| 5.5.x   |
| 5.6.x   |
| 7.0.x   |
| 7.1.1   |

##配置开发环境##

用户在配置开发环境之前需要先确保已安装 Apache 和 PHP。

- **Linux**

   1. 打开 `/etc/php5/apache2/php.ini` 文件

   2. 在该文件的[PHP]配置段中新增如下行

     ```lang-ini
     extension=<PATH>/libsdbphp-x.x.x.so
     ```

     其中 PATH 为 libsdbphp-x.x.x.so 文件放置路径；x.x.x 为 PHP 版本

    3. 保存并关闭文件

    4. 重新启动 apache2 服务

     ```lang-bash
     $ service apache2 restart（SUSE/Redhat/Ubuntu） 或  service httpd restart（CentOS）
     ```

    5. 编写包含如下内容 PHP 测试脚本，保存为 `test.php` 文件，并放在 apache 的 Web 服务目录下

     ```lang-php
     <?php phpinfo(); ?>
     ```

    6. 通过浏览器打开如下网址

     ```lang-http
     http://<IP>/test.php
     ```

     \<IP\>为 apache 所在的主机 IP，在打开的页面中查看是否包含 SequoiaDB 模块。

- **Windows**

 暂未提供 Windows 驱动开发包
