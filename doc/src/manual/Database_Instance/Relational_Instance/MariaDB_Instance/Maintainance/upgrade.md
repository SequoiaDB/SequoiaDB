[^_^]:
    MariaDB 实例-升级

升级操作用于将 MariaDB 实例组件从低版本升级至较高版本，升级后不会改动任何配置和数据，同版本间也可进行升级。

>**Note:**
> 
> 升级操作不支持将 MariaDB 实例组件从高版本降级为低版本。

用户可使用 installmode 参数指定 upgrade 升级模式进行自动升级。以 `sequoiasql-mariadb-3.4-linux_x86_64-enterprise-installer.run` 为例对 MariaDB 实例组件进行升级，升级步骤如下：

1. 使用文本模式指定升级参数进行升级

    ```lang-bash
    # ./sequoiasql-mariadb-3.4-linux_x86_64-installer.run --mode text --installmode upgrade
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
    
    欢迎来到 SequoiaSQL MariaDB Server 安装程序
    
    ----------------------------------------------------------------------------
    ```

4. 显示可升级的选项，输入1，选择升级 `/opt/sequoiasql/mariadb` 目录下的安装；输入2则表示选择自定义路径，若指定的路径下存在安装则升级，没有则安装 

    ```lang-text
    ----------------------------------------------------------------------------
    请指定 SequoiaSQL MariaDB Server 将会被安装到的目录
    
       版本信息  安装目录
    
    [1] 3.4    /opt/sequoiasql/mariadb
    [2] other option
    请选择一个选项 [1] : 
    ```

5. 输入回车，确认继续

    ```lang-text
    ----------------------------------------------------------------------------
    设定现在已经准备将 SequoiaSQL MariaDB Server 安装到您的电脑.
    
    您确定要继续? [Y/n]: 
    ```

6. 升级成功

    ```lang-text
    ----------------------------------------------------------------------------
    正在安装 SequoiaSQL MariaDB Server 于您的电脑中，请稍候.
    
     安装中
     0% ______________ 50% ______________ 100%
     开始升级
    **************************  检查列表 *************************************
    检查：在/etc/default/sequoiasql-mariadb中获取用户名 ...... ok
    检查：安装目录/opt/sequoiasql/mariadb不为空 ...... ok
    检查：旧版本 3.4 与新版本 3.4 兼容 ...... ok
    检查：磁盘空间足够 ...... ok
    检查：umask配置 ...... ok
    检查：用户sdbadmin存在，并获取用户组 ...... ok
    检查：相关进程已停止 ...... ok
    #########################################
    
    ----------------------------------------------------------------------------
    安装程序已经完成安装 SequoiaSQL MariaDB Server 于你的电脑中.
    ```

7. 查看当前版本

    ```lang-bash
    # /opt/sequoiasql/mariadb/bin/sdb_maria_ctl --version
    ```

