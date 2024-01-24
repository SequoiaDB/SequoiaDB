
本文档按照问题分类，对 SequoiaDB 常见问题的现象、错误码、诊断方案以及修复方法进行描述。

##系统配置类问题##

1. SDB_OOM(-2)
   * 系统内存分配失败
   * 问题诊断：出现该问题是由于系统虚拟内存分配已达到上限而触发，切换至相应的用户，可以通过 *"ulimit -Sa"* 进行查看，确认 *"virtual memory"* 的大小是否跟所有数据文件（包括索引文件、大对象文件）相接近，如果是，则需要修改该值。
   * 问题修复：建议将相应用户的 *"virtual memory"* 设置为 *"unlimited"*。

2. SDB_DPS_FILE_SIZE_NOT_SAME(-123)、SDB_DPS_FILE_NOT_RECOGNISE(-124)
   * 当前节点的同步日志大小/个数与配置文件中的大小/个数不相符，目前SequoiaDB不支持在初始化后更改同步日志文件大小和个数
   * 问题修复：调整到原配置即可。若需要强行更改同步日志文件大小和个数，则首先需要确保同一复制组内的所有节点同步日志已经一致（可以通过直连节点并执行 *"[db.snapshot( SDB_SNAP_SYSTEM )][snapsystem]"* 查看 "CompleteLSN"是否相同），然后停止复制组内的所有节点，并删除每一个节点上的同步日志目录（默认为 \<dbpath\>/replicalog），再修改到新的配置并重启节点即可。

##网络问题##

1. SDB_NETWORK(-15)、SDB_NETWORK_CLOSE(-16)
   * 通信套节字关闭
   * 问题诊断：
      * 请检查是否配置防火墙策略；
      * 请检查交换机是否配置安全策略，是否故障；
      * 请检查机器网卡是否故障；
      * 可以用 *"ping \<hostname\>"* 或 *"telnet \<hostname:port\>"* 进行相关的测试；
      * 请检查节点或客户端是否重启或关闭。

2. SDB_NET_CANNOT_LISTEN(-78)
   * 通信监听端口冲突
   * 问题诊断：查看节点的诊断日志，找到冲突的端口，并通过 *"netstat -anp|grep \<port\>"* 确认端口是否被占用。
   * 问题修复：若端口被占用，则需要先停止占用该端口的进程；重启该节点。

3. SDB_NET_CANNOT_CONNECT(-79)
   * 无法连接指定的地址
   * 问题诊断：
      * 通过查看节点的诊断日志，找到目的端的地址和端口信息；
      * 检查目的节点是否启动；
      * 检查当前节点的 "host" 配置是否正确；
      * 检查当前节点和目的节点是否开启防火墙；
   * 问题修复：
      * 请根据上述每一步的检查进行相应的修复，并重试操作。

4. SDB_NET_BROKEN_MSG(-84)
   * 消息格式错误或长度不正确，当前消息包最大长度为512MB

5. SDB_COORD_REMOTE_DISC(-134)
   * 对端节点断开连接
   * 问题诊断：
      * 请查看该协调节点的诊断日志，找到发生错误的数据节点；检查该数据节点是否异常重启；
      * 在开启 "optimeout" 配置的情况下，若对端节点在指定时间内无心跳响应，则操作也会被中断，并返回 SDB_COORD_REMOTE_DISC 错误，在这种情况下，请检查机器是否负载过高，磁盘IO过慢等。
   * 问题修复：
      * 若数据节点发生重启，则需要联系售后工程师进行处理。
      * 若由于机器负载过高，磁盘压力大引起，则需要关闭或增大 "optimeout" 。

6. SDB_TOO_MANY_OPEN_FD(-255)
   * 连接句柄数达到上限
   * 问题诊断：出现该问题是由于进程的句柄数达到配置的上限，切换至相应的用户，可以通过 *"ulimit -Sa"* 进行查看，确认 *"open files"* 的配置是否小于当前节点所有文件数加连接数，如果是，则需要修改该值。
   * 问题修复：建议将相应用户的 *"open files"* 设置为 *"unlimited"*。

