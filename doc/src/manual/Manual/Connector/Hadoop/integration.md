本文档主要介绍 Hadoop 的环境搭建和操作示例。

搭建Hadoop环境
----
SequoiaDB 巨杉数据库支持 hadoop 1.x 和 hadoop 2.x 版本，用户需要自行安装并配置 Hadoop。

配置连接环境
----
Hadoop、SequoiaDB 与 MapReduce 对接，需要准备 `hadoop-connector.jar` 和 `sequoiadb.jar`，用户可以在 SequoiaDB 安装目录下面的 `hadoop` 目录（默认为 `/opt/sequoiadb/hadoop`）中找到这两个 jar 包，运行以下命令查看 hadoop 的 classpath：

```lang-bash
$ hadoop classpath
```

用户需要在 classpath 中选择一个目录，把 `hadoop-connector.jar` 和 `sequoiadb.jar` 放入所选目录后重启 hadoop 集群。

编写 MapReduce
----

**hadoop-connector.jar 中一些重要的类：**

- SequoiadbInputFormat：读取 SequoiaDB 的数据

- SequoiadbOutputFormat：向 SequoiaDB 写入数据

- BSONWritable：BSONObject 的包装类，实现了 WritableComparable 接口，用于序列化 BSONObject 对象

**SequoiaDB 和 MapReduce 的配置：**

sequoiadb-hadoop.xml 是配置文件，放于用户编写的 MapReduce 工程源码根目录下。

- sequoiadb.input.url：指定作为输入源 SequoiaDB 的 URL 路径，格式为：hostname1:port1,hostname2:port2

- sequoiadb.input.user：指定输入源 SequoiaDB 的用户名，默认为 null

- sequoiadb.input.passwd：指定输入源 SequoiaDB 用户密码，默认为 null

- sequoiadb.in.collectionspace：指定作为输入源 SequoiaDB 集合空间

- sequoiadb.in.collection：指定作为输入源 SequoiaDB 集合

- sequoiadb.query.json：指定输入源 SequoiaDB 的查询条件，使用 Json 结构，默认为 null

- sequoiadb.selector.json：指定输入源 SequoiaDB 的字段筛选，使用 Json 结构，默认为 null

- sequoiadb.preferedinstance：指定从输入源 SequoiaDB 中获取数据时，连接哪个数据节点，默认为 anyone，可填值为 slave、master、anyone 和 node1~7

- sequoiadb.output.url：指定作为输出源 SequoiaDB 的 URL 路径

- sequoiadb.output.user：指定输出源 SequoiaDB 用户名，默认为 null

- sequoiadb.output.passwd：指定输出源 SequoiaDB 用户密码，默认为 null

- sequoiadb.out.collectionspace：指定作为输出源 SequoiaDB 的集合空间

- sequoiadb.out.collection：指定作为输出源 SequoiaDB 的集合

- sequoiadb.out.bulknum：指定每次向输出源 SequoiaDB 写入的记录条数，对写入性能进行优化

示例
----

- 读取 HDFS 文件，处理后写入 SequoiaDB 

 ```lang-javascript
 public class HdfsSequoiadbMR {
     static class MobileMapper extends  Mapper<LongWritable,Text,Text,IntWritable>{
         private static final IntWritable ONE=new IntWritable(1);
         @Override
         protected void map(LongWritable key, Text value, Context context)
                 throws IOException, InterruptedException {
             String valueStr=value.toString();
 
             String mobile_prefix=valueStr.split(",")[3].substring(0,3);
             context.write(new Text(mobile_prefix), ONE);
         }
 
     }
 
     static class MobileReducer extends Reducer<Text, IntWritable, NullWritable, BSONWritable>{
 
         @Override
         protected void reduce(Text key, Iterable<IntWritable> values,Context context)
                 throws IOException, InterruptedException {
                 Iterator<IntWritable> iterator=values.iterator();
                 long sum=0;
                 while(iterator.hasNext()){
                     sum+=iterator.next().get();
                 }
                 BSONObject bson=new BasicBSONObject();
                 bson.put("prefix", key.toString());
                 bson.put("count", sum);
                 context.write(null,new BSONWritable(bson));
         }
 
     }
 
     public static void main(String[] args) throws IOException, InterruptedException, ClassNotFoundException {
         if(args.length < 1){
             System.out.print("please set input path ");
             System.exit(1);
         }
         Configuration conf=new Configuration();
         conf.addResource("sequoiadb-hadoop.xml"); //加载配置文件
         Job job=Job.getInstance(conf);
         job.setJarByClass(HdfsSequoiadbMR.class);
         job.setJobName("HdfsSequoiadbMR");
         job.setInputFormatClass(TextInputFormat.class);
         job.setOutputFormatClass(SequoiadbOutputFormat.class); //reduce 输出写入到 SequoiaDB 中
         TextInputFormat.setInputPaths(job, new Path(args[0]));
 
         job.setMapperClass(MobileMapper.class);
         job.setReducerClass(MobileReducer.class);
 
         job.setMapOutputKeyClass(Text.class);
         job.setMapOutputValueClass(IntWritable.class);
 
         job.setOutputKeyClass(NullWritable.class);
         job.setOutputValueClass(BSONWritable.class);
 
         job.waitForCompletion(true);
     }
 }
 ```

