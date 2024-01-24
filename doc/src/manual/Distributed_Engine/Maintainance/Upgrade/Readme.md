[^_^]:
    升级
    作者：杨上德
    时间：20190818
    评审意见
    王涛：时间：
    许建辉：时间：
    市场部：时间：20190911


升级是将软件从较早版本升级到后续发布的较新版本，以获取新的功能，或者完成对特定软件问题的修复。

由于 SequoiaDB 巨杉数据库的分布式特性，根据升级期间是否需要停止对外服务，分为[离线升级][offline]和[滚动升级][rollingupgrade]两种模式。除少数特定版本外，SequoiaDB 均提供向后兼容能力，即直接升级软件即可，数据无需特殊处理。具体的兼容信息可查看[版本兼容列表][compatibility]。相同版本也可以升级，但不支持从高版本降级到低版本。

本章主要内容包括：
+ [离线升级][offline]
+ [滚动升级][rollingupgrade]

[^_^]:
    本文中用到的所有链接
[offline]:manual/Distributed_Engine/Maintainance/Upgrade/offline.md
[rollingupgrade]:manual/Distributed_Engine/Maintainance/Upgrade/rollingupgrade.md
[compatibility]:manual/Distributed_Engine/Maintainance/Upgrade/compatibility.md
