[^_^]:
    概述
    作者：谭钊波
    时间：20190228
    评审意见
    王涛：
    许建辉：
    市场部：20190325


用户的数据呈现出多样性，这些数据可以归纳为以下三类：

- 结构化数据：指表单类型的数据存储结构，如银行联机交易等传统业务使用的数据
- 半结构化数据：如用户画像、物联网设备日志采集、应用点击流分析等场景产生的数据
- 非结构化数据：指没有固定结构的数据，如海量的图片、音频、视频、和文档等数据

SequoiaDB 巨杉数据库支持 [JSON 存储][data_mode_document]与[块存储][data_mode_lob]，能够很轻松地满足用户对多样性数据的存储与管理的要求。

##数据模型架构##

SequoiaDB 巨杉数据库的数据管理模型如图 1 所示， 该模型展示了 SequoiaDB 的数据存储模型以及集群组成所涉及的基本概念。

![SequoiaDB 巨杉数据库数据模型架构图][data_mode_architecture_diagram]


##数据存储##

图 1 的数据模型中，数据文件最终都需要在磁盘文件中进行持久化存储，与之相关的三个概念如下：
- 页（Page）：页是数据库文件中用于组织数据的一种基本结构，SequoiaDB 使用页来对文件中的空间进行管理与分配
- 数据块（Extent）：由若干个页组成，用于存放记录
- 文件（File）：磁盘上的物理文件，用于持久化集合数据、索引以及 Lob 数据


###结构/半结构化数据存储###

在图 1 的数据模型中，与结构/半结构化数据存储相关的三个核心逻辑概念包括：

- [集合（Collection）][data_mode_collection]：又称为表（Table），用于存放文档的逻辑对象
- [集合空间（Collection Space）][data_mode_collection_space]：用于存储集合的对象，物理上对应于一组磁盘上的文件
- [文档（Document）][data_mode_document]：又称为记录（Record），以 [BSON（二进制化的 JSON）][data_mode_bson]结构存储在集合中

一个集合空间可以包含多个集合，一个集合会包含若干个数据块。集合使用链表把这些数据块串联起来。当向集合中插入文档时，需要从数据块中分配空间。如果当前数据块没有足够空间，后台线程将分配新的数据块（必要时对数据文件进行扩展），并把新的数据块挂到该集合的数据块链表上。每个数据块内的记录也通过链表的形式组织起来，这样在进行表扫描时，可顺序读取数据块内的所有记录。

###非结构化数据存储###

在结构/半结构化数据存储的基础上，与非结构化数据存储相关的核心逻辑概念包括：

- [大对象（Lob，Large Object）][data_mode_lob]：指基于块存储，用于存储如图片、音频、视频、文档等没有固定结构的数据

大对象依附于普通集合存在。当用户上传一个大对象时，系统为其分配一个唯一的 [OID][data_mode_oid] 值， 后续对该大对象的操作可通过该值来进行指定。

##集群组成##

在图 1 数据模型架构中，与集群组成相关的四个核心逻辑概念包括：

- [节点（Node）][data_mode_node]：存放集合记录的逻辑对象
- [复制组（Replica Group）][data_mode_group]：包含若干副本节点的逻辑对象
- [域（Domain）][data_mode_domain]：包含若干复制组的逻辑对象
- [实例（Instance）][data_mode_instance]：一份完整的集合记录所涉及的数据节点称为一个实例。在多副本的情况下，复制组中存在多份完整的集合记录，也就拥有了多个数据节点实例。在业务场景中，用户可以访问所有的实例，以充分利用冗余的多副本数据。

SequoiaDB 的集群管理相关概念的关系如图 2 所示。

![SequoiaDB 巨杉数据库数据集群概念关系图][data_mode_cluster_manager_concept]

在集群中，一个复制组可以包含 1~7 个节点。一个域可以包含一个或者若干个复制组，以提供给专门的业务使用。当把集合空间创建在一个域上的时候，该集合空间下的所有集合将根据该集合的分区键自动切分到域所包含的复制组中。集合的数据切分对提升性能起到极大的促进作用。关于集群和数据切分的详细介绍，可分别参考[复制组][data_mode_replication]及[分区][data_mode_sharding]章节的内容。

##索引##

在 SequoiaDB 中，[索引][data_mode_index]是一种特殊的数据对象。索引本身不作为保存用户数据的容器，而是作为一种特殊的元数据，用以提高数据访问的效率。

##全文索引##

SequoiaDB 通过与 Elasticsearch 配合提供全文检索能力。以此为基础，SequoiaDB 提供一种新类型的索引——[全文索引][data_mode_text_index]。该索引与普通索引的典型区别在于索引数据不是存放于 SequoiaDB 的数据节点的索引文件中，而是存储在 Elasticsearch 中。

##序列##

SequoiaDB 提供[自增字段][data_mode_sequence]能力。在创建集合时，用户可以指定一个或者多个字段为自增字段。

[^_^]:
    本文使用的所有链接及引用
[data_mode_architecture_diagram]:images/Distributed_Engine/Architecture/Data_Model/data_mode_architecture_diagram.png
[data_mode_cluster_manager_concept]:images/Distributed_Engine/Architecture/Data_Model/replica_group_concept.png

[data_mode_collection_space]:manual/Distributed_Engine/Architecture/Data_Model/collection_space.md
[data_mode_collection]:manual/Distributed_Engine/Architecture/Data_Model/collection.md
[data_mode_document]:manual/Distributed_Engine/Architecture/Data_Model/document.md
[data_mode_lob]:manual/Distributed_Engine/Architecture/Data_Model/lob.md
[data_mode_oid]:manual/Manual/Sequoiadb_Command/SpecialObjects/OID.md

[data_mode_index]:manual/Distributed_Engine/Architecture/Data_Model/index.md
[data_mode_text_index]:manual/Distributed_Engine/Architecture/Data_Model/text_index.md
[data_mode_sequence]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md
[data_mode_instance]:manual/Distributed_Engine/Architecture/Data_Model/instance.md

[data_mode_domain]:manual/Distributed_Engine/Architecture/domain.md
[data_mode_group]:manual/Distributed_Engine/Architecture/Sharding/architecture.md
[data_mode_node]:manual/Distributed_Engine/Architecture/Node/readme.md
[data_mode_replication]:manual/Distributed_Engine/Architecture/Replication/Readme.md
[data_mode_sharding]:manual/Distributed_Engine/Architecture/Sharding/Readme.md
[data_mode_bson]:http://www.bsonspec.org/