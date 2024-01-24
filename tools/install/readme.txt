服务器软硬件要求：http://doc.sequoiadb.com/cn/sequoiadb-cat_id-1461056740-edition_id-0
Linux环境推荐配置：http://doc.sequoiadb.com/cn/sequoiadb-cat_id-1461057844-edition_id-0

setup.sh支持以下功能：
  1.默认使用mode text模式安装sequoiadb，提供用户选择安装sequoiasql-mysql或者sequoaisql-postgresql，默认选择sequoaisql-mysql
  2.提供可选参数--sdb，表示安装sequoiadb
  3.提供可选参数--mysql，表示安装sequoiasql-mysql
  4.提供可选参数--pg，表示安装sequoiasql-postgresql 
setup.sh --clean支持以下功能：
  1.用户可以指定--sdb只清除 SequoiaDB 的安装和数据
  2.用户可以指定--mysql只清除 MySQL 实例组件的安装和数据
  3.用户可以指定--pg只清除 PostgreSQL 实例组件的的安装和数据
  4.用户可以指定--force强制清除通过setup.sh安装的所有软件
  5.用户可以指定--local清除当前机器存在的所有 SequoiaDB、MySQL 实例组件以及 PostgreSQL 实例组件
更多可参考：http://doc.sequoiadb.com/cn/sequoiadb-cat_id-1519612914-edition_id-0
