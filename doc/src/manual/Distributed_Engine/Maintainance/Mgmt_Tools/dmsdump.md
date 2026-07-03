[^_^]:
    数据库检测工具


sdbdmsdump 是 SequoiaDB 巨杉数据库的数据文件检查工具，用于检测数据库文件结构的正确性，并且给出检测结果报告。

> **Note:**
>
> sdbdmsdump 在 1.8 版本前名为 sdbinspt，1.8 版本后更名为 sdbdmsdump。

运行需求
----

- sdbdmsdump 不需要与数据库连接。
- 运行 sdbdmsdump 命令的用户必须对数据库的数据与索引文件拥有读权限。

语法规则
----
```lang-text
sdbdmsdump <--action | -a arg> [--dbpath | -d arg] [--output | -o arg] [ --dumpdata |-t arg] [--dumpindex | -i arg] [--dumplob | -b arg] [--crccheck | -C arg] [--record | -p arg]

sdbdmsdump <--action | -a arg> [--dbpath | -d arg] [--output | -o arg] [ --csname | -c arg] [--dumpdata | -t arg]

sdbdmsdump <--action | -a arg> [--dbpath | -d arg] [--indexpath | -x arg] [--output | -o arg] [--verbose | -v arg] [ --csname | -c arg] [--dumpindex | -i arg]

sdbdmsdump <--action | -a arg> [--dbpath | -d arg] [ --dumpdata |-t arg]

sdbdmsdump --help | -h

sdbdmsdump --version | -v
```

参数说明
----

| 参数名      | 缩写 | 描述                                                                                     |
| ----        | ---- | ----                                                                                     |
| --help      | -h   | 返回基本帮助和用法文本                                                                   |
| --dbpath    | -d   | 指定数据库文件所在目录，默认为当前目录                                                   |
| --indexpath | -x   | 指定索引文件所在目录，默认为 dbpath                                                      |
| --lobpath   | -g   | 指定 Lob 文件所在目录，默认为 dbpath                                                     |
| --lobmpath  | -m   | 指定 Lob Meta 文件所在目录，默认为 dbpath                                                |
| --output    | -o   | 指定输出文件或目录，默认为屏幕输出。指定为目录时，生成的文件前缀为 'dmsdump'。<br> 输出文件时，会自动加 '.%seq_no%' 的后缀，'seq_no' 从 0 递增，当文件大小超过 '--maxfilesize' 时进行切换                    |
| --verbose   | -v   | dump操作时，是否进行格式化输出（取值为 true 或者 false），默认值为 true                  |
| --hex       | -e   | dump操作时，是否输出十六制原始数据（取值为 true 或者 false），默认值为 true              |
| --csname    | -c   | 指定集合空间名，如果不指定则为全部集合空间                                               |
| --clname    | -l   | 指定集合名，如果不指定则为全部集合                                                       |
| --action    | -a   | 指定操作，必须指定值，但不能和 repair 同时使用，取值列表如下：<br> "inspect"：检测并报告任何数据损坏 <br> "dump"：将数据页格式化并输出 <br> "all"：检测数据页损坏，并格式化输出数据页                         |
| --dumpdata  | -t   | 指定操作数据文件（取值为 true 或 false），默认值为 false                                 |
| --dumpindex | -i   | 指定操作索引文件（取值为 true 或 false），默认值为 false                                 |
| --dumplob   | -b   | 指定操作 Lob 文件（取值为 true 或者 false），默认值为 false                              |
| --crccheck  | -C   | inspect 操作时，是否校验 Lob 数据页 CRC（取值为 true 或 false），默认值为 false。<br> 开启后会逐页回读 Lob 数据文件并重算 CRC 进行比对，耗时明显高于普通 inspect，故默认关闭，需要时显式开启。仅在 `--dumplob true` 且 `--action inspect`（或 all）时生效 |
| --pagestart | -s   | dump操作时，指定起始数据页，默认为 -1                                                    |
| --numpage   | -n   | dump操作时，指定数据页数量，默认值为 1；当指定 -s 参数为非负值时，该参数生效             |
| --record    | -p   | dump操作时，指定显示格式化输出数据或索引内容（取值为 true 或 false），默认值为 false     |
| --meta      |      | 指定是否只操作元数据（取值为 true 或 fasle ），默认值为 false                            |
| --maxfilesize|     | 指定输出单个文件的最大大小，单位：MB，取值：[10, 524288]，默认值为 500                   |
| --force     |      | 强制输出无效的 mb 、delete list 与 index list 等（取值为 true 或 false），默认值为 false |