- 读取 SequoiaDB 中的数据，处理后写入 HDFS

 ```lang-javascript
 public class SequoiadbHdfsMR {
     static class ProvinceMapper extends Mapper<Object, BSONWritable,IntWritable,IntWritable>{
         private static final IntWritable ONE=new IntWritable(1);
         @Override
         protected void map(Object key, BSONWritable value, Context context)
                 throws IOException, InterruptedException {
                        BSONObject obj = value.getBson();
             int province=(Integer) obj.get("province_code");
             context.write(new IntWritable(province), ONE);
         }
 
     }
 
     static class ProvinceReducer extends Reducer<IntWritable,IntWritable,IntWritable,LongWritable>{
 
         @Override
         protected void reduce(IntWritable key, Iterable<IntWritable> values,
                 Context context)
                 throws IOException, InterruptedException {
             Iterator<IntWritable> iterator=values.iterator();
             long sum=0;
             while(iterator.hasNext()){
                 sum+=iterator.next().get();
             }
             context.write(key,new LongWritable(sum));
         }
 
     }
 
 
     public static void main(String[] args) throws IOException, InterruptedException, ClassNotFoundException {
         if(args.length<1){
             System.out.print("please set  output path ");
             System.exit(1);
         }
         Configuration conf=new Configuration();
         conf.addResource("sequoiadb-hadoop.xml");
         Job job=Job.getInstance(conf);
         job.setJarByClass(SequoiadbHdfsMR.class);
         job.setJobName("SequoiadbHdfsMR");
         job.setInputFormatClass(SequoiadbInputFormat.class);
         job.setOutputFormatClass(TextOutputFormat.class);
 
 
         FileOutputFormat.setOutputPath(job, new Path(args[0]+"/result"));
 
         job.setMapperClass(ProvinceMapper.class);
         job.setReducerClass(ProvinceReducer.class);
 
         job.setMapOutputKeyClass(IntWritable.class);
         job.setMapOutputValueClass(IntWritable.class);
 
         job.setOutputKeyClass(IntWritable.class);
         job.setOutputValueClass(LongWritable.class);
 
         job.waitForCompletion(true);
     }
 }
 ```

- 配置信息

 ```lang-xml
 <?xml version="1.0" encoding="UTF-8"?>
 <configuration>
   <property>
      <name>sequoiadb.input.url</name>
      <value>localhost:11810</value>
   </property>
   <property>
      <name>sequoiadb.output.url</name>
      <value>localhost:11810</value>
   </property>
   <property>
      <name>sequoiadb.in.collectionspace</name>
      <value>default</value>
   </property>
   <property>
      <name>sequoiadb.in.collect</name>
      <value>student</value>
   </property>
   <property>
      <name>sequoiadb.out.collectionspace</name>
      <value>default</value>
   </property>
   <property>
      <name>sequoiadb.out.collect</name>
      <value>result</value>
   </property>
     <property>
      <name>sequoiadb.out.bulknum</name>
      <value>10</value>
   </property>
 </configuration>
 ```

