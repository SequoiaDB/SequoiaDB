[^_^]:
    操作概述

完成存储集群和实例的部署后，用户可以使用 SAC 对主机、存储集群、MySQL 实例、MariaDB 实例、PostgreSQL 实例和 SequoiaDB 数据进行操作。 

本章将介绍 SAC 的一些常用操作，包含以下内容：

**主机操作**
 
  - [删除主机][remove_host] 
  - [更新主机信息][update_host_info]

**存储集群操作**

  - [管理存储集群][manage_module]
  - [重启存储集群][restart_module]
  - [删除存储集群][remove_module]

**SequoiaDB 数据操作**

  - [集合空间][collection_space]
  - [集合][collection]
  - [索引][index]
  - [记录][record]
  - [Lob][lob]

**MySQL 实例操作**

  - [数据库操作][database_mysql]
  - [数据操作][record_mysql]
  - [管理数据表][manage_mysql]
  - [管理实例][manage_mysql]
  - [重启实例][restart_mysql]
  - [删除实例][uninstall_mysql]
  - [移除实例存储][remove_mysql]

**PostgreSQL 实例操作**

  - [数据库操作][database_pg]
  - [数据操作][record_pg]
  - [管理数据表][manage_table_pg]
  - [管理实例][manage_pg]
  - [重启实例][restart_pg]
  - [删除实例][uninstall_pg]
  - [移除实例存储][remove_pg]

**MariaDB 实例操作**

  - [数据库操作][database]
  - [数据操作][record]
  - [管理数据表][manage_table]
  - [管理实例][manage_instance]
  - [重启实例][restart_instance]
  - [删除实例][uninstall]
  - [移除实例存储][remove]




[^_^]:
    本文使用的所有引用及链接
[remove_host]:manual/SAC/Operation/Host/remove_host.md
[update_host_info]:manual/SAC/Operation/Host/update_host_info.md

[manage_module]:manual/SAC/Operation/Module/manage_module.md
[restart_module]:manual/SAC/Operation/Module/restart_module.md
[remove_module]:manual/SAC/Operation/Module/remove_module.md

[manage_table]:manual/SAC/Operation/Mysql/manage_table.md
[database_mysql]:manual/SAC/Operation/Mysql/database.md
[record_mysql]:manual/SAC/Operation/Mysql/record.md
[manage_mysql]:manual/SAC/Operation/Mysql/manage_instance.md
[restart_mysql]:manual/SAC/Operation/Mysql/restart_instance.md
[uninstall_mysql]:manual/SAC/Operation/Mysql/uninstall_instance.md
[remove_mysql]:manual/SAC/Operation/Mysql/remove_instance_storage.md

[manage_pg]:manual/SAC/Operation/Postgresql/manage_instance.md
[database_pg]:manual/SAC/Operation/Postgresql/database.md
[record_pg]:manual/SAC/Operation/Postgresql/record.md
[manage_table_pg]:manual/SAC/Operation/Postgresql/manage_table.md
[restart_pg]:manual/SAC/Operation/Postgresql/restart_instance.md
[uninstall_pg]:manual/SAC/Operation/Postgresql/uninstall_instance.md
[remove_pg]:manual/SAC/Operation/Postgresql/remove_instance_storage.md

[collection_space]:manual/SAC/Operation/Sequoiadb_Data/collection_space.md
[collection]:manual/SAC/Operation/Sequoiadb_Data/collection.md
[index]:manual/SAC/Operation/Sequoiadb_Data/index.md
[record]:manual/SAC/Operation/Sequoiadb_Data/record.md
[lob]:manual/SAC/Operation/Sequoiadb_Data/lob.md
[sequoiadb_configs]:manual/SAC/Operation/Sequoiadb_Data/sequoiadb_configs.md

[add_mariadb_module]:manual/SAC/Deployment/Deployment_Bystep/deployment_mariadb.md
[manage_table]:manual/SAC/Operation/Mariadb/manage_table.md
[database]:manual/SAC/Operation/Mariadb/database.md
[record]:manual/SAC/Operation/Mariadb/record.md
[manage_instance]:manual/SAC/Operation/Mariadb/manage_instance.md
[restart_instance]:manual/SAC/Operation/Mariadb/restart_instance.md
[uninstall]:manual/SAC/Operation/Mariadb/uninstall_mariadb.md
[remove]:manual/SAC/Operation/Mariadb/remove_mariadb_storage.md
