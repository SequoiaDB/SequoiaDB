本文档介绍如何使用 Java 接口向 SequoiaS3 发送请求及接收响应。

SequoiaS3 安装路径下的 `sample` 目录中有一个 maven 类型的 Java 工程样例。
使用 IDEA 打开该工程(【File】->【Open】->选中文件夹中的 `Pom.xml`->【Open as Project】->【Open Existing Project】->【New Window】)，将 `Test.java` 中的 endPoint 修改为提供 S3 服务的 IP 和端口，开始使用 `sample` 中的样例对存储桶和对象及区域进行操作。

初始化客户端
----

首先修改 endPoint 的地址和端口，使其指向 SequoiaS3 的地址和端口。

```lang-java
String accessKey="ABCDEFGHIJKLMNOPQRST";
String secretKey="abcdefghijklmnopqrstuvwxyz0123456789ABCD";
String endPoint = "http://localhost:8002";

SequoiaS3 sequoiaS3 = SequoiaS3ClientBuilder.standard()
            .withEndpoint(endPoint)
            .withAccessKeys(accessKey, secretKey)
            .build();

AWSCredentials credentials = new BasicAWSCredentials(accessKey,secretKey);
AwsClientBuilder.EndpointConfiguration endpointConfiguration = new AwsClientBuilder.EndpointConfiguration(endPoint, null);
AmazonS3 s3 = AmazonS3ClientBuilder.standard()
            .withEndpointConfiguration(endpointConfiguration)
            .withCredentials(new AWSStaticCredentialsProvider(credentials))
            .build();
```

参数定义
----

```lang-java
String regionName = "region-example";
String bucketName = "bucketname";
String objectName = "objectname";
File file = new File("example.png");
```

创建区域
----

创建一个名为"region-example"的区域，该区域设置每年创建一个新的集合空间，在该集合空间中每月创建一个新的集合，用于存放对象数据

```lang-java
CreateRegionRequest request = new CreateRegionRequest(regionName)
            .withDataCLShardingType(DataShardingType.MONTH)
            .withDataCSShardingType(DataShardingType.YEAR);
sequoiaS3.createRegion(request);
```

获取区域列表
----

查询当前系统中的区域列表

```lang-java
ListRegionsResult listRegionsResult = sequoiaS3.listRegions();
```

查询区域配置
----

查询区域配置及区域内的存储桶列表

```lang-java
GetRegionResult regionResult = sequoiaS3.getRegion(regionName);
Region region = regionResult.getRegion();
List<String> buckets = regionResult.getBuckets();
System.out.println("region:" + region.toString());
for (int i=0; i < buckets.size(); i++) {
    System.out.println("Name:" + buckets.get(i));
}
```

创建存储桶
----

在 region-example 区域中创建一个名为"bucketname"的桶

```lang-java
s3.createBucket(bucketName, regionName );
```

开启版本控制
----

打开指定存储桶的版本控制功能

该功能未开启时，同一对象多次上传，历史记录会被覆盖，该功能开启后，同一对象多次上传的历史记录都会被记录在系统中

```lang-java
BucketVersioningConfiguration cfg = new BucketVersioningConfiguration("Enabled");
SetBucketVersioningConfigurationRequest request = new SetBucketVersioningConfigurationRequest(bucketName, cfg);
s3.setBucketVersioningConfiguration(request);
```

上传对象
----

从本地上传一个名为 `example.png` 的文件到存储桶中，并命名为"objectname"

```lang-java
PutObjectRequest request = new PutObjectRequest(bucketName, objectName, file);
s3.putObject(request);
```

获取对象
----

从存储桶中获得对象内容，并将对象内容存储在本地文件中

```lang-java
String filePath = "example.png";
GetObjectRequest request = new GetObjectRequest(bucketName, objectName);
S3Object result = s3.getObject(request);
S3ObjectInputStream s3is = result.getObjectContent();
FileOutputStream fos = new FileOutputStream(new File(filePath));
byte[] read_buf = new byte[1024];
int read_len = 0;
while ((read_len = s3is.read(read_buf)) > 0) {
    fos.write(read_buf, 0, read_len);
}
s3is.close();
fos.close();
```

复制对象
----

复制源对象到目标对象

```lang-java
CopyObjectRequest request = new CopyObjectRequest(sourceBucket, sourceObject, destBucket, destObject);
CopyObjectResult result = s3.copyObject(request);
```

获取指定版本的对象
----

获取指定版本的对象，当不指定 versionId 时，获取最新版本的对象

```lang-java
GetObjectRequest request = new GetObjectRequest(bucketName, objectName, versionId);
S3Object object = s3.getObject(request);
```

查询桶内对象列表
----

查询存储桶中所有对象

```lang-java
ListObjectsV2Result result = s3.listObjectsV2(bucketName);
```

查询桶中所有版本
----

查询指定存储桶中的所有版本的对象信息，包括历史版本以及删除标记，当桶中版本记录过多，可以进行多次分批查询。

```lang-java
ListVersionsRequest request = new ListVersionsRequest()
                                  .withBucketName(bucketName);
VersionListing result = s3.listVersions(request);
if (result.isTruncated())
{
   result = s3.listNextBatchOfVersions(result);
}
```

删除对象
----

删除指定对象

版本功能未开启时，删除指定对象会直接将对象内容从系统中删除，版本功能开启后，删除操作会在系统中生成一个对象的删除标记，原对象内容会作为历史记录保留在系统中

```lang-java
s3.deleteObject(bucketName, objectName);
```

删除指定版本的对象
----

删除指定版本的对象，可以删除历史版本或删除标记，该操作会彻底删除系统中关于该版本的记录

```lang-java
s3.deleteVersion(bucketName, objectName, versionId);
```

删除多个对象
----

一次删除多个对象。对每个删除的对象可以指定版本号也可以不指定，当不指定版本号时相当于 deleteObject，当指定版本号时相当于 deleteVersion，deleteObjects 接口等于一次执行了多个 deleteObject 和 deleteVersion。

```lang-java
DeleteObjectsRequest request = new DeleteObjectsRequest(bucketName);
List<DeleteObjectsRequest.KeyVersion> keys = new ArrayList<DeleteObjectsRequest.KeyVersion>();
VersionListing versions = s3.listVersions(new ListVersionsRequest().withBucketName(bucketName));

List<S3VersionSummary> versionSummaries = versions.getVersionSummaries();
for (S3VersionSummary object : versionSummaries){
   keys.add(new DeleteObjectsRequest.KeyVersion(object.getKey(),object.getVersionId()));
}

request.setKeys(keys);
s3.deleteObjects(request);
```

删除桶
----

删除指定存储桶

```lang-java
s3.deleteBucket(bucketName);
```

删除区域
----

删除指定区域

```lang-java
sequoiaS3.deleteRegion(regionName);
```

查询区域是否存在
----

判断指定区域是否存在

```lang-java
Boolean isRegionExist = sequoiaS3.headRegion(regionName);
System.out.println("Region("+ regionName +") exist: " + isRegionExist);
```