执行报告
---

* 运行参数报告，如下：

   ```lang-text
   Run Options   :
   Database Path : /opt/sequoiadb/database/data/11830
   Index path    : /opt/sequoiadb/database/data/11830
   Lobm path     : /opt/sequoiadb/database/data/11830
   Lob path      : /opt/sequoiadb/database/data/11830
   Output File   : outputdump.txt
   Max file size : 500
   Verbose       : True
   Hex           : True
   CS Name       : {all}
   CL Name       : {all}
   Action        : dump
   Repaire       :
   Dump/Inspect Options  :
      Dump Data  : True
      Dump Index : True
      Dump Lob   : True
      Start Page : -1
      Num Pages  : 1
      Show record: False
      Only Meta  : False
      force      : False
   ```

* 执行结果报告，如下：

   ```
   ++++++++  Result report  ++++++++
     Start Time             : 2024-09-24-00.20.04.350000
     End Time               : 2024-09-24-00.20.19.358000
     Cost                   : 15.01 (sec)
     Total Errors           : -
     Output File Num        : 1
     Total Data File Num    : 10
     Total Index File Num   : 10
     Total Lob File Num     : 1
     Total Data File Size   : 1.758 (GB)
     Total Index File Size  : 1.844 (GB)
     Total Lob File Size    : 6.830 (GB)
     Total File Size        : 10.433 (GB)
     Total Free Page Size   : 2.993 (GB)
     Total Free Tail Size   : 2.887 (GB)
     Total Free Size        : 3.062 (GB)
     Data Read IO Detail Info :
       Read Bytes           : 5.023 (GB)
       IO Read Bytes        : 5.277 (GB)
       Read Times           : 77917
       IO Read Times        : 18029
       Min Read Len         : 36
       Max Read Len         : 67108864
       Read Speed           : 342.72 (MB/S)
       Read Cache Ratio     : 76.86%
     Output Write IO Detail Info :
       Write Times          : 78482
       Write Bytes          : 468.968 (MB)
       Write Speed          : 31.25 (MB/S)
   ```

* 执行项报告

  当一项任务（如 Header、SME、MME 等）执行成功时，则打印 __"... Done Succeed"__，反之则打印 __"... Done with Errors: "__，且会详细打印错误项信息： __"\*\*\* Error: ... "__，示例：

   ```
   *** Error: Collection records is not the same[mb->_totalRecords: 10000, ext total records: 9800]
   ```


示例
----

* sdbdmsdump 在当前目录下检测所有集合空间和集合的文件结构，格式化输出数据和索引文件的检测结果至 `output.txt` 文件

   ```lang-bash
   $ sdbdmsdump -d . -o output.txt -a all -t true -i true -b true -p true
   ```

* 指定数据文件目录 `/opt/sequoiadb/database/data/11830`，指定操作为“inspect”检测并报告数据损坏信息，格式化输出集合空间 sample 的数据文件检测结果至 `outputdata.txt` 文件

   ```lang-bash
   $ sdbdmsdump -d /opt/sequoiadb/database/data/11830 -o outputdata.txt -c sample -a inspect -t true
   ```

   生成结果文件 `outputdata.txt.<num>`，记录集合空间 sample 数据文件中 Header 段、SME 空间段、MME 元数据管理段、数据段等结构检测结果信息，以及该集合空间下所有集合数据信息（如记录总数、使用数据页、溢出记录数、压缩记录数等），数据无损坏则检测结果记录为 "Done Succeed" ，显示内容可能如下：

   ```lang-text
   Inspect collection space ./sample.1.data
     Inspect Storage Unit Header: sample
     Inspect Storage Unit Header Done Succeed
   
     Inspect Space Management Extent:
     Inspect Space Management Extent Done Succeed   [0.01 s]
   
     Inspect Metadata Management Extent:
     Inspect Metadata Management Extent Done Succeed
    
     Inspect Data for collection [0 : employee]
     Inspect Data for collection Done Succeed   [0.02 s]
       ++++ The collection data info ++++
       Total Record              : 100054
       Total Data Pages          : 264
       Total Data Free Space     : 82944 (0.48%)
       Total OVF Record          : 0 (0.00%)
       Total Compressed Record   : 0 (0.00%)
       Total Deleting Record     : 0

     Inspect Space Management Extent:
     Inspect Space Management Extent Done Succeed   [0.00 s]

   Inspect collection space is Done Succeed   [0.06 s]
   ++++ The collection space info ++++
     File Size       : 532.06 (MB)
     Pages Num       : 8192
     Used Pages      : 273
     Max Used PageID : 272
     Free Page Size  : 494.94 (MB)
     Free Tail Size  : 494.94 (MB)
     Total Free Size : 495.02 (MB)
   ```