##节点可靠性问题##

1. SDB_APP_FORCED(-18)
   * 节点被强制退出或停止
   * 问题诊断：根据节点的诊断日志，查看节点退出原因。
   * 问题修复：重启对应节点。

2. SDB_DMS_INVALID_SU(-35)
   * 节点上数据文件、索引文件或大对象文件损坏
   * 问题诊断：
      * 请检查磁盘是否发生故障；
      * 请检查节点是否发生异常，或机器是否发生异常重启；
   * 问题修复：
      * 若磁盘故障，则需要更换磁盘；
      * 若由于节点异常或机器重启导致，则检查故障文件大小是否小于64KB，小于则直接清除（包含该集合空间的其它文件一并清除），并重启节点; 否则需要联系售后工程师进行恢复。

3. SDB_DPS_CORRUPTED_LOG(-98)
   * 同步日志记录损坏
   * 问题诊断：请检查磁盘是否损坏
   * 问题修复：系统会尝试自动修复该错误，误长时间都不能修复，则联系售后工程师进行处理。

##数据可靠性问题##

1. SDB_DMS_CORRUPTED_SME(-115)、SDB_DMS_CORRUPTED_EXTENT(-139)
   * 数据文件、索引文件或大对象文件的损坏
   * 问题诊断：请检查磁盘是否发生故障，机器是否强行重启，节点是否异常。
   * 问题修复：发生该错误系统会尝试自动修复，若不能自动修复则需要联系售后工程师进行恢复。

2. SDB_CLS_FULL_SYNC(-129)
   * 节点正在进行数据全量同步
   * 问题诊断：引发全量同步的原因有：1）当前节点的同步的日志在其它节点中已被写脏；2）当前节点异常重启；3）发生主切换，新的主节点回滚失败（对于删除集合，删除集合空间无法进行回滚）。
   * 问题修复：系统会自动修复，在全量同步完成后重试操作。

3. SDB_RTN_IN_REBUILD(-150)
   * 节点正在进行本地数据恢复
   * 问题诊断：引发本地数据恢复的原因是该复制组内所有节点都异常重启。
   * 问题修复：系统会自动修复，在恢复完成后重试操作，若不能自动修复则需要联系售后工程师进行恢复。

##功能问题##

1. SDB_FNE(-4)
   * 指定文件不存在
   * 问题诊断：请检查操作相关的"文件"、"大对象"等是否存在。

2. SDB_FE(-5)
   * 指定文件已存在
   * 问题诊断：请检查操作相关的"文件"、"大对象"等是否存在。
   * 问题修复：
      * "节点启动"或"创建集合空间"发生该错误时，是由于节点异常导致的文件残留，可以对异常的文件进行清理，并重试操作。
      * "创建节点"发生该错误时，可以对已残留的节点配置文件进行清理，并重试操作。
      * "获取大对象"发生该错误时，可以更改本地要存储的文件名，并重试操作。

3. SDB_INVALIDARG(-6)
   * 指定参数格式或值不正确
   * 问题诊断：请查看对应节点的诊断日志，找到该参数错误的详细描述，并加以修正重试。

4. SDB_INTERRUPT(-8)
   * 发生系统中断，异致节点退出或操作终止
   * 问题诊断：“节点”发生中断退出，即为收到相应信号量（比如SIGTERM等），即外部对该节点执行"kill -15"、"sdbstop"、"service sdbcm stop"或"操作系统重启"等相应操作，节点正常停止。

5. SDB_NOSPC(-11)
   * 磁盘空间不足
   * 问题诊断：检查节点对应的数据目录、索引目录、大对象目录等的磁盘空间是否达到上限。

