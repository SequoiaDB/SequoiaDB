Java 驱动连接池用于创建和管理连接。通过连接复用以减少创建连接的资源消耗、提升获取连接的效率。对于需要频繁创建连接的场景，建议使用连接池。

##连接池用法##

连接池的使用步骤主要如下：

1. 创建 SequoiadbDatasource 对象
2. 调用其 getConnection/releaseConnection 方法获取/释放连接
3. 关闭连接池

详情可查看 [Java API][api]。完整的连接池使用示例请参考 SequoiaDB 安装目录下的 samples/Java/com/sequoiadb/samples/Datasource.java

###创建连接池对象###

指定 SequoiaDB 集群的 coord 地址、鉴权信息，以及设置连接池配置参数，之后创建连接池对象。

 ```lang-java
 ArrayList<String> addrs = new ArrayList<String>();
 addrs.add("sdbserver1:11810");  // SequoiaDB 集群的协调节点地址
 addrs.add("sdbserver2:11810");
 addrs.add("sdbserver3:11810");
 
 String userName = "admin";
 String password = "admin";
 
 // 连接池参数配置
 DatasourceOptions dsOpt = new DatasourceOptions();
 dsOpt.setMaxCount(500);
 dsOpt.setMaxIdleCount(50);
 dsOpt.setMinIdleCount(20);

 // 连接参数配置
 ConfigOptions nwOpt = new ConfigOptions();
 nwOpt.setConnectTimeout(200);
 nwOpt.setMaxAutoConnectRetryTime(0);
 
 SequoiadbDatasource ds = SequoiadbDatasource.builder()
                .serverAddress(addrs)
                .userConfig(new UserConfig(userName, password))
                .datasourceOptions(dsOpt)
                .configOptions(nwOpt)
                .build();
 ```

> **Note:**
>
> * 在执行上述程序的机器上，需要配置 Sequoiadb 集群协调节点的主机名/IP地址映射关系。否则上述代码执行时将无法识别 sdbserver1、sdbserver2、sdbserver3。
>
> * 如果某一协调节点不可用，如  sdbserver3:11810 ，那么连接池将不会使用其创建新连接。

###使用连接池###

使用连接池获取、归还连接。

 ```lang-java
 Sequoiadb db = null;
 try {
     // 获取连接
     db = ds.getConnection();
 
     // 使用连接 db...
 } catch (BaseException | InterruptedException e) {
     // 异常处理
 } finally {
     // 归还连接
     if ( db != null ) {
         ds.releaseConnection( db );
     }
 }
 ```

###关闭连接池###

 ```lang-java
 ds.close();
 ```

##配置##

###连接池配置###

用于控制连接池的运行。常用配置如下：

* 设置连接池可管理的最大连接数量，默认值为 500。当连接数量达到上限时，连接池将不再创建新连接。

 ```lang-java
 DatasourceOptions dsOpt = new DatasourceOptions();
 dsOpt.setMaxCount(500);
 ```

* 设置连接池中闲置连接（可直接使用的连接）数量。连接池将动态维持池中的闲置连接数量在 [MinIdleCount, MaxIdleCount] 范围中。

 ```lang-java
 dsOpt.setMaxIdleCount(50);
 dsOpt.setMinIdleCount(20);
 ```

* 设置每次创建新连接的数量，默认值为 10。当连接池闲置连接数量小于 MinIdleCount 时，连接池就会触发连接创建任务，每次最多创建 DeltaIncCount 数量的新连接。

 ```lang-java
 dsOpt.setDeltaIncCount(10);
 ```

* 设置连接池内闲置连接的最大闲置时间，默认值为 0（表示最大闲置时间为无穷大），单位：毫秒。连接池会定期检查池内的闲置连接，并销毁闲置时间超时的连接。

 ```lang-java
 dsOpt.setKeepAliveTimeout(0);
 ```

* 设置闲置连接检查任务的执行周期，默认值为 60000（1 分钟），单位：毫秒。该任务会定期检查闲置连接的数量，以及它们的闲置时间是否超时。

 ```lang-java
 dsOpt.setCheckInterval(60 * 1000);
 ```

###连接配置###

连接池创建新连接时使用的网络参数，常用配置如下：

* 设置连接超时时间，单位：毫秒。

 ```lang-java
 ConfigOptions nwOpt = new ConfigOptions();
 nwOpt.setConnectTimeout(200);
 ```

* 设置连接失败的重试时间，单位：毫秒。建议设置为 0，表示不重试。

 ```lang-java
 nwOpt.setMaxAutoConnectRetryTime(0);
 ```

###位置集配置###

用于配置连接池所属的位置集，以控制连接池优先访问的节点。配置后，连接池将优先访问该位置集下的节点。如果当前位置集中的节点不可用，将在具有亲和性关系的位置集中，选取新的节点进行访问。通过该配置，能够尽量避免访问异地节点，降低网络延时。

1. 集群协调节点及对应的位置集如下：

    ```lang-text
    节点信息          位置集
    sdbserver1:11810  "GuangDong.guangzhou"
    sdbserver2:11810  "GuangDong.shenzhen"
    sdbserver3:11810  "GuangDong"
    sdbserver4:11810  "ShangHai"
    sdbserver5:11810  "ChongQing"
    ```

    >**Note:**
    >
    > 用户可通过 [setLocation()][setLocation_link] 设置节点的位置集。

2. 创建连接池，并指定所属位置集为"GuangDong.guangzhou"

    ```lang-java
    ArrayList<String> addrs = new ArrayList<String>();
    addrs.add("sdbserver1:11810");
    addrs.add("sdbserver2:11810");
    addrs.add("sdbserver3:11810");
    addrs.add("sdbserver4:11810");
    addrs.add("sdbserver5:11810");

    String location = "GuangDong.guangzhou";
    SequoiadbDatasource ds = SequoiadbDatasource.builder()
                .serverAddress(addrs)
                .userConfig(new UserConfig(userName, password))
                .datasourceOptions(dsOpt)
                .configOptions(nwOpt)
                .location(location)
                .build();
     ```

[^_^]:
     本文使用的所有引用和链接
[api]:api/java/html/index.html
[setLocation_link]:manual/Manual/Sequoiadb_Command/SdbNode/setLocation.md