* 指定索引文件目录 `/opt/sequoiadb/database/data/11830`，指定操作为“inspect”检测并报告数据损坏信息，输出集合空间 sample 的索引数据检测结果至 `outputindex.txt` 文件

   ```lang-bash
   $ sdbdmsdump -d /opt/sequoiadb/database/data/11830 -x /opt/sequoiadb/database/data/11830 -o outputindex.txt -c sample -a inspect -i true
   ```

   生成结果文件 `outputindex.txt.<num>`，记录集合空间 sample 索引文件中 Header 段、SME 空间管理段、数据段检测结果，以及该集合空间下所有集合的索引信息（如索引页、索引空闲空间、唯一索引个数等），无损坏则检查结果记录为 "Done Succeed" ，显示内容可能如下：

   ```lang-text
   Inspect collection space /opt/sequoiadb/database/data/11830/sample.1.idx
     Inspect Storage Unit Header: sample
     Inspect Storage Unit Header Done Succeed
   
     Inspect Space Management Extent:
     Inspect Space Management Extent Done Succeed   [0.01 s]
   
     Inspect Metadata Management Extent:
     Inspect Metadata Management Extent Done Succeed
    
     Inspect Index for collection [0 : employee]
       Inspect Index [00] Control Block Extent 0x00000000 (0):
       Inspect Index Control Block Extent Done Succeed

       Inspect Index [01] Control Block Extent 0x000000df (223):
       Inspect Index Control Block Extent Done Succeed
   
       Index Inspection for Collection [0000], Index [00] ($id)
       Index Inspection Done Succeed
         IndexPage        : 81
         TotalFreeSpace   : 2339634 (44.62%)
         MaxLevel         : 2
         KeyNodeNum       : 100054
         DelKeyNodeNum    : 0 (0.00%)

       Index Inspection for Collection [0000], Index [01] (idx1)
       Index Inspection Done Succeed
         IndexPage        : 55
         TotalFreeSpace   : 1036460 (29.29%)
         MaxLevel         : 2
         KeyNodeNum       : 100054
         DelKeyNodeNum    : 0 (0.00%)

     Inspect Index for collection Done Succeed   [0.05 s]
     ++++ The collection index info ++++
       Total Index Pages        : 136
       Total Index Free Space   : 3376094 (38.44%)
       Unique Index Number      : 1
       Global Index Number      : 0

     Inspect Space Management Extent:
     Inspect Space Management Extent Done Succeed   [0.00 s]

   Inspect collection space is Done Succeed   [0.07 s]
   ++++ The collection space info ++++
     File Size       : 656.06 (MB)
     Pages Num       : 10240
     Used Pages      : 136
     Max Used PageID : 223
     Free Page Size  : 631.50 (MB)
     Free Tail Size  : 626.00 (MB)
     Total Free Size : 634.72 (MB)
   ```