6. SDB_TIMEOUT(-13)
   * 超时错误
   * 问题诊断：
      * 并发事务执行产生死锁
      * 会话设置操作超时，可以通过 [Sdb.getSessionAttr\(\)][getSessionAttr] 检查 Timeout 选项是否被设置。
   * 问题修复：
      * 对产生死锁的事务进行回滚操作，可参考 [Sdb.transRollback\(\)][transRollback]。
      * 对会话设置操作超时，请参考 [Sdb.setSessionAttr\(\)][setSessionAttr]，设置 Timeout 为合理的值。

7. SDB_DMS_NOSPC(-21)、SDB_IXM_NOSPC(-40)
   * 集合空间剩余空间不足
   * 问题诊断：检查当个集合空间对应文件是否达到容量上限，请参考[集合空间限制][sequoiadb_limitation]。

8. SDB_DMS_EXIST(-22)
   * 集合已存在
   * 问题诊断：检查操作中的集合名是否拼写错误。

9. SDB_DMS_NOTEXIST(-23)
   * 集合不存在
   * 问题诊断：
      * 检查操作中的集合名是否拼写错误；
      * 使用 *"[db.listCollections()][listCollections]"* 确认该集合是否存在，若该集合存在，而其它操作依然报该错误，原因可能为：1）操作位于备节点，而备节点还未同步；2）创建该集合由于节点故障导致在实际节点上不存在。
   * 问题修复：设置会话访问"主节点"的属性 *"[db.setSessionAttr({PreferredInstanace:"M"})][setSessionAttr]"* ，并执行 *"[db.\<cs\>.\<cl\>.find()][find]"* 操作进行自动重建修复。（如需要修复表分区中的主表报错 -23，需要为每个子表执行重建修复。）

10. SDB_DMS_RECORD_TOO_BIG(-24)
   * 记录超过最大限制
   * 问题诊断：请检查操作的记录大小是否超过[记录限制][sequoiadb_limitation]。

11. SDB_DMS_CS_EXIST(-33)
   * 集合空间已存在
   * 问题诊断：请检查操作中的集合空间名是否拼写错误。

12. SDB_DMS_CS_NOTEXIST(-34)
   * 集合空间不存在
   * 问题诊断：
      * 请检查集合空间名是否拼写错误；
      * 使用 *"db.listCollectionSpaces()"* 确认该集合空间是否存在，若该集合空间存在，而其它操作仍然报该错误，原因可能为：1）操作位于备节点，而备节点还未同步；2）创建该集合空间空由于节点故障导致在实际节点上不存在。
   * 问题修复：设置会话访问"主节点"的属性 *"[db.setSessionAttr({PreferredInstance:"M"})][setSessionAttr]"*，并重试该操作。

13. SDB_IXM_MULTIPLE_ARRAY(-37)
   * 复合索引字段中数组类型过多，目前复合索引只允许1个字段为数组类型

14. SDB_IXM_DUP_KEY(-38)
   * 与该记录相同的唯一索引值冲突，对于集合默认有 *"$id"* 的唯一索引

15. SDB_IXM_KEY_TOO_LARGE(-39)
   * 通过记录生成的索引值大小超过1000字节
   * 问题修复：请检查索引是否合理，建议需要建索引的字段值长度应小于128字节，从而可以达到提升性能的效果。

16. SDB_DMS_MAX_INDEX(-42)
   * 集合的索引达到上限，单集合最大支持创建64个索引

17. SDB_DMS_INIT_INDEX(-43)
   * 初始化索引页失败
   * 问题诊断：
      * 该索引在操作过程中被删除；
      * 磁盘发生故障；
   * 问题修复：重试操作，若故障未修复，则需要联系售后工程师进行修复。

18. SDB_IXM_EXIST(-46)
   * 相同的索引名已存在
   * 问题诊断：请检查操作中的索引名是否拼写错误。

