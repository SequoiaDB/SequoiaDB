
***************************************安装部署ES环境*******************************************
1、将es的安装包放在/opt目录下并解压，并修改解压后es的属性为sdbadmin
1) tar -zxvf elasticsearch-6.2.2.tar.gz
2) chown -R  sdbadmin:sdbadmin_group elasticsearch-6.2.2

2、在每个机器的sas盘（6个sas盘）的目录下创建data目录给es存储数据,并将创建的目录的属性修改为sdbadmin
1）mkdir /data/disk_sas1/data/ ..........
2)chown -R sdbadmin:sdbadmin_group /data/disk_sas1/data/

3、修改三台机器中的ES安装包里面的elasticsearch.yml配置文件
1）vi elasticsearch-6.2.2/config/elasticsearch.yml
   （1.1）cluster.name: FullTextSearch
    (1.2)node.name: node1
   （1.3）path.data: /data/disk_sas1/data/,/data/disk_sas2/data/,/data/disk_sas3/data/,/data/disk_sas4/data/,/data/disk_sas5/data/,/data/disk_sas6/data/
    (1.4)network.host: 192.168.30.53
   （1.5)http.port: 9200
    (1.6)discovery.zen.ping.unicast.hosts: ["192.168.30.53", "192.168.30.54", "192.168.30.55"]
    (1.7)discovery.zen.minimum_master_nodes: 2
4、修改三台机器中的ES安装包里面的jvm.options配置文件
1）vi elasticsearch-6.2.2/config/jvm.options
  （1.1）将es的内存修改为31g

**************************************安装部署SDB环境*******************************************
1、将最新版本的db release安装包放在/opt目录下并修改属性为可执行,执行安装
2、将脚本createSixGroupThreeNode.js复制到其中一台机器上，然后执行脚本搭建db的六组三节点环境
3、修改sequoiadb/conf/limits.conf文件将core_file_size的值修改为-1，然后重启服务
4、将脚本buildAdapt.sh和30_53_sdbseadapter、30_54_sdbseadapter、30_55_sdbseadapter复制到其中一台机器上面，执行脚本buildAdapt.sh部署启动适配器
5、使用sdbadmin用户启动es

**************************************测试工具Jmeter部署与插件安装***********************************************
1、在apache官网下载apache-jmeter安装包，解压在/opt目录下
2、在jmeter插件下载官网下载两个插件还有StartAgent压缩包包，将插件放在jmeter的ext目录下，然后将StartAgent压缩包解压到三台服务器上，执行./StartAgent.sh启动，当执行jmeter命令运行相应的java request时会自动监控服务器的相应情况

**************************************测试执行过程****************************************************
1、创建集合test.test,并设置自动切分（会自动切分到六个组上），创建全文索引
1）db.createCS("test").createCL("test",{ShardingKey:{"_id":1},ShardingType:"hash",AutoSplit:true,AutoIncrement:{Field:"autoIncrementField"}})
2）db.test.test.createIndex("fullTextIndexName",{inst_id:"text"},false)
2、执行builddata Project中的Build.java向集合中灌数据
3、负载机执行命令jmeter -n -t example.jmx跑测试脚本