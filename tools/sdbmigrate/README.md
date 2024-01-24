Sdbmigrate - 数据迁移工具
==========================================

sdbimprt.sh 与 sdbexprt.sh 脚本是由原生的导入导出工具 sdbimprt、sdbexprt 扩展而来，以满足多集合空间、多集合和多并发场景下的导入导出需求。通过 --help 获取 sdbimprt.sh 与 sdbexprt.sh 详细的使用信息。

> Note:
>
> 1. sdbimprt.sh 较之原生的 sdbimprt 工具，新增配置文件功能（--conf 参数），支持以配置文件的方式配置多个需要导入的集合、集合空间。导入配置文件参考 conf/sample/import.conf。
>
> 2. sdbexprt.sh 较之原生的 sdbexprt 工具，新增并发导出功能（--jobs 参数），同时舍弃了原先繁杂的配置文件格式，转而采用了一种更清晰、易用的配置文件格式，新导出配置文件参考 conf/sample/export.conf。

###目录说明

```sh
.
├── bin
│   ├── common.sh          公共脚本
│   ├── sdbimprt.sh        导入工具脚本
│   └── sdbexprt.sh        导出工具脚本
└── conf
    └──sample
        ├── import.conf    导入配置文件样例
        └── export.conf    导出配置文件样例
```

###使用说明

1. 如果需要使用 --conf 参数，则需要编辑配置文件。

 ```lang-bash
 $ cp conf/sample/* conf/
 vim conf/import.conf          #vim conf/export.conf
 ```

2. 根据实际需要设置 sdbimprt.sh（sdbexprt.sh）的执行参数，并执行，以下为导入示例：

 ```lang-bash
 $ ./bin/sdbimprt.sh --conf ./conf/import.conf
 ```

3. 查看导入（导出）的结果文件，获取导入（导出）的实际情况。

 ```lang-bash
 $ vim sdbimport.result           #vim sdbexport.result
 ```

###注意事项

1. sdbexprt.sh 脚本的 --jobs 并发导出，是直接从多个数据组中并行导出数据。集合数据被切分得越离散、分布得越均匀，其并发导出性能越好。

2. 指定集合空间进行导出时，为了防止导出的数据重复，将不导出该集合空间下的主分区集合（逻辑集合，所导出数据与其子集合数据一致）

3. sdbimprt.sh 与 sdbexprt.sh 的参数支持在命令行与配置文件中填写。在命令行中填写参数时，该参数将对指定的所有集合、集合空间生效；在配置文件中填写时，该参数仅对某个集合或集合空间生效。配置文件中参数的优先级高于命令行中相同的参数。