19. SDB_IXM_NOTEXIST(-47)、SDB_RTN_INDEX_NOTEXIST(-52)
   * 指定索引不存在
   * 问题诊断：该索引在操作过程中被删除；
   * 问题修复：重试操作，若故障未修复，则需要联系售后工程师进行修复。

20. SDB_DMS_SU_OUTRANGE(-55)
   * 单节点集合空间达到上限，单节点最多支持16384个集合空间

21. SDB_IXM_DROP_ID(-56)、SDB_IXM_DROP_SHARD(-164)
   * 系统索引不允许删除（包括 "$id"和"$shard"）
   * 问题修复：若要删除 "$id" 索引，请使用 *"[db.\<cs\>.\<cl\>.dropIdIndex()][dropIdIndex]"* 接口，但删除后该集合不支持"更新"和"删除"。

22. SDB_PMD_RG_NOT_EXIST(-59)、SDB_COOR_NO_NODEGROUP_INFO(-138)、SDB_CLS_GRP_NOT_EXIST(-154)、SDB_CLS_NO_GROUP_INFO(-167)
   * 复制组不存在
   * 问题诊断：请使用 *"[db.listReplicaGroups()][listReplicaGroups]"* 检查复制组是否存在。
   * 问题修复：若上述检查复制组存在，请使用 *"[db.invalidateCache({Global:true})][invalidateCache]"* 清空所有节点的缓存，并重试操作。

23. SDB_PMD_RG_EXIST(-60)、SDB_CAT_GRP_EXIST(-153)
   * 复制组已存在
   * 问题诊断：请检查操作中的复制组名是否拼写错误。

24. SDB_PMD_SESSION_NOT_EXIST(-62)
   * 指定会话不存在
   * 问题诊断：可以通过直连该节点，并执行 *"[db.snapshot( SDB_SNAP_SESSIONS )][SDB_SNAP_SESSIONS]"* 确认该会话是否存在。

25. SDB_PMD_FORCE_SYSTEM_EDU(-63)
   * 系统会话不允许被强制结束

26. SDB_BACKUP_HAS_ALREADY_START(-67)
   * 其它备份任务正在执行, 当前系统只允许同时执行一个离线备份任务

27. SDB_BAR_DAMAGED_BK_FILE(-70)
   * 备份文件损坏
   * 问题诊断：请检查磁盘是否损坏。
   * 问题修复：重新执行备份，并删除该备份文件。

28. SDB_RTN_NO_PRIMARY_FOUND(-71)、SDB_CLS_NOT_PRIMARY(-104)
   * 复制组不存在主节点
   * 问题诊断：
      * 检查复制组的所有节点是否都已经启动（1、在复制组所有节点都异常后，需要所有节点都启动才能选举；2、在复制组节点正常重启时，若存在节点未启动，则其它节点需要等待一定周期才能开始选举，默认时间是10分钟；3、在复制组节点正常重启时，需要有 N/2 +1 个节点成功启动才会选举）。
      * 直连复制组的每一个节点，执行 *"[db.snapshot( SDB_SNAP_SYSTEM )][SDB_SNAP_SYSTEM]"* 并查看 "IsPrimary" 是否为 "true"；
      * 检查复制组的每一个节点的诊断日志，查看节点当前的错误信息。
   * 问题修复：
      * 若检查当前复制组存在 "IsPrimary" 为 "true" 的节点，则执行 *"[db.invalidateCache({Global:true})][invalidateCache]"* 清除所有缓存并重试；
      * 若存在节点未启，请启动节点。

29. SDB_REPL_GROUP_NOT_ACTIVE(-90)
   * 复制组未激活，不能被分配给域、集合空间和集合
   * 问题修复：请执行 *"[db.getRG(\<name\>).start()][start]"* 对该复制组进行激活操作。

