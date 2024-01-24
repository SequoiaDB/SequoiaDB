
JBoss 的数据连接配置其实就是配置数据源，在 JBoss 中配置数据源的方式有两种：
- 根据创建模型来创建数据源
- 通过热部署来创建数据源（和部署 web 应用相似）

热部署方式相对于创建模型方式更方便快捷且比较灵活，因此本次配置数据源选择热部署方式。

1. 复制数据库驱动，将 `postgresql-42.0.0.jre7.jar` 复制到 `/opt/jboss/standalone/deployments/` 目录下

    ```lang-javascript
    $ cp postgresql-42.0.0.jre7.jar /opt/jboss/standalone/deployments/
    ```

2. 打开浏览器，地址为 JBoss 绑定的 IP，访问端口默认 9990，例如 `http://192.168.31.8:9990`

3. 输入用户名和密码，用户名是部署时创建的 JBoss 后台用户

4. 点击 **登录** 按钮

5. 进入【Runtime】->【Deployments】界面，可以看到已部署但未启用的驱动，手动点击驱动后的 **Enable** 按钮进行启用

   ![登录后台管理界面][web-3]

6. 点击 **Add content** 添加数据库 点击 **Enable** 启用数据库驱动
   ![新增数驱动][web-3]

7. 进入【Profile】->【DataSources】，点击 **Add** 创建 JNDI

   ![创建JNDI][ds-2]

   ![创建JNDI][ds-3]

    >**Note:**
    >
    >在 JBoss 中 JNDI 的名字要以 Java:jboss/ 开头。JNDI 是由 sun 公司提供的 Java 命名系统接口，通过将名字与服务建立逻辑关联，从而通过不同的名字访问不同的服务。

8. 选择 `postgresql-42.0.0.jre7.jar` 驱动（用户可根据实际情况自行选择）

   ![选择驱动][ds-4]

9. 配置数据源连接

   ![创建数据源地址][ds-5]

   > **Note:**
   >
   > Connectiion url 的格式为 jdbc:postgresql://host:port/DBName，用户一定要严格按照此格式书写，此处用户名和密码必须在 PostgreSQL 中存在。

10. 点击 **Enable** 启动 postDS 数据源

   ![启用postDS数据源][ds-6]

11. 在【Selection】->【Connection】页面点击 **test connnection**，测试数据源是否成功配置
   ![部署成功][ds-7]



[^_^]:
    本文使用的所有引用及链接
[web-3]:images/Manual/Webserverapp/Jboss/web-3.png
[ds-2]:images/Manual/Webserverapp/Jboss/ds-2.png
[ds-3]:images/Manual/Webserverapp/Jboss/ds-3.png
[ds-4]:images/Manual/Webserverapp/Jboss/ds-4.png
[ds-5]:images/Manual/Webserverapp/Jboss/ds-5.png
[ds-6]:images/Manual/Webserverapp/Jboss/ds-6.png
[ds-7]:images/Manual/Webserverapp/Jboss/ds-7.png
