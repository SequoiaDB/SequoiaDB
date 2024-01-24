SequoiaDB 巨杉数据库除了基本类型对象，还包含特殊类型对象。

特殊类型对象如下：

| 名称 | 描述 |
|------|------|
| [BinData][BinData] | Base64 形式的二进制数据 |
| [BSONArray][BSONArray] | BSON 数组 |
| [BSONObj][BSONObj] | BSON 对象 |
| [MaxKey][MaxKey] | 所有数据类型中的最大值 |
| [MinKey][MinKey] | 所有数据类型中的最小值 |
| [NumberDecimal][NumberDecimal] | 高精度数，可以保证精度不丢失 |
| [NumberLong][NumberLong] | 长整型 |
| [OID][OID] | 对象 ID 为一个 12 字节的 BSON 数据类型 |
| [Regex][Regex] | 正则表达式 |
| [SdbDate][SdbDate] | YYYY-MM-DD 形式的日期 |
| [Timestamp][Timestamp] | YYYY-MM-DD-HH.mm.ss.ffffff 形式的时间戳 |

[^_^]:
     本文使用的所有引用及链接
[BinData]:manual/Manual/Sequoiadb_Command/SpecialObjects/BinData.md
[BSONArray]:manual/Manual/Sequoiadb_Command/SpecialObjects/BSONArray.md
[BSONObj]:manual/Manual/Sequoiadb_Command/SpecialObjects/BSONObj.md
[MaxKey]:manual/Manual/Sequoiadb_Command/SpecialObjects/MaxKey.md
[MinKey]:manual/Manual/Sequoiadb_Command/SpecialObjects/MinKey.md
[NumberDecimal]:manual/Manual/Sequoiadb_Command/SpecialObjects/NumberDecimal.md
[NumberLong]:manual/Manual/Sequoiadb_Command/SpecialObjects/NumberLong.md
[OID]:manual/Manual/Sequoiadb_Command/SpecialObjects/OID.md
[Regex]:manual/Manual/Sequoiadb_Command/SpecialObjects/Regex.md
[SdbDate]:manual/Manual/Sequoiadb_Command/SpecialObjects/SdbDate.md
[Timestamp]:manual/Manual/Sequoiadb_Command/SpecialObjects/Timestamp.md