30. SDB_DMS_INCOMPATIBLE_MODE(-92)
   * 集合当前状态和操作不兼容
   * 问题诊断：
      * 执行 *"[db.snapshot( SDB_SNAP_COLLECTIONS )][SDB_SNAP_COLLECTIONS]"* 查看对应集合的 "Status" 状态，或通过执行 *"[sdbdmsdump -d \<dbpath\> -o \<output_file\> -c \<cs\> -l \<cl\> -a dump --meta true][sdbdmsdump]"* 查看对应集合的 "Status"；
      * 若集合状态为 "OFFLINE REORG"，则不允许从外部对该集合进行任何操作；
   * 问题修复：
      * 出现上述现象，系统会自行重组修复；若无法自动修复，则可以通过执行 *"[sdbdmsdump -d \<dbpath\> -o \<output_file\> -c \<cs\> -l \<cl\> -r mb:Flag=0][sdbdmsdump]"* 进行强制修复。

31. SDB_DMS_INCOMPATIBLE_VERSION(-93)
   * SequoiaDB程序与当前的数据文件版本不兼容，当前不支持从高版本回退到低版本
   * 问题修复：请更新到正确的版本。

32. SDB_CLS_NODE_NOT_ENOUGH(-105)
   * 复制组当前激活节点数不满足集合同步写幅本数要求
   * 问题诊断：请检查该复制组内所有节点是否启动。
   * 问题修复：启动该复制组内未启动的节点，或者通过 *"[db.\<cs\>.\<cl\>.alter()][alter]"* 降低集合同步写幅本数。

33. SDB_CLS_DATA_NODE_CAT_VER_OLD(-107)、SDB_CLS_COORD_NODE_CAT_VER_OLD(-108)
   * 数据节点/协调节点编目信息过旧
   * 问题修复：该错误系统会自动修复，如未能自动修复，可以通过执行 *"[db.invalidateCache({Global:true})][invalidateCache]"* 清空所有节点缓存并重试。

34. SDB_APP_INTERRUPT(-116)
   * 当前操作被中断
   * 问题诊断：请检查 节点是否正在停止，通讯是否中断，机器是否重启，以及是否有其它人员对该会话进行"强制中断"。
   * 问题修复: 重试操作。

35. SDB_CAT_AUTH_FAILED(-128)
   * 该节点在编目中未配置，鉴权失败
   * 问题诊断：请检查是否更改节点的主机名或服务名。
   * 问题修复：暂时不允许修复节点主机名，请改回原主机名即可。

36. SDB_CAT_NO_NODEGROUP_INFO(-133)
   * 没有可用复制组
   * 问题诊断：请检查是否创建复制组并激活。

37. SDB_CAT_NO_MATCH_CATALOG(-135)
   * 不能匹配到有效的分区信息
   * 问题诊断：可以通过执行 *"[db.snapshot( SDB_SNAP_CATALOG )][SDB_SNAP_CATALOG]"* 查看对应集合的分区信息，并与操作的记录进行比较，确保记录能够匹配到已有的分区信息。
   * 问题修复：通过 *"[db.\<cs\>.\<cl\>.attachCL()][attachCL]"*持载记录对应的分区即可。

38. SDBCM_FAIL(-140)
   * 远程节点操作失败
   * 问题诊断：请检查 "sdbcm" 是否启动。
   * 问题修复: 启动 "sdbcm"。

39. SDBCM_NODE_EXISTED(-145)
   * 指定节点已存在
   * 问题诊断：可以通过执行 *"[db.listReplicaGroups()][listReplicaGroups]"* 查看节点是否存在。
   * 问题修复：若检查节点不存在，则手动停止对应机器的 "sdbcm"，并删除 "conf/local/" 下该节点的目录，然后重启 "sdbcm" 并重试操作。

40. SDBCM_NODE_NOTEXISTED(-146)、SDB_CLS_NODE_NOT_EXIST(-155)
   * 指定节点不存在
   * 问题诊断： 可以通过执行 *"[db.listReplicaGroups()][listReplicaGroups]"* 查看节点是否存在。

