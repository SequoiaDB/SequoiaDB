
本文档主要介绍通过 AWS SDK for C++ 开发的程序样例。

环境
----
RHEL/CentOS 7

[AWS SDK for C++ 1.0.164][download_sdk]

开发
----
### 编译 SDK

1. 配置 yum 源
```lang-bash
vim /etc/yum.repos.d/CentOS-Base.repo
```

2. 将以下内容添加到 yum 源配置文件 `CentOS-Base.repo` 中
```lang-ini
[centos]
name=centos7
baseurl=http://mirrors.163.com/centos/7/os/x86_64/
enabled=1
gpgcheck=0
```

3. 刷新源
```lang-bash
yum makecache fast
```

4. 安装 EPEL 源
```lang-bash
yum -y install epel-release
```

5. 刷新源
```lang-bash
yum makecache fast
```

6. 安装必备软件和库
```lang-bash
yum install openssl-devel
yum -y erase cmake
yum -y install cmake3 gcc-c++ libstdc++-devel libcurl-devel zlib-devel
cd /usr/bin; ln -s cmake3 cmake
```

7. 准备源码
```lang-bash
tar -zxf 1.0.164.tar.gz -C /tmp
mkdir -p /tmp/build; cd /tmp/build
cmake -DCMAKE_BUILD_TYPE=Release /tmp/aws-sdk-cpp-1.0.164
```

8. 编译源码
```lang-bash
make -j `nproc` -C aws-cpp-sdk-core
make -j `nproc` -C aws-cpp-sdk-s3
```

9. 安装头文件和库到一个目录
```lang-bash
mkdir -p /tmp/install
make install DESTDIR=/tmp/install -C aws-cpp-sdk-core
make install DESTDIR=/tmp/install -C aws-cpp-sdk-s3
```

### 编写示例代码

1. 创建文件 `s3example.cpp`

   ```lang-bash
   mkdir /opt/s3cpp;touch s3example.cpp
   ```

