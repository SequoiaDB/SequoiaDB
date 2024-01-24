
1. 在 eclipse 中创建 Java web 项目（如 HelloEJB）并导出 war 包

   ![eclipse中创建Java web项目][web-0]

2. 进入【Runtime】->【Deployments】界面中，点击【Manage eployments】->【Add Content】，从本地选择 `HelloEJB.war` 包上传到 JBoss 服务器

   ![上传war到jboss服务][web-2]

3. 项目上传到 JBoss 服务器后，选择【HelloEJB.war】点击 **Enable** 启用

   ![启用war][web-3]

4. 在浏览器上输入 `http://ip:port/项目名`（如 `http://192.168.31.8:8080/HelloEJB/`），
网页上输出数据库的主版本号说明成功部署

   ![查看运行效果][web-6]



[^_^]:
    本文使用的所有引用及链接
[web-0]:images/Manual/Webserverapp/Jboss/web-0.png
[web-2]:images/Manual/Webserverapp/Jboss/web-2.png
[web-3]:images/Manual/Webserverapp/Jboss/web-3.png
[web-6]:images/Manual/Webserverapp/Jboss/web-6.png