41. SDB_LOCK_FAILED(-147)
   * 加锁失败
   * 问题诊断：当删除集合空间出现该错误时，是由于还有其它在该集合空间上的查询或大对象游标未关闭导致，可以查看诊断日志，找到出错的节点，并直连该节点，执行 *"[db.snapshot( SDB_SNAP_CONTEXTS)][SDB_SNAP_CONTEXTS]"* 找到对应的游标和会话。
   * 问题修复：直连该节点，并且可以通过执行 *"[db.forceSession(\<sessionID\>)][forceSession]"* 强制终止该会话，并重试操作。

42. SDB_COLLECTION_NOTSHARD(-169)
   * 该集合为非分区集合
   * 问题修复：可以通过 *"[db.\<cs\>.\<cl\>.alter()][alter]"* 将该集合改为分区集合。

43. SDB_CL_NOT_EXIST_ON_GROUP(-172)
   * 集合的指定分区不存在于指定复制组上
   * 问题诊断：通过 *"[db.snapshot( SDB_SNAP_CATALOG )][SDB_SNAP_CATALOG]"* 查看指定集合的分区信息，确认分区信息是否正确。

44. SDB_MULTI_SHARDING_KEY(-174)
   * 分区键含有数组，且该数组中有多个值，目前分区键中若含有数组类型，需要保证数组中只有1个元素

45. SDB_CLS_BAD_SPLIT_KEY(-176)
   * 分区范围已经在目标复制组
   * 问题诊断：通过 *"[db.snapshot( SDB_SNAP_CATALOG )][SDB_SNAP_CATALOG]"* 查看指定集合的分区信息，确认分区信息是否正确。

46. SDB_DPS_TRANS_DOING_ROLLBACK(-191)
   * 节点正在执行事务回滚操作
   * 问题修复：在回滚操作完成后进行重试。

47. SDB_QGM_AMBIGUOUS_FIELD(-194)
   * 选择字段名存在冲突
   * 问题修复：请对选择字段名加上来源别名。

48. SDB_DMS_INVALID_INDEXCB(-199)
   * 该索引被删除
   * 问题诊断：通过执行 *"[db.\<cs\>.\<cl\>.listIndexes()][listIndexes]"* 确认该索引是否存在。

49. SDB_DPS_LOG_FILE_OUT_OF_SIZE(-203)
   * 事务日志空间不足
   * 问题修复：可以通过修复"日志文件个数"或"日志文件大小"增大日志空间，默认日志空间为1.2GB。

50. SDB_CATA_RM_NODE_FORBIDDEN(-204)
   * 不允许删除复制组内的主节点或最后一个节点
   * 问题修复：可以使用 *"[db.removeRG(\<name\>)][removeRG]"* 接口删除整个复制组。

51. SDB_CAT_RM_GRP_FORBIDDEN(-208)
   * 不允许删除非空分区数
   * 问题诊断：执行 *"[db.snapshot( SDB_SNAP_CATALOG )][SDB_SNAP_CATALOG]"* 检查各集合中的 "CataInfo" 是否包含待删除的复制组。
   * 问题修复：需要确保待删除的复制组中不存在集合，可以删除对应的集合，或将该集合切分至其它复制组。

52. SDB_CAT_DOMAIN_NOT_EXIST(-215)、SDB_CAT_DOMAIN_EXIST(-216)
   * 指定域不存在/已存在
   * 问题诊断：执行 *"[db.listDomains()][listDomains]"* 检查指定域是否存在或不存在。

53. SDB_CAT_GROUP_NOT_IN_DOMAIN(-216)
   * 指定切分的复制组不在集合空间所属域内
   * 问题修复：当集合空间指定域后，其切分的复制组也必须在域内；可以通过 *"[domain.alter()][alter]"* 将该分组内加入域，或更改切分的复制组。

54. SDB_INVALID_MAIN_CL_TYPE(-244)
   * 表分区集合分区类型不正确，表分区集合必须为范围分区

