
##环境准备##

用户自行下载需要的安装包，对应版本参考如下：

软件| 版本
---|---
[PostgreSQL][pgsql] | Postgresql9.3.4.tar.gz
[postgresql-JDBC][jdbc] | postgresql-42.0.0.jre7.jar
[jboss][jboss] | jboss-as-7.1.1.Final.zip
[JDK][jdk] | JDK1.7

	
 > **Note:**
 >
 > JBoss7 与 jdk1.8 不兼容，建议使用 jdk-1.7.x 的版本。postgresql-JDBC 对 JDK 的驱动依赖很强，不同的 JDK 版本对应的 postgresql-JDBC 版本不一样，用户需要参看下载官网上说明。


##安装配置##

- **安装并部署 PostgreSQL**

 用户需自行安装并[部署 PostgreSQL][install_deploy]。

- **配置 JDK 环境变量**

   1. 修改配置文件

     ```lang-bash
     $ vim /etc/profile
     ```

     增加如下配置项

     ```lang-ini
     export JAVA_HOME=/usr/java/jdk1.7.0_67
     export CLASSPATH=${JAVA_HOME}/lib:${JAVA_HOME}/jre/lib
     export PATH=${JAVA_HOME}/bin:${PATH}
     ```

   2. 使环境变量生效

     ```lang-bash
     $ source /etc/profile
     ```

- **安装 JBoss**

   1. 将安装包解压到 `/opt` 目录下，并修改名字为“jboss”

      ```lang-bash
      $ unzip jboss-as-7.1.1.Final.zip -d /opt
      $ mv JBoss-7.1.1.Final jboss
      ```

     > **Note:**
     >
     > JBoss 有“domain”和“standalone”两种运行模式，两种模式的安装和配置是相同的。由于“standalone”提供的功能相对比较多，比如应用热部署、丰富的 web 操作界面等，因此为了降低安装的复杂度，在本次安装中采用的运行模式是“standalone”。

   2. 修改 `standalone.conf` 配置文件

      ```lang-bash
      $ vim /opt/jboss/bin/standalone.conf
      ```

      设置 JAVA_HOME 的值（用户可根据实际情况配置）

      ```lang-ini
      JAVA_HOME=/usr/java/jdk1.7.0_67
      ```

   3. 修改 `standalone.xml` 配置文件

      ```lang-bash
      $ vim /opt/jboss/standalone/configuration/standalone.xml
      ```

      将所有的 127.0.0.1 修改成本机 IP，修改方式如下：

      ```lang-text
      :%s/127.0.0.1/192.168.31.8/g		
      ```

     > **Note:**
     >
     > JBoss 作为一个 Web 容器，不仅能实现一些 Java EE 的规范（JMX、JMS、JTA、EJB等），也能对外提供服务（ OSGI 功能等），所以建议将所有的地址配置在非 127 的网段里。

    4. 启动数据库

      ```lang-bash
      $ /opt/jboss/bin/standalone.sh
      ```

     >**Note:**
     >
     > 执行 `standalone.sh` 脚本启动 JBoss，所有的日志信息会输出到屏幕上；建议配置日志文件，根据日志的级别来选择日志是持久化还是重定向到 `/dev/null` 中。本次安装选择直接启动 `standalone.sh`，目的是安装过程中能够及时查看信息，环境安装成功后可以根据自己需求来处理日志。

- **添加JBoss后台访问用户**

   1. 进入 `bin` 目录执行 `add-user.sh`

     ```lang-bash
     $ cd /opt/jboss/bin
     $ ./add-user.sh
     ```
   2. 程序提示选择要创建的用户类型，如输入a

     ```lang-text
     What type of user do you wish to add? 
     a) Management User (mgmt-users.properties) 
     b) Application User (application-users.properties)
     (a): a    							
     ```
   3. 提示输入创建用户的角色、用户名和密码（默认创建 ManagementRealm 角色）

     ```lang-text
     Enter the details of the new user to add.
     Realm (ManagementRealm) : 
     Username (jboss) : jboss
     Password : 
     Re-enter Password : 
     About to add user 'jboss' for realm 'ManagementRealm'
     Is this correct yes/no? yes
     ```

   4. 提示创建成功

     ```lang-text
     Added user 'jboss' to file    '/opt/jboss/standalone/configuration/mgmt-users.properties'
     Added user 'jboss' to file '/opt/jboss/domain/configuration/mgmt-users.properties'
     ```


[^_^]:
     本文使用的所有引用及链接
[pgsql]:https://www.postgresql.org/download/
[jdbc]:https://jdbc.postgresql.org/
[jboss]:http://jbossas.jboss.org/downloads/
[jdk]:https://www.oracle.com/index.html/
[install_deploy]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/install_deploy.md
