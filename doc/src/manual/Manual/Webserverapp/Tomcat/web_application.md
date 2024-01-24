
1. 将待发布的 web 应用 war 包放入 tomcat 服务器的 `/usr/local/apache-tomcat-7.0.68/webapps` 目录下，例如：将实现连接 PostgreSQL，并获取 PostgreSQL 版本的 test 应用打成 `test.war` 放入该目录下

2. 重启 tomcat 使 web 应用加载成功

   ```lang-bash
   # /usr/local/apache-tomcat-7.0.68/bin/shutdown.sh
   # /usr/local/apache-tomcat-7.0.68/bin/startup.sh
   ```

3. 在浏览器上输入 `http://ip:port/项目名`，验证 web 应用是否发布成功，例如：test 应用发布成功时界面会显示 PostgreSQL 数据库版本信息

![web_api][webtest]


[^_^]:
    本文使用的所有引用及链接
[webtest]:images/Manual/Webserverapp/Tomcat/webtest.jpg