55. SDB_DMS_REACHED_MAX_NODES(-249)
   * 复制组节点达到上限，复制组最多支持7个幅本

56. SDB_CLS_WAIT_SYNC_FAILED(-252)
   * 操作等待备节点同步失败
   * 问题诊断：出象该故障，为该复制组内在操作过程中出现象节点心跳中断或节点故障所致，请检查该复制组内每个节点是否正常，或发生过异常重启。

57. SDB_DPS_TRANS_DIABLED(-253)
   * 事务未开启
   * 问题修复：修改节点的配置文件，开启事务功能。

##用户权限问题##

1. SDB_PERM(-3)
   * 无相应的访问权限
   * 问题诊断：出现该问题，请查看对应节点的诊断日志，根据日志中的错误信息找到对应的"文件"或"目录"，并确认相应用户对该"文件"或"目录"具备读写等权限。
   * 问题修复：给相应的"文件"或"目录"赋予该用户相应的权限。

2. SDB_AUTH_AUTHORITY_FORBIDDEN(-179)
   * 数据库鉴权失败
   * 问题修复：请使用正确的用户名和密码。若要取消数据库鉴权，则删除所有用户即可。

3. SDB_AUTH_USER_NOT_EXIST(-300)
   * 用户名或密码不正确

[snapsystem]:manual/Manual/Snapshot/SDB_SNAP_SYSTEM.md
[transRollback]: manual/Manual/Sequoiadb_Command/Sdb/transRollback.md
[setSessionAttr]:manual/Manual/Sequoiadb_Command/Sdb/setSessionAttr.md
[sequoiadb_limitation]:manual/Manual/sequoiadb_limitation.md#集合空间
[listCollections]:manual/Manual/Sequoiadb_Command/Sdb/listCollections.md
[setSessionAttr]:manual/Manual/Sequoiadb_Command/Sdb/setSessionAttr.md
[find]:manual/Manual/Sequoiadb_Command/SdbCollection/find.md
[dropIndex]:manual/Manual/Sequoiadb_Command/SdbCollection/dropIdIndex.md
[listReplicaGroups]:manual/Manual/Sequoiadb_Command/Sdb/listReplicaGroups.md
[invalidateCache]:manual/Manual/Sequoiadb_Command/Sdb/invalidateCache.md
[SDB_SNAP_SESSIONS]:manual/Manual/Snapshot/SDB_SNAP_SESSIONS.md
[sdbdmsdump]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/dmsdump.md
[alter]:manual/Manual/Sequoiadb_Command/SdbCollection/alter.md
[invalidateCache]:manual/Manual/Sequoiadb_Command/Sdb/invalidateCache.md
[SDB_SNAP_CATALOG]:manual/Manual/Snapshot/SDB_SNAP_CATALOG.md
[attachCL]:manual/Manual/Sequoiadb_Command/SdbCollection/attachCL.md
[listReplicaGroups]:manual/Manual/Sequoiadb_Command/Sdb/listReplicaGroups.md
[SDB_SNAP_CONTEXTS]:manual/Manual/Snapshot/SDB_SNAP_CONTEXTS.md
[forceSession]:manual/Manual/Sequoiadb_Command/Sdb/forceSession.md
[listIndexes]:manual/Manual/Sequoiadb_Command/SdbCollection/listIndexes.md
[removeRG]:manual/Manual/Sequoiadb_Command/Sdb/removeRG.md
[listDomains]:manual/Manual/Sequoiadb_Command/Sdb/listDomains.md
[SDB_SNAP_SYSTEM]:manual/Manual/Snapshot/SDB_SNAP_SYSTEM.md
[getSessionAttr]:manual/Manual/Sequoiadb_Command/Sdb/getSessionAttr.md
[SDB_SNAP_COLLECTIONS]:manual/Manual/Snapshot/SDB_SNAP_COLLECTIONS.md
[start]:manual/Manual/Sequoiadb_Command/SdbReplicaGroup/start.md
