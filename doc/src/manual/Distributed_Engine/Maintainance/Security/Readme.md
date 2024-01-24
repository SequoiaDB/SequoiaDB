SequoiaDB 巨杉数据库通过鉴权、加密通信、审计日志和密码管理等保障系统安全。

本章将向用户介绍以下内容：
- [数据库系统安全][system_security]：指定登陆数据库的权限
- [网络安全][network_security]：配置并使用 SSL 加密连接
- [鉴权机制][authentication_algorithm]：SequoiaDB 3.4.1 之前的版本，支持 MD5 鉴权；SequoiaDB 3.4.1 及以上的版本，支持 SCRAM-SHA256 鉴权。
- [基于角色的权限管理][rbac]：通过角色来管理用户的权限

[^_^]:
    本文使用的所有引用和链接
[system_security]:manual/Distributed_Engine/Maintainance/Security/system_security.md
[network_security]:manual/Distributed_Engine/Maintainance/Security/network_security.md
[authentication_algorithm]:manual/Distributed_Engine/Maintainance/Security/authentication_algorithm.md
[rbac]:manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/Readme.md