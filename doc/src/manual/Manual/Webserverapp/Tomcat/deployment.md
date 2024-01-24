
##环境准备##

用户需要自行安装 PostgreSQL，可参考 [PostgreSQL 部署][install_deploy]。

##安装配置##

  
1. 通过 [tomcat 官网][dowmload]下载对应版本的 tomcat 安装包

2. 使用 root 用户登录，将 tomcat 安装包放在 `/opt` 目录下，并解压安装包

   ```lang-bash
   # tar -zxvf apache-tomcat-7.0.68.tar.gz
   ```

3. 将解压后的文件拷贝到 `/usr/local` 下

   ```lang-bash
   # cp -R /opt/apache-tomcat-7.0.68 /usr/local/
   ```

4. 打开 `/usr/local/apache-tomcat-7.0.68/bin/catalina.sh` 文件

   ```lang-bash
   # vim /usr/local/apache-tomcat-7.0.68/bin/catalina.sh
   ```

   增加如下配置项，配置内存大小（用户可根据实际需求进行修改）： 
  
   ```lang-ini
   JAVA_OPTS="-server -Xms800m -Xmx800m -XX:PermSize=64M -XX:MaxNewSize=256m -XX:MaxPermSize=128m -Djava.awt.headless=true"
   ```

5. 查看 tomcat 端口是否被占用（默认是 8080）
 
   ```lang-bash
   # netstat -lnpt | grep 8080
   ```

   如果端口被占用，可通过 `/usr/local/apache-tomcat-7.0.68/conf/server.xml` 文件的 port 参数修改端口

   ```lang-xml
   <Connector port="8080" protocol="HTTP/1.1"
                  connectionTimeout="20000"
                  redirectPort="8443" />
   ```


6. 启动 tomcat 服务器

   ```lang-bash
   # /usr/local/apache-tomcat-7.0.68/bin/startup.sh
   ```

7. 访问 `http://ip:port` 验证 tmocat 服务是否启动成功，启动成功页面显示如下：


![部署][tomcathome]


[^_^]:
     本文使用的所有引用和链接
[install_deploy]:manual/Database_Instance/Relational_Instance/PostgreSQL_Instance/install_deploy.md
[dowmload]:http://tomcat.apache.org/download-70.cgi
[tomcathome]:images/Manual/Webserverapp/Tomcat/tomcathome.jpg