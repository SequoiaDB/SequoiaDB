[^_^]: 
    FlinkSQL 连接器-Readme

Apache Flink 是一个数据处理框架，支持处理流式实时数据。SequoiaDB 巨杉数据库通过 FlinkSQL 连接器实现与 Flink 对接，提供处理流式数据的能力。同时，通过并发直连数据节点分片读取、分批写入等方式，大大提高从 SequoiaDB 读取和写入数据的效率。

FlinkSQL 连接器实现了从 SequoiaDB 批量读取数据至 Flink 进行数据分析与处理，也能以批量或流式的方式将分析处理后的数据写入 SequoiaDB。 
