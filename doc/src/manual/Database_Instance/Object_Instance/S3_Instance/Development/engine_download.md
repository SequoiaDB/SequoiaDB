[^_^]:
    S3驱动下载
    作者：常小龙
    时间：20190601
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190703


本文档主要介绍基于 Amazon S3 开发工具包的下载和安装。

AWS SDK for Java
----
Java 语言工具包可以通过 Maven POM 引用依赖或直接下载 jar 包导入工程的方式进行下载及安装。

-  **通过 Maven POM 引用**

 将此处代码粘贴在工程的 POM 文件中， 然后更新 Maven 工程
```lang-xml
<dependencies>
    <dependency>
        <groupId>com.amazonaws</groupId>
        <artifactId>aws-java-sdk-s3</artifactId>
        <version>1.11.343</version>
    </dependency>
</dependencies>
<dependencyManagement>
    <dependencies>
        <dependency>
            <groupId>com.amazonaws</groupId>
            <artifactId>aws-java-sdk-bom</artifactId>
            <version>1.11.343</version>
        </dependency>
    </dependencies>
</dependencyManagement>
```
- **下载 jar 包**

 用户可以直接[下载 aws-java-sdk-s3 驱动包][download_java]。

AWS SDK for C++
----
C++ 语言工具包可以通过直接[下载 jar 包][download_cpp]导入工程的方式进行安装及下载。

命令行工具
----
s3cmd 是一款 Amazon S3 命令行工具，可以通过以下命令在联网情况下进行下载安装。

1. 下载
```lang-bash
wget http://nchc.dl.sourceforge.net/project/s3tools/s3cmd/2.0.2/s3cmd-2.0.2.tar.gz
```

2. 安装
```lang-bash
tar -zxf s3cmd-2.0.2.tar.gz -C /usr/local/
mv /usr/local/s3cmd-2.0.2/ /usr/local/s3cmd/
ln -s /usr/local/s3cmd/s3cmd /usr/bin/s3cmd
```


[^_^]:
    相关连接

[download_java]:https://repo1.maven.org/maven2/com/amazonaws/aws-java-sdk-s3/1.11.343/aws-java-sdk-s3-1.11.343.jar
[download_cpp]:https://github.com/aws/aws-sdk-cpp/archive/1.7.114.zip