* 指定数据目录 `/opt/sequoiadb/database/data/11830`，指定操作为“inspect”检测并报告数据损坏信息，输出集合空间 sample 的大对象数据检测结果至 `outputlob.txt` 文件

   ```lang-bash
   $ sdbdmsdump -d /opt/sequoiadb/database/data/11830 -o outputlob.txt -c sample -a inspect -b true
   ```

   生成结果文件 `outputlob.txt.<num>`，记录集合空间 sample 大对象文件中 Header 段、SME 空间管理段、BME 桶管理段、数据段检测结果，以及该集合空间下所有集合的大对象信息（如数据页个数、大对象总大小等），无损坏则检查结果记录为 "Done Succeed" ，显示内容可能如下：

   ```lang-text
   Inspect collection space /opt/sequoiadb/database/data/11830/sample.1.lobm
     Inspect Storage Unit Header: sample
     Inspect Storage Unit Header Done Succeed
   
     Inspect Space Management Extent:
     Inspect Space Management Extent Done Succeed   [0.01 s]

     Inspect Storage Unit Header: sample
     Inspect Storage Unit Header Done Succeed

     Inspect Bucket Management Extent:
     Inspect Bucket Management Extent Done Succeed   [0.11 s]
     Bucket Management Extent Balance:
       Valid Bucket Count    : 25649
       Min Valid Bucket Pos  : 1057
       Max Valid Bucket Pos  : 16775625
       Valid Page Count      : 27407
       Invalid Page Count    : 0
       Total Lob Count       : 17500
       Max Bucket Depth      : 4
       Min Bucket Depth      : 1
       Average Bucket Depth  : 1.068541
       Variance              : 0.070861 (Less is Better)
         00-Depth Number     : 16751567 (99.85%)
         01-Depth Number     : 23978 (93.49%)
         02-Depth Number     : 1587 (6.19%)
         03-Depth Number     : 81 (0.32%)
         04-Depth Number     : 3 (0.01%)
    
     Inspect Lob for collection [0 : employee]
     Inspect Lob for collection Done Succeed   [0.02 s]
     ++++ The collection lob info ++++
       Collection ID     : 0
       Total Lob Pages   : 27407
       Total Lob Size    : 2614220545
       Lob Usage Rate    : 36.39%
       Total Lobs        : 17500
       Page CRC Check    : skipped (use --crccheck true)

     Inspect Space Management Extent:
     Inspect Space Management Extent Done Succeed   [0.00 s]

   Inspect collection space is Done Succeed   [0.15 s]
   ++++ The collection space info ++++
     File Size       : 6994.12 (MB) (lobm and lobd)
     Pages Num       : 27648
     Used Pages      : 27407
     Max Used PageID : 27406
     Free Page Size  : 60.25 (MB)
     Free Tail Size  : 60.25 (MB)
     Total Free Size : 60.25 (MB)
   ```

   其中 "Page CRC Check" 反映 Lob 数据页 CRC 校验结果。默认不校验，显示为 `skipped (use --crccheck true)`；仅当增加 `--crccheck true` 时才逐页比对，此时显示 `Pass N, Fail M, NoCRC K`（`Pass` 为校验通过页数，`Fail` 为校验失败页数，`NoCRC` 为未携带 CRC 而跳过的页数，如老版本数据、非全写页）。

* 在上例基础上增加 `--crccheck true` 开启 Lob 数据页 CRC 校验，用于检测数据页是否损坏

   ```lang-bash
   $ sdbdmsdump -d /opt/sequoiadb/database/data/11830 -o outputlob.txt -c sample -a inspect -b true -C true
   ```

   校验通过时，"Page CRC Check" 显示各类页数统计；若存在损坏页，则每个损坏页额外打印一条明细，并计入 `Fail`，同时该集合的检测结果记录为 "Done with Errors"，显示内容可能如下：

   ```lang-text
     Inspect Lob for collection [0 : employee]
   *** Error: Lob data page(1036) crc mismatch, expect[0x1a2b3c4d] actual[0x5e6f7a8b] dataLen[262144]
     Inspect Lob for collection Done with Errors: 1   [0.85 s]
       ++++ The collection lob info ++++
       Collection ID     : 0
       Total Lob Pages   : 27407
       Total Lob Size    : 2614220545
       Lob Usage Rate    : 36.39%
       Total Lobs        : 17500
       Page CRC Check    : Pass 25647, Fail 1, NoCRC 1759
   ```

   > **Note:**
   >
   > `expect` 为 Lob 元数据（lobm）中记录的 CRC，`actual` 为当前数据页（lobd）重算得到的 CRC，`dataLen` 为参与计算的数据长度。二者不一致说明数据页或其 CRC 记录已损坏。

