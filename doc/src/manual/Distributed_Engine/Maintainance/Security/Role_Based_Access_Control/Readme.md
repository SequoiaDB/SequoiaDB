在SequoiaDB中Role-Based Access Control (RBAC) 是一种权限管理机制，它基于角色的概念来控制用户对数据库资源的访问权限。RBAC 允许管理员创建角色，并为这些角色分配特定的权限。然后，将角色授予用户，从而简化和灵活地管理用户权限。

本章将向用户介绍以下内容：
- [权限、资源和操作][privilege_resource_actions]：RBAC机制中的权限、资源和操作介绍
- [操作类型][action_types]：操作类型及对应命令
- [内建角色][builtin_roles]：内建角色介绍
- [用户自定义角色][user_defined_roles]：如何创建用户自定义角色
- [管理用户和角色][manager_users_and_roles]：如何管理用户和角色
- [访问控制][access_control]：如何通过角色来控制用户对数据库的操作权限
- [用户升级指南][user_update_guide]：如何将旧版本用户升级到基于角色的访问控制用户


[^_^]:
    本文使用的所有引用和链接
[privilege_resource_actions]:manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/privilege_resource_actions.md
[action_types]:manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/action_types.md
[builtin_roles]:manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/builtin_roles.md
[user_defined_roles]:manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/user_defined_roles.md
[manager_users_and_roles]:manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/manage_users_and_roles.md
[access_control]:manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/access_control.md
[user_update_guide]:manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/user_update_guide.md