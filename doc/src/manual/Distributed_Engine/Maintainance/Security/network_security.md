SequoiaDB 巨杉数据库通过配置可以支持 SSL。SequoiaDB 客户端和 SequoiaDB 实例可以直接使用 SSL 加密连接。

SequoiaDB支持
----
若要使用 SSL 加密连接，需要 SequoiaDB 1.12 或更高版本。

目前该特性仅限于 SequoiaDB 企业版，社区版暂不支持。

客户端支持
----
所有官方支持的客户端驱动都支持 SSL，包括 C、C++、Java、Python、C#、PHP、REST API 及 SDB Shell。

##配置SequoiaDB使用SSL##

在安装部署时，通过配置参数开启 SequoiaDB 对 SSL 连接的支持：

--usessl，默认值为 false，设为 true 时开启 SSL，允许客户端通过 SSL 加密连接，同时仍然接受非 SSL 加密连接

```lang-ini
sequoiadb --usessl=true
```

该参数也支持使用配置文件配置，可参考[数据库配置][config]。

SequoiaDB 在开启 SSL 后会自动创建证书，不需要用户指定。

客户端使用SSL
----
客户端必须与开启 SSL 的 SequoiaDB 配合才能使用 SSL加密连接。所有官方支持的客户端驱动都支持 SSL。

*   C

    [C 驱动][c_driver]接口使用 sdbSecureConnect() 和 sdbSecureConnect1() 建立 SSL连接，使用方式与 sdbConnect() 和 sdbConnect1() 相同。

*   C++

    [C++ 驱动][cpp_driver]中类 `sdb` 的构造函数有参数 useSSL，设为 true 时使用 SSL 连接。

*   Java

    [Java 驱动][java_driver]中类 `com.sequoiadb.net.ConfigOptions` 有接口 setUseSSL( boolean useSSL )，设为 true 时使用 SSL 连接。

*   Python

    [Python 驱动][python_driver]中类 `client` 构造函数有可选参数 ssl，设为 true 时使用 SSL连接。

*   C#

    [C# 驱动][csharp_driver]中类 `SequoiaDB.ConfigOptions` 有属性 UseSSL，设为 true 时使用 SSL 连接。

*   PHP

    [PHP 驱动][php_driver]中有类 `SecureSdb`，该类是 SequoiaDB 的子类，类 `SecureSdb` 的对象使用 SSL 连接。

*   REST API

    [REST API][rest] 支持 https。

*   SDB Shell

    SDB Shell 中共有类 [SecureSdb][secure]，该类是 Sdb 的子类，类 SecureSdb 的对象使用 SSL 连接。


工具支持
----
[sdbexprt][expert]、[sdbimprt][imprt]、[sdblobtool][lobtool] 和 [sdbtop][top] 均支持 SSL 连接。


[^_^]:
     本文使用的所有链接和引用
[config]:manual/Distributed_Engine/Maintainance/Database_Configuration/configuration_parameters.md
[c_driver]:manual/Database_Instance/Json_Instance/Development/c_driver/Readme.md
[cpp_driver]:manual/Database_Instance/Json_Instance/Development/cpp_driver/Readme.md
[java_driver]:manual/Database_Instance/Json_Instance/Development/java_driver/Readme.md
[python_driver]:manual/Database_Instance/Json_Instance/Development/python_driver/Readme.md
[csharp_driver]:manual/Database_Instance/Json_Instance/Development/csharp_driver/Readme.md
[php_driver]:manual/Database_Instance/Json_Instance/Development/php_driver/Readme.md
[rest]:manual/Database_Instance/Json_Instance/Development/rest/Readme.md
[secure]:manual/Manual/Sequoiadb_Command/SecureSdb.md
[expert]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbexprt.md
[imprt]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbimprt.md
[lobtool]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/lobtools.md
[top]:manual/Distributed_Engine/Maintainance/Monitoring/sdbtop.md