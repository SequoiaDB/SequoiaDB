[^_^]:
    MySQL 实例-升级

升级操作用于将 MySQL 实例组件从低版本升级至较高版本。升级分为自动升级和手动升级，升级后不会改动任何配置和数据，同版本间也可进行升级。

>**Note:**
>
> 升级操作不支持将 MySQL 实例组件从高版本降级为低版本。

##自动升级##

自动升级适用于将 MySQL 实例组件从 3.2 及以上版本升级到更高版本，用户可使用 installmode 参数指定 upgrade 升级模式进行自动升级。以 `sequoiasql-mysql-3.2.4-linux_x86_64-enterprise-installer.run` 为例对 MySQL 实例组件进行升级，升级步骤如下：

1. 使用文本模式指定升级参数进行升级

    ```lang-bash
    # ./sequoiasql-mysql-3.2.4-linux_x86_64-installer.run --mode text --installmode upgrade
    ```

2. 程序提示选择向导语言，输入2，选择中文

    ```lang-text
    Language Selection
    
    Please select the installation language
    [1] English - English
    [2] Simplified Chinese - 简体中文
    Please choose an option [1] : 2
    ```

3. 显示安装协议，输入1，选择忽略阅读并同意协议；输入2则表示阅读详细的协议内容

    ```lang-text
    ----------------------------------------------------------------------------
    由BitRock InstallBuilder评估本所建立
    
    欢迎来到 SequoiaSQL MySQL Server 安装程序
    
    ----------------------------------------------------------------------------
    GNU 通用公共授权
    第二版, 1991年6月
    著作权所有 (C) 1989，1991 Free Software Foundation, Inc. 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
    允许每个人复制和发布本授权文件的完整副本，但不允许对它进行任何修改。
    
    [1] 同意以上协议: 了解更多的协议内容，可以在安装后查看协议文件
    [2] 查看详细的协议内容
    请选择一个选项 [1] : 
    ```

4. 显示可升级的选项，输入1，选择升级 `/opt/sequoiasql/mysql` 目录下的安装 ；输入2则表示选择自定义路径，若指定的路径下存在安装则升级，没有则安装 

    ```lang-text
    ----------------------------------------------------------------------------
    请指定 SequoiaSQL MySQL Server 将会被安装到的目录
    
       版本信息  安装目录
    
    [1] 3.2    /opt/sequoiasql/mysql
    [2] other option
    请选择一个选项 [1] : 
    ```

5. 输入回车，确认继续

    ```lang-text
    ----------------------------------------------------------------------------
    设定现在已经准备将 SequoiaSQL MySQL Server 安装到您的电脑.
    
    您确定要继续? [Y/n]: 
    ```

6. 升级成功

    ```lang-text
    ----------------------------------------------------------------------------
    正在安装 SequoiaSQL MySQL Server 于您的电脑中，请稍候.
    
     安装中
     0% ______________ 50% ______________ 100%
     开始升级
    **************************  检查列表 *************************************
    检查：在/etc/default/sequoiasql-mysql中获取用户名 ...... ok
    检查：安装目录/opt/sequoiasql/mysql不为空 ...... ok
    检查：旧版本 3.2 与新版本 3.2.4 兼容 ...... ok
    检查：磁盘空间足够 ...... ok
    检查：umask配置 ...... ok
    检查：用户sdbadmin存在，并获取用户组 ...... ok
    检查：相关进程已停止 ...... ok
    #########################################
    
    ----------------------------------------------------------------------------
    安装程序已经完成安装 SequoiaSQL MySQL Server 于你的电脑中.
    ```

7. 查看当前版本

    ```lang-bash
    # /opt/sequoiasql/mysql/bin/sdb_mysql_ctl --version
    ```

##手工升级##

手工升级适用于将 MySQL 实例组件从 3.2 以下的版本升级到 3.2 或以上版本。例如将已存在端口号为 3306 且路径为 `/opt/sequoiasql/mysql/database/3306` 的数据库实例从 3.0.2 升级到 3.2 版本，升级步骤如下：

1. 以 root 用户登陆目标主机，进入安装路径，卸载旧版本的 MySQL 实例组件

    ```lang-bash
    # cd /opt/sequoiasql/mysql
    # ./uninstall --mode unattended
    ```
    
    >**Note:**
    >  
    > 卸载不会清除数据目录以及安装路径下的配置文件 `auto.cnf`。

2. 赋予安装包可执行权限，指定静默模式安装

    ```lang-bash
    # chmod u+x sequoiadb-3.2-linux_x86_64-installer.run
    # ./sequoiadb-3.2-linux_x86_64-installer.run --mode unattended
    ```

3. 切换目录和用户

    ```lang-bash
    # cd /opt/sequoiasql/mysql
    # su sdbadmin
    ```

4. 将旧的数据目录 `database/3306` 进行备份

    ```lang-bash
    # mv database/3306 database/3306_bk
    ```

5. 添加实例名为 mysqld3306 的数据库实例

    ```lang-bash
    # bin/sdb_mysql_ctl addinst mysqld3306 -P 3306 -D database/3306
    ```

6. 停止实例

    ```lang-bash
    # bin/sdb_mysql_ctl stop mysqld3306
    ```

7. 备份新的实例数据目录下的 `database/3306/auto.cnf`

    ```lang-bash
    # mv database/3306/auto.cnf database/3306/auto.cnf_bk
    ```

8. 将备份数据拷贝到新的实例目录下

    ```lang-bash
    # cp -r database/3306_bk/* database/3306
    ```

9. 将原有配置文件的配置项合入到新的配置文件中（手动转移[mysqld]中的内容），并将新的配置文件还原
 
    ```lang-bash
    # mv database/3306/auto.cnf_bk database/3306/auto.cnf
    ```

10. 查看当前版本

    ```lang-bash
    # bin/sdb_mysql_ctl --version
    ```

11. 启动实例

    ```lang-bash
    # bin/sdb_mysql_ctl start mysqld3306
    ```