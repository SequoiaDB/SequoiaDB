
从 HDFS 文件中导入数据到 SequoiaDB
```lang-javascript
hive> insert overwrite table sdb_tab select * from hdfs_tab;

Total MapReduce jobs = 1
Launching Job 1 out of 1
Number of reduce tasks is set to 0 since there's no reduce operator
Starting Job = job_201310172156_0010, Tracking URL = http://bl465-5:50030/jobdetails.jsp?jobid=job_201310172156_0010
Kill Command = /opt/hadoop-hive/hadoop-1.2.1/libexec/../bin/hadoop job  -kill job_201310172156_0010
Hadoop job information for Stage-0: number of mappers: 1; number of reducers: 0
2013-10-18 04:44:47,733 Stage-0 map = 0%,  reduce = 0%
2013-10-18 04:44:49,763 Stage-0 map = 100%,  reduce = 0%, Cumulative CPU 1.85 sec
2013-10-18 04:44:50,777 Stage-0 map = 100%,  reduce = 0%, Cumulative CPU 1.85 sec
2013-10-18 04:44:51,795 Stage-0 map = 100%,  reduce = 100%, Cumulative CPU 1.85 sec
MapReduce Total cumulative CPU time: 1 seconds 850 msec
Ended Job = job_201310172156_0010
10 Rows loaded to sdb_tab
MapReduce Jobs Launched:
Job 0: Map: 1   Cumulative CPU: 1.85 sec   HDFS Read: 2301 HDFS Write: 0 SUCCESS
Total MapReduce CPU Time Spent: 1 seconds 850 msec
OK
Time taken: 12.201 seconds
```

> **Note:**
>
> 用户在导入数据到 SequoiaDB 之前，需要确保 SequoiaDB 已经创建基于 HDFS 文件的 hdfs_tab 集合，并 load 数据。