* 指定数据文件目录 `/opt/sequoiadb/database/data/11830`，指定集合空间 sample 设定操作数据文件，指定操作为“dump”将数据页格式化并输出至 `datadump.txt` 文件

   ```lang-bash
   $ sdbdmsdump -d /opt/sequoiadb/database/data/11830 -o datadump.txt -c sample -a dump -t true
   ```

   生成结果文件 `datadump.txt.<num>`，记录集合空间 sample 数据文件中 Header 段、SME 空间管理段、MME 元数据管理段和数据段存储结构信息。

   Header 段记录集合空间名与存储单元头的格式化内容，根据用户选择 verbose，输出可视化的格式化结果（相似内容以“……”省略）

   ```lang-text
   Dump collection space /opt/sequoiadb/database/data/11830/sample.1.data
   Storage Unit Header Dump:
   0x0000000000000000 : 5344 4244 4154 4100 0300 0000 0000 0100   SDBDATA.........
   0x0000000000000010 : 4109 0000 7465 7374 0000 0000 0000 0000   A...sample......
   0x0000000000000020 : 0000 0000 0000 0000 0000 0000 0000 0000   ................
   *……
   *
    Eye Catcher       : SDBDATA
    Version           : 3
    Page Size         : 65536
    Total Size        : 2369
    SU Name           : sample
    Sequence          : 1
    Num of Col        : 2
    HWM of Col        : 2
    Page Num          : 2048
    Secret value      : 0x5c8a050f5c19b9e6 (6668147761803278822)
    Lob Page Sz       : 262144
    Lob Flag          : 0
    Commit Flag       : 1
    Commit LSN        : 0x00000000c5744ed8 (3312733912)
    Commit Time       : 2024-09-23-22.36.25.884000 (1727102185884)
    Unique ID         : 0x000244d8 (148696)
    Segment Size      : 134217728
    Create Timestamp  : 2024-09-23-18.26.22.532000 (1727087182532)
    Update Timestamp  : 2024-09-23-22.36.25.884000 (1727102185884)
   ```


   格式化输出 SME 段可视化结果，显示这个存储单元中已经被占用和未被占用的数据页（0 表示该数据页空闲，1 表示数据页被使用）

   ```lang-text
   Space Management Extent Dump:
    0000000000 - 0000001250 [ 0x01 (Occupied) ]
    0000001251 - 0134217727 [ 0x00 (Free) ]
    Total: 134217728, Allocated: 2048, Used: 1251, HWMPageID: 1451
    Has errors: FALSE
   ```

   最后输出 MME 段格式化数据，在 MME 中首先对该集合空间中每个集合打印其 1024 字节的裸元数据，同时输出格式化数据

   ```lang-text
   Metadata Management Extent Dump:
   0x0000000000000000 : 6261 7200 0000 0000 0000 0000 0000 0000   employee........
   0x0000000000000010 : 0000 0000 0000 0000 0000 0000 0000 0000   ................
   *
   0x0000000000000080 : 0100 0000 0900 0000 1102 0000 0200 0000   ................
   0x0000000000000090 : FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF   ................
   0x00000000000000A0 : FFFF FFFF FFFF FFFF 1002 0000 24FF 0000   ............$...
   0x00000000000000B0 : 0900 0000 E8FE 0000 FFFF FFFF FFFF FFFF   ................
   0x00000000000000C0 : FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF   ................
   0x00000000000000D0 : FFFF FFFF FFFF FFFF 1102 0000 50D3 0000   ............P...
   0x00000000000000E0 : FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF   ................
   *
   0x0000000000000130 : 0000 0000 0200 0000 FFFF FFFF FFFF FFFF   ................
   0x0000000000000140 : FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF   ................
   *
   0x0000000000000230 : 0000 0000 0200 0000 0000 0000 FFFF FFFF   ................
   0x0000000000000240 : FFFF FFFF 0000 0000 A086 0100 0000 0000   ................
   0x0000000000000250 : 0902 0000 6100 0000 CCEB 0100 0000 0000   ....a...........
   0x0000000000000260 : 7592 0C00 0000 0000 0000 0000 FFFF FFFF   u...............
   0x0000000000000270 : FFFF FFFF FFFF FFFF 00FF 6401 0000 0000   ..........d.....
   0x0000000000000280 : 0000 0000 3ACF B801 0000 0000 3ACF B801   ....:.......:...
   0x0000000000000290 : 0000 0000 0000 0000 0000 0000 0000 0000   ................
   0x00000000000002A0 : 0000 0000 0100 0000 104D C9BE 0000 0000   .........M......
   0x00000000000002B0 : 661B 0980 6901 0000 0100 0000 0083 C2BC   f...i...........
   0x00000000000002C0 : 0000 0000 671B 0980 6901 0000 0000 0000   ....g...i.......
   0x00000000000002D0 : FFFF FFFF FFFF FFFF 0000 0000 0000 0000   ................
   0x00000000000002E0 : FFFF FFFF 0100 0000 1C09 0000 0000 0000   ................
   0x00000000000002F0 : 0000 0000 0000 0000 0000 0000 0000 0000   ................
   *
   Collection name   : employee
    CL Unique ID      : 0x0000091C00000001 (10015863734273)
    Flag              : 0x0001 (Used)
    Attributes        : 0x0000 ()
    Collection ID     : 0
    First extent ID   : 0x00000009 (9)
    Last extent ID    : 0x00000211 (529)
    Logical ID        : 0x00000000 (0)
    Index HWM         : 2
    Number of indexes : 2
    First Load ExtID  : 0xffffffff (-1)
    Last Load ExtID   : 0xffffffff (-1)
    Expand extent ID  : 0x00000000 (0)
    Total records     : 100000
    Total lobs        : 0
    Total data pages  : 521
    Total data free sp: 125900
    Total index pages : 97
    Total idx free sp : 823925
    Total lob pages   : 0
    Total org data len: 28888890
    total data len    : 28888890
    Dict extent ID    : 0xffffffff (-1)
    New Dict extent ID: 0xffffffff (-1)
    Dict stat page ID : 0xffffffff (-1)
    Dictionary version: 0
    Compression Type  : 0xff (none)
    Last comp ratio   : 100
    Data Commit Flag  : 1
    Data Commit LSN   : 0x00000000bec94d10 (3200863504)
    Data Commit Time  : 2019-03-15-14.27.54.342000 (1552631274342)
    Idx Commit Flag   : 1
    Idx Commit LSN    : 0x00000000bcc28300 (3166864128)
    Idx Commit Time   : 2019-03-15-14.27.54.343000 (1552631274343)
    Lob Commit Flag   : 0
    Lob Commit LSN    : 0xffffffffffffffff (-1)
    Lob Commit Time   : 1970-01-01-08.00.00.000000 (0)
    Extend option extent ID: 0xffffffff (-1)
    Create Timestamp  : 2019-03-15-14.27.54.343000 (1552631274343)
    Update Timestamp  : 2019-03-15-14.27.54.343000 (1552631274343)
    Deleted list :
      256  : 000000b8 0000ff90 (184, 65424)
      512  : 0000000a 0000ff74 (10, 65396)
       16K : 00000110 00004ab8 (272, 19128)
    Index extent:
       0 : 0x00000000 (0)
       1 : 0x00000002 (2)
   ```

   对于该集合空间中的每个集合，格式化其元数据块和数据块与索引块(由于数据量较大，部分内容以“……”省略)

   ```lang-text
    Dump Meta Extent for Collection [0]
    ExtentID: 0x00000000 (0)
   0x0000000000000000 : 4D45 0900 0000 0101 0000 0100 0100 0000   ME..............
   0x0000000000000010 : 0900 0000 1102 0000 FFFF FFFF FFFF FFFF   ................
   0x0000000000000020 : FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF   ................
   *
   0x0000000000080010 : 0000 0000 0000 0000 0000 0000 0000 0000   ................
   *
    Meta Extent Header :
       Eye Catcher  : ME
       Extent Size  : 9
       CollectionID : 0
       Flag         : 0x01 (InUse)
       Version      : 1
       Segment num  : 65536
       Used seg num : 1
    Segment extent info :
          0 : [0x00000009, 0x00000211]
    Used segment num: 1, has error: FALSE
   
   Dump Data for Collection [0]
    ExtentID: 0x00000009 (9)
   0x0000000000000000 : 4445 0100 0000 0101 0000 0000 FFFF FFFF   DE..............
   0x0000000000000010 : 0A00 0000 C100 0000 2400 0000 94FD 0000   ........$.......
   0x0000000000000020 : 1801 0000 004F 0100 2400 0000 FFFF FFFF   .....O..$.......
   0x0000000000000030 : 7401 0000 1D01 0000 075F 6964 005C 8B45   t........_id.\.E
   0x0000000000000040 : 8063 6827 1E68 4E33 AA10 6100 0000 0000   .ch'.hN3..a.....
   0x0000000000000050 : 1062 0000 0000 0002 6300 F200 0000 7364   .b......c.....sd
   0x0000000000000060 : 6761 7364 6773 6161 6173 6467 6767 6767   gasdgsaaasdggggg
   0x0000000000000070 : 6767 6767 6767 6767 6767 6767 6767 6767   gggggggggggggggg
   *
   0x0000000000000140 : 6767 6767 6767 6767 6767 6767 6767 3000   gggggggggggggg0.
   0x0000000000000150 : 0000 0000 0000 0000 0000 0000 0000 0000   ................
   *
   ……
   
    Data Extent Header:
       Eye Catcher  : DE
       Extent Size  : 1
       CollectionID : 0
       Flag         : 0x01 (InUse)
       Version      : 1
       Logic ID     : 13
       PrevExtent   : 0x00000015 (21)
       NextExtent   : 0x00000017 (23)
       Record Count : 192
       First Record : 0x00000024 (36)
       Last Record  : 0x0000fdd0 (64976)
       Free Space   : 220

    ExtentID: 0x00000017 (23)
   0x0000000000000000 : 4445 0100 0000 0101 0E00 0000 1600 0000   DE..............
   0x0000000000000010 : 1800 0000 C000 0000 2400 0000 D0FD 0000   ........$.......
   0x0000000000000020 : DC00 0000 0053 0100 2400 0000 FFFF FFFF   .....S..$.......
   0x0000000000000030 : 7801 0000 2001 0000 075F 6964 005C 8B45   x... ...._id.\.E
   0x0000000000000040 : 8263 6827 1E68 4E3E 2B10 6100 810A 0000   .ch'.hN>+.a.....
   0x0000000000000050 : 1062 0081 0A00 0002 6300 F500 0000 7364   .b......c.....sd
   0x0000000000000060 : 6761 7364 6773 6161 6173 6467 6767 6767   gasdgsaaasdggggg
   0x0000000000000070 : 6767 6767 6767 6767 6767 6767 6767 6767   gggggggggggggggg
   ……
   ```

