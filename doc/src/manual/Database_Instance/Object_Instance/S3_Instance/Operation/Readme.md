SequoiaS3 系统实现通过 AWS S3 接口访问 SequoiaDB 巨杉数据库的能力。

SequoiaS3 将 S3 接口中的区域、桶和对象映射为 SequoiaDB 中的集合空间、集合、记录和 Lob，以实现桶的增删查、对象的增删查、对象的版本管理及分段上传的能力。