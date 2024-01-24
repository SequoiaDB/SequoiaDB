本文档主要介绍 WebSphere 的部署。


##环境准备##

用户可参考 [WebSphere Application Server detailed system requirements][websphere]，检查系统是否满足 WebSphere 安装的软硬件要求。

##安装配置##

用户需要自行从 IBM 官网下载 WebSphere 的试用版，并将下载的安装包拷贝到安装服务器上。

下述示例以 WebSphere 试用版 `was.cd.70011.trial.base.opt.linux.ia32.tar.gz` 为例介绍安装与配置步骤。示例中 WebSphere 安装包位于 `/opt/web/packet`，WebSphere 安装目录为 `/opt/IBM/WebSphere/AppServer`。

1. 解压并安装 WebSphere

   ```lang-bash
   suse113-1:~ # cd /opt/web/packet/
   suse113-1:/opt/web/packet # tar -xzvf was.cd.70011.trial.base.opt.linux.ia32.tar.gz
   suse113-1:/opt/web/packet # cd WAS/
   suse113-1:/opt/web/packet/WAS # ./install
   ```

2. 弹出安装界面，点击 **Next**

   ![install-1][install_1]

3. 选择 【I accept both the IBM and the non-IBM terms】后点击 **Next**

   ![install-2][install_2]

4. 点击 **Next**

   ![install-3][install_3]

5. 勾选所有选项后点击 **Next**

   ![install-4][install_4]

6. 选择安装路径后点击 **Next**

   ![install-5][install_5]

7. 点击 **Next**

   ![install-6][install_6]

8. 填写用户名和密码后点击 **Next**

   ![install-7][install_7]

9. 点击 **Next**

   ![install-8][install_8]

10. 点击 **Next**

   ![install-9][install_9]

11. 点击 **Next**，开始安装

   ![install-10][install_10]

12. 点击 **Finish**

   ![install-11][install_11]

13. 点击 **Installation verfiication**

  ![install-12][install_12]

14. 服务自启动，并且提示安装校验完成，表示完成安装（完成后窗口可关闭）

  ![install-13][install_13]


[^_^]:
    本文使用的所有引用及链接
[websphere]:http://www-01.ibm.com/support/docview.wss?rs=180&uid=swg27006921
[install_1]:images/Manual/Webserverapp/Websphere/install_1.jpg
[install_2]:images/Manual/Webserverapp/Websphere/install_2.jpg
[install_3]:images/Manual/Webserverapp/Websphere/install_3.jpg
[install_4]:images/Manual/Webserverapp/Websphere/install_4.jpg
[install_5]:images/Manual/Webserverapp/Websphere/install_5.jpg
[install_6]:images/Manual/Webserverapp/Websphere/install_6.jpg
[install_7]:images/Manual/Webserverapp/Websphere/install_7.jpg
[install_8]:images/Manual/Webserverapp/Websphere/install_8.jpg
[install_9]:images/Manual/Webserverapp/Websphere/install_9.jpg
[install_10]:images/Manual/Webserverapp/Websphere/install_10.jpg
[install_11]:images/Manual/Webserverapp/Websphere/install_11.jpg
[install_12]:images/Manual/Webserverapp/Websphere/install_12.jpg
[install_13]:images/Manual/Webserverapp/Websphere/install_13.jpg