2. 将下面代码块复制到 `s3example.cpp` 文件中，保存并退出

   ```lang-cpp
   #include <aws/core/Aws.h>
   #include <aws/core/auth/AWSCredentialsProvider.h>
   #include <aws/s3/S3Client.h>
   #include <aws/s3/model/CreateBucketRequest.h>
   #include <aws/s3/model/Object.h>
   #include <aws/s3/model/PutObjectRequest.h>
   #include <aws/s3/model/ListObjectsRequest.h>
   #include <aws/s3/model/GetObjectRequest.h>
   #include <aws/s3/model/DeleteObjectRequest.h>
   
   #include <iostream>
   #include <fstream>
   #include <sys/stat.h>
   
   using namespace Aws::S3;
   using namespace Aws::S3::Model;
   using namespace std;
   
   /**
    * 创建桶
    * @param bucket_name 桶名
    * @param client 客户端对象
    * @return 是否创建成功
    */
   bool create_bucket(const Aws::String &bucket_name, const Aws::S3::S3Client &client)
   {
   	Aws::S3::Model::CreateBucketRequest request;
   	request.SetBucket(bucket_name);
   
   	auto outcome = client.CreateBucket(request);
   	if (!outcome.IsSuccess())
   	{
   		auto err = outcome.GetError();
   		std::cout << "ERROR: CreateBucket: " <<
   			err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
   		return false;
   	}
   	return true;
   }
   
   /**
    * 判断本地文件是否存在
    * @param name 文件名
    * @return 文件是否存在
    */
   inline bool file_exists(const std::string& name)
   {
   	struct stat buffer;
   	return (stat(name.c_str(), &buffer) == 0);
   }
   
   /**
    * 上传文件
    * @param bucket_name 桶名
    * @param object_name 对象名
    * @param file_name 本地文件名
    * @param client 客户端对象
    * @return 是否上传成功
    */
   bool upload_object(const Aws::String& bucket_name,
   	const Aws::String& object_name,
   	const std::string& file_name,
   	const Aws::S3::S3Client& client)
   {
   	if (!file_exists(file_name)) {
   		std::cout << "ERROR: NoSuchFile: The specified file does not exist"
   			<< std::endl;
   		return false;
   	}
   
   	Aws::Client::ClientConfiguration clientConfig;
   	Aws::S3::S3Client s3_client(clientConfig);
   	Aws::S3::Model::PutObjectRequest object_request;
   
   	object_request.SetBucket(bucket_name);
   	object_request.SetKey(object_name);
   	const std::shared_ptr<Aws::IOStream> input_data =
   		Aws::MakeShared<Aws::FStream>("SampleAllocationTag",
   			file_name.c_str(),
   			std::ios_base::in | std::ios_base::binary);
   	object_request.SetBody(input_data);
   
   	// 上传对象
   	auto put_object_outcome = client.PutObject(object_request);
   	if (!put_object_outcome.IsSuccess()) {
   		auto error = put_object_outcome.GetError();
   		std::cout << "ERROR: " << error.GetExceptionName() << ": "
   			<< error.GetMessage() << std::endl;
   		return false;
   	}
   	return true;
   }
   
   /**
    * 获取对象列表
    * @param bucket_name 桶名
    * @param client 客户端对象
    */
   void list_object(const Aws::String& bucket_name,
   	const Aws::S3::S3Client& client)
   {
   	Aws::S3::Model::ListObjectsRequest objects_request;
   	objects_request.WithBucket(bucket_name);
   
   	auto list_objects_outcome = client.ListObjects(objects_request);
   
   	if (list_objects_outcome.IsSuccess())
   	{
   		Aws::Vector<Aws::S3::Model::Object> object_list =
   			list_objects_outcome.GetResult().GetContents();
   
   		for (auto const &object : object_list)
   		{
   			std::cout << "* " << object.GetKey() << std::endl;
   		}
   	}
   	else
   	{
   		std::cout << "ListObjects error: " <<
   			list_objects_outcome.GetError().GetExceptionName() << " " <<
   			list_objects_outcome.GetError().GetMessage() << std::endl;
   	}
   }
   
   /**
    * 下载文件
    * @param bucket_name 桶名
    * @param object_name 对象名
    * @param client 客户端对象
    * @param filepath 本地文件路径
    */
   void download_object(const Aws::String& bucket_name,
   	const Aws::String& object_name,
   	const Aws::S3::S3Client& client,
   	const std::string& filepath)
   {
   	Aws::S3::Model::GetObjectRequest object_request;
   	object_request.SetBucket(bucket_name);
   	object_request.SetKey(object_name);
   
   	// 获取对象
   	auto get_object_outcome = client.GetObject(object_request);
   	if (get_object_outcome.IsSuccess())
   	{
   		// 获取流
   		auto &retrieved_file = get_object_outcome.GetResultWithOwnership().GetBody();
   
   		// 写入本地文件
   		const char * filename = filepath.data();
   		std::ofstream output_file(filename, std::ios::binary);
   		output_file << retrieved_file.rdbuf();
   	}
   	else
   	{
   		auto error = get_object_outcome.GetError();
   		std::cout << "ERROR: " << error.GetExceptionName() << ": "
   			<< error.GetMessage() << std::endl;
   	}
   }
   
   /**
    * 删除文件
    * @param bucket_name 桶名
    * @param object_name 对象名
    * @param client 客户端对象
    * @return 是否删除成功
    */
   bool delete_object(const Aws::String& bucket_name,
   	const Aws::String& object_name,
   	const Aws::S3::S3Client& client)
   {
   	Aws::S3::Model::DeleteObjectRequest object_request;
   	object_request.WithBucket(bucket_name).WithKey(object_name);
   
   	auto delete_object_outcome = client.DeleteObject(object_request);
   
   	if (!delete_object_outcome.IsSuccess())
   	{
   		std::cout << "DeleteObject error: " <<
   			delete_object_outcome.GetError().GetExceptionName() << " " <<
   			delete_object_outcome.GetError().GetMessage() << std::endl;
   		return false;
   	}
   	return true;
   }
   
   /**
    * 获取桶列表
    * @param client 客户端对象
    */
   void list_bucket(const Aws::S3::S3Client& client)
   {
       auto response = client.ListBuckets();
       if (response.IsSuccess()) {
           auto buckets = response.GetResult().GetBuckets();
           for (auto iter = buckets.begin(); iter != buckets.end(); ++iter) {
               cout << iter->GetName() << "\t" << iter->GetCreationDate().ToLocalTimeString(Aws::Utils::DateFormat::ISO_8601) << endl;
           }
       } else {
           cout << "Error while ListBuckets " << response.GetError().GetExceptionName()
               << " " << response.GetError().GetMessage() << endl;
       }
   
   }
   
   /**
    * 主函数
    */
   int main(int argc, char* argv[])
   {
   	Aws::SDKOptions options;
   	options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace;
   	Aws::InitAPI(options);
   	Aws::Client::ClientConfiguration cfg;
   	// S3服务器地址和端口
   	cfg.endpointOverride = "192.168.44.200:8002";
   	cfg.scheme = Aws::Http::Scheme::HTTP;
   	cfg.verifySSL = false;
   
   	// 认证的Key (accessKeyId, secretKey)
   	Aws::Auth::AWSCredentials cred("ABCDEFGHIJKLMNOPQRST", "abcdefghijklmnopqrstuvwxyz0123456789ABCD");
   	Aws::S3::S3Client client(cred, cfg, false, false);
   
   	// 相关变量
   	const Aws::String bucket_name = "mybucket";
   	const Aws::String object_name = "s3exmple.cpp";
   	const std::string file_name = "/opt/s3cpp/s3exmple.cpp";
   
   	// 创建桶
   	std::string message = create_bucket(bucket_name, client) ? "创建成功" : "创建失败";
   	std::cout << message << std::endl;
   
   	// 获取桶列表
   	list_bucket(client);
   
       // 上传文件
   	if (upload_object(bucket_name, object_name, file_name, client)) {
   		std::cout << "将文件 " << file_name
   			<< " 上传到桶 " << bucket_name
   			<< " 中的 " << object_name
   			<< "对象 操作成功" << std::endl;
   	}
   
   	// 获取对象列表
   	list_object(bucket_name, client);
   
   	// 下载对象
   	download_object(bucket_name, object_name, client, "/opt/s3cpp/s3exmple.cpp.download");
   
   	// 删除对象
   	message = delete_object(bucket_name, object_name, client) ? "删除成功" : "删除失败";
   	std::cout << message << std::endl;;
   
   	Aws::ShutdownAPI(options);
   }
   ```

### 编译示例代码并运行

1. 编译
```lang-bash
g++ -std=c++11 -I/tmp/install/usr/local/include -L/tmp/install/usr/local/lib64 -laws-cpp-sdk-core -laws-cpp-sdk-s3 s3example.cpp -o s3example
```

2. 指定环境变量
```lang-bash
export LD_LIBRARY_PATH=/tmp/install/usr/local/lib64
```

3. 运行生成的可执行文件 `s3example`
```lang-bash
[root@sdb02 s3cpp]# ./s3example 
创建成功
mybucket	2019-05-28T17:45:16Z
将文件 /opt/s3cpp/s3example.cpp 上传到桶 mybucket 中的 s3example.cpp对象 操作成功
* s3exmple.cpp
删除成功
```

[^_^]:
    相关连接

[download_sdk]: https://github.com/aws/aws-sdk-cpp/archive/1.0.164.tar.gz