
***************************************��װ����ES����*******************************************
1����es�İ�װ������/optĿ¼�²���ѹ�����޸Ľ�ѹ��es������Ϊsdbadmin
1) tar -zxvf elasticsearch-6.2.2.tar.gz
2) chown -R  sdbadmin:sdbadmin_group elasticsearch-6.2.2

2����ÿ��������sas�̣�6��sas�̣���Ŀ¼�´���dataĿ¼��es�洢����,����������Ŀ¼�������޸�Ϊsdbadmin
1��mkdir /data/disk_sas1/data/ ..........
2)chown -R sdbadmin:sdbadmin_group /data/disk_sas1/data/

3���޸���̨�����е�ES��װ�������elasticsearch.yml�����ļ�
1��vi elasticsearch-6.2.2/config/elasticsearch.yml
   ��1.1��cluster.name: FullTextSearch
    (1.2)node.name: node1
   ��1.3��path.data: /data/disk_sas1/data/,/data/disk_sas2/data/,/data/disk_sas3/data/,/data/disk_sas4/data/,/data/disk_sas5/data/,/data/disk_sas6/data/
    (1.4)network.host: 192.168.30.53
   ��1.5)http.port: 9200
    (1.6)discovery.zen.ping.unicast.hosts: ["192.168.30.53", "192.168.30.54", "192.168.30.55"]
    (1.7)discovery.zen.minimum_master_nodes: 2
4���޸���̨�����е�ES��װ�������jvm.options�����ļ�
1��vi elasticsearch-6.2.2/config/jvm.options
  ��1.1����es���ڴ��޸�Ϊ31g

**************************************��װ����SDB����*******************************************
1�������°汾��db release��װ������/optĿ¼�²��޸�����Ϊ��ִ��,ִ�а�װ
2�����ű�createSixGroupThreeNode.js���Ƶ�����һ̨�����ϣ�Ȼ��ִ�нű��db���������ڵ㻷��
3���޸�sequoiadb/conf/limits.conf�ļ���core_file_size��ֵ�޸�Ϊ-1��Ȼ����������
4�����ű�buildAdapt.sh��30_53_sdbseadapter��30_54_sdbseadapter��30_55_sdbseadapter���Ƶ�����һ̨�������棬ִ�нű�buildAdapt.sh��������������
5��ʹ��sdbadmin�û�����es

**************************************���Թ���Jmeter����������װ***********************************************
1����apache��������apache-jmeter��װ������ѹ��/optĿ¼��
2����jmeter������ع������������������StartAgentѹ�����������������jmeter��extĿ¼�£�Ȼ��StartAgentѹ������ѹ����̨�������ϣ�ִ��./StartAgent.sh��������ִ��jmeter����������Ӧ��java requestʱ���Զ���ط���������Ӧ���

**************************************����ִ�й���****************************************************
1����������test.test,�������Զ��з֣����Զ��зֵ��������ϣ�������ȫ������
1��db.createCS("test").createCL("test",{ShardingKey:{"_id":1},ShardingType:"hash",AutoSplit:true,AutoIncrement:{Field:"autoIncrementField"}})
2��db.test.test.createIndex("fullTextIndexName",{inst_id:"text"},false)
2��ִ��builddata Project�е�Build.java�򼯺��й�����
3�����ػ�ִ������jmeter -n -t example.jmx�ܲ��Խű