* 指定索引文件目录 `/opt/sequoiadb/database/data/11830`，指定集合空间 sample 设定操作索引文件，指定操作为“dump”将索引数据页格式化并输出至 `indexdump.txt` 文件

   ```lang-bash
   $ sdbdmsdump -d /opt/sequoiadb/database/data/11830 -x /opt/sequoiadb/database/data/11830 -o indexdump.txt -c sample -a dump -i true
   ```

   生成结果文件 `indexdump.txt.<num>`，记录集合空间 sample 索引文件中 Header 段、SME 空间管理段、数据段存储结构信息，输出格式化信息和前一个示例中数据文件格式化信息类似，该集合空间每个集合的索引都有一个固定的索引控制块，通过该控制块可以知道索引的定义，输出内容如下：

   ```lang-text
   Dump Index Def for Collection [0]
       Index [ 0 ] : 0x00000000
   0x0000000000000000 : 4943 0000 0000 0101 0000 0000 0500 0000   IC..............
   0x0000000000000010 : FFFF FFFF 0100 0000 0000 0000 5200 0000   ............R...
   0x0000000000000020 : 075F 6964 005C 8B45 7973 CE93 6736 B8E3   ._id.\.Eys..g6..
   0x0000000000000030 : DB03 6B65 7900 0E00 0000 105F 6964 0001   ..key......_id..
   0x0000000000000040 : 0000 0000 026E 616D 6500 0400 0000 2469   .....name.....$i
   0x0000000000000050 : 6400 0875 6E69 7175 6500 0110 7600 0000   d..unique...v...
   0x0000000000000060 : 0000 0865 6E66 6F72 6365 6400 0100 0000   ...enforced.....
   0x0000000000000070 : 0000 0000 0000 0000 0000 0000 0000 0000   ................
   *
       Eye Catcher  : IC
       Index Flags  : 0 (Normal)
       CollectionID : 0
       Flag         : 0x01 (InUse)
       Version      : 1
     Logical ID     : 0
       Root Ext    : 0x00000005 (5)
   Scan extent LID : 0xffffffff (-1)
        Type       : 1 (Positive)
       Index Def   : { "_id": { "$oid": "5c8b457973ce936736b8e3db" }, "key": { "_id": 1 }, "name": "$id", "unique": true, "v": 0, "enforced": true }
   
       Index [ 1 ] : 0x00000002
   0x0000000000000000 : 4943 0000 0000 0101 0100 0000 0700 0000   IC..............
   0x0000000000000010 : FFFF FFFF 0100 0000 0000 0000 3F00 0000   ............?...
   0x0000000000000020 : 075F 6964 005C 8B45 7873 CE93 7236 B8E3   ._id.\.Exs..r6..
   0x0000000000000030 : E303 6B65 7900 0C00 0000 1061 0001 0000   ..key......a....
   0x0000000000000040 : 0000 026E 616D 6500 0700 0000 2473 6861   ...name.....$sha
   0x0000000000000050 : 7264 0010 7600 0000 0000 0000 0000 0000   rd..v...........
   0x0000000000000060 : 0000 0000 0000 0000 0000 0000 0000 0000   ................
   *
   ```

   索引页则类似如下（由于数据量较大，相似内容以“……”省略）：

   ```lang-text
    Index Dump for Collection [00], Index [00]
       Dump Index Page 5:
   0x0000000000000000 : 4945 3100 0000 0100 FFFF FFFF 82FD 5EFA   IE1...........^.
   0x0000000000000010 : 6000 0000 0100 0000 1300 0000 E494 0000   `...............
   0x0000000000000020 : F2FF 0000 0400 0000 1E00 0000 4C2D 0000   ............L-..
   0x0000000000000030 : E5FF 0000 0800 0000 2800 0000 B4C4 0000   ........(.......
   0x0000000000000040 : D8FF 0000 0A00 0000 3300 0000 1C5D 0000   ........3....]..
   0x0000000000000050 : CBFF 0000 0C00 0000 3D00 0000 84F4 0000   ........=.......
   0x0000000000000060 : BEFF 0000 0E00 0000 4800 0000 EC8C 0000   ........H.......
   0x0000000000000070 : B1FF 0000 1000 0000 5300 0000 5425 0000   ........S...T%..
   0x0000000000000080 : A4FF 0000 1200 0000 5D00 0000 BCBC 0000   ........].......
   0x0000000000000090 : 97FF 0000 1300 0000 6800 0000 2455 0000   ........h...$U..
   ……
   Index Extent Header:
       Eye Catcher  : IE
       Total Keys   : 49
       CollectionID : 0
       Flag         : 0x01 (InUse)
       Version      : 0
       Parent Ext   : 0xffffffff (-1) (root)
       Free Offset  : 0x0000fd82 (64898)
       Total Free   : 0x0000fa5e (64094)
       Right Child  : 0x00000060 (96)
   
       Dump Index Page 1:
   0x0000000000000000 : 4945 F107 0000 0100 0500 0000 C298 9E19   IE..............
   0x0000000000000010 : FFFF FFFF FFFF FFFF 0900 0000 2400 0000   ............$...
   0x0000000000000020 : F2FF 0000 FFFF FFFF 0900 0000 7401 0000   ............t...
   0x0000000000000030 : E5FF 0000 FFFF FFFF 0900 0000 C402 0000   ................
   ……
   ``` 

* 指定数据文件目录 `/opt/sequoiadb/database/data/11830`，指定集合空间 sample 设定操作大对象文件，指定操作为“dump”将大对象数据页格式化并输出至 `lobdump.txt` 文件

   ```lang-bash
   $ sdbdmsdump -d /opt/sequoiadb/database/data/11830 -o lobdump.txt -c sample -a dump -b true
   ```

   生成结果文件 `lobdump.txt.<num>`，记录集合空间 sample 大对象文件中 Header 段、SME 空间管理段、数据段存储结构信息，输出格式化信息和前一个示例中数据文件格式化信息类似，大对象元数据页类似如下（由于数据量较大，相似内容以“……”省略）：

   ```lang-text
    Dump Lob Page 1035:
   0x0000000000000000 : 0000 0000 0000 66E5 0859 3900 0465 4E7F   ......f..Y9..eN.
   0x0000000000000010 : 0000 0000 0000 0400 FFFF FFFF FFFF FFFF   ................
   0x0000000000000020 : 1A00 0000 0000 0001 0000 0000 0000 0000   ................
   0x0000000000000030 : 0000 0000 0000 0000 0000 0000 0000 0000   ................
   Lobm dmsLobDataMapBlk (1035):
    BucketID       : 4458816
    Oid            : 000066e50859390004654e7f
    Sequence       : 0
    Data Len       : 262144
    Prev PageId    : -1
    Next PageId    : -1
    CL LogicId     : 26
    MB Id          : 0
    Status         : DMS_LOB_PAGE_NORMAL (0)
    New Flag       : DMS_LOB_PAGE_NEW (1)
    CRC Flag       : DMS_LOB_PAGE_CRC_FULL (1)
    CRC            : 0x1a2b3c4d
   ```

   > **Note:**
   >
   > "CRC Flag" 表示该数据页的 CRC 状态：`DMS_LOB_PAGE_CRC_FULL` 表示整页写入并携带 CRC，`DMS_LOB_PAGE_CRC_NONE` 表示未携带 CRC（如老版本数据、非全写页）。仅当为 `DMS_LOB_PAGE_CRC_FULL` 时才输出 "CRC" 值。dump 操作仅展示 CRC 记录，不进行校验；如需校验请使用 `--action inspect --crccheck true`。

   如要该页为大对象的第0个分片，则会将大对象元数据进行输出，类似如下：

   ```lang-text
   0x0000000000000000 : D024 A810 0000 0000 4B2B F1EC 9101 0000   .$......K+......
   0x0000000000000010 : 0102 0000 FD42 F1EC 9101 0000 0000 0000   .....B..........
   0x0000000000000020 : 0000 0000 0000 0000 0000 0000 0000 0000   ................
   *
   Lobd Meta:
    Lob Len        : 279454928
    Create Time    : 2024-09-14-03.51.53.931000 (1726257113931)
    Status         : DMS_LOB_COMPLETE (1)
    Version        : DMS_LOB_META_CURRENT_VERSION (2)
    Mod Time       : 2024-09-14-03.51.59.997000 (1726257119997)
    Flag           : NO PIECESINFO (0)
    PiecesInfo Num : 0
   ``` 
