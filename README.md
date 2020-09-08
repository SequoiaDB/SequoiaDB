更新说明：

大家好， 我们的开源项目“SequoiaDB”近期受到了此前GitHub被攻击的影响，代码库出现了混乱。之前我们和平台都努力尝试修复但是并没有解决问题，无奈最后只能将项目清除并重新上传项目代码，给您带来的不便和影响我们也深表歉意。 如有任何问题，请在issue向我们留言，或登录SequoiaDB官网（链接到联系我们）向我们的支持人员咨询

我们今后还会持续加大开源方面的投入，和大家共同构建开源社区，也希望大家今后多多支持！


### SequoiaDB README


#### About Us

SequoiaDB is a MySQL/PostgreSQL-compatible distributed relational database.

It supports ACID, horizontal scaling, HA/DR and Multi-model data storage engine.



#### **SequoiaDB Specific Characteristics**

- **Standard SQL access, MySQL compatibility**

  SequoiaDB supports standard SQL access, and achieves MySQL/PostgreSQL/SparkSQL compatibility in protocol level.

- **Financial-level distributed OLTP**

  As a financial-level multi-model distributed database, SequoiaDB is designed to support MySQL/PostgreSQL compatible OLTP. 

- **Distributed Architecture**

  SequoiaDB data storage engine applies native distributed architecture. The data are stored in different physical nodes, which could achieve the automated data distribution and management, as well as the flexible scalability. 

- **Multi-Model Data Engines**

  The flexible storage data types of SequoiaDB support structured, semi-structured and unstructured data, realizing the unified management of multi-model data. 

- **HTAP (Hybrid Transactional and Analytical Processing)**

  Through the support of SQL and the integration of Spark, SequoiaDB achieves the hybrid transactional and analytical processing, and agile and flexible business application implementation to handle more complicated business scenarios.



#### **Documentation**

- [English](http://www.sequoiadb.com/en/index.php?m=Files&a=index)

- [Chinese](http://doc.sequoiadb.com/cn/SequoiaDB)



#### **Architecture**

#### ![Architecture](https://s2.ax1x.com/2019/04/12/AbTjrn.png)



#### **Contact us**

- [Contact SequoiaDB Team](http://www.sequoiadb.com/cn/About)

- Twitter: [@_SequoiaDB](https://twitter.com/_SequoiaDB)
- [Medium](https://medium.com/@_SequoiaDB)



#### **License**

Most SequoiaDB source files are made available under the terms of the
GNU Affero General Public License (AGPL). See individual files for details.
All source files for clients, drivers and connectors are released
under Apache License v2.0.



#### **Restriction**

- SequoiaDB officially supports x86_64 and ppc64 Linux build on CentOS, Redhat, SUSE and Ubuntu.

- Windows build and 32 bit build are for testing purpose only.
