
本文档将介绍 SequoiaS3 支持的 Rest 接口。

## 桶API

下述将介绍桶相关的接口。

### GET Service

查询用户创建的所有存储桶

**请求语法**

```lang-rest
GET / HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**结果解析**

查询结果以 XML 形式在响应消息体中显示。

| 元素 | 说明 |
| ---- | ---- |
| ListAllMyBucketsResult| 包含 Owner 和 Buckets |
| Owner | 存储桶所有者，包含 ID 和 DisplayName，属于 ListAllMyBucketsResult |
| ID | 存储桶所有者的 ID，属于 ListAllMyBucketsResult.Owner |
| DisplayName | 存储桶所有者的名称，属于 ListAllMyBucketsResult.Owner |
| Buckets | 桶列表，包含若干 Bucket，属于 ListAllMyBucketsResult |
| Bucket | 存储桶，包含 Name 和 CreationDate，属于 ListAllMyBucketsResult.Buckets |
| Name | 存储桶名称，属于 ListAllMyBucketsResult.Buckets.Bucket |
| CreationDate | 存储桶创建时间，属于 ListAllMyBucketsResult.Buckets.Bucket |

**示例**

响应结果如下：

```lang-xml
<ListAllMyBucketsResult>
  <Owner>
    <DisplayName>username</DisplayName>
    <ID>34455</ID>
  </Owner>
  <Buckets>
    <Bucket>
      <Name>mybucket</Name>
      <CreationDate>2019-02-03T16:45:09.000Z</CreationDate>
    </Bucket>
    <Bucket>
      <Name>samples</Name>
      <CreationDate>2019-02-03T16:41:58.000Z</CreationDate>
    </Bucket>
  </Buckets>
</ListAllMyBucketsResult>
```

### PUT Bucket

创建存储桶

> **Note:**
>
> 存储桶名需在整个系统内唯一，长度在 3~63 之间。

**请求语法**

```lang-rest
PUT /bucketname HTTP/1.1
Host: ip:port
Content-Length: length
Date: date
Authorization: authorization string
<CreateBucketConfiguration>
  <LocationConstraint>Region</LocationConstraint>
</CreateBucketConfiguration>
```

**请求元素**

用户需要在请求消息体中使用 XML 形式指定存储桶创建的区域，如果不指定则存储桶创建在默认的区域上。

| 元素 | 说明 |
| ---- | ---- |
| CreateBucketConfiguration | 包含 LocationConstraint |
| LocationConstraint | 指定创建存储桶的区域，类型为 string |

**示例**

响应结果如下：

```lang-rest
HTTP/1.1 200 OK
Location: /bucketName
Content-Length: 0
Date: Fri, 16 Aug 2019 11:11:53 GMT
```


### DELETE Bucket

删除存储桶

**请求语法**

```lang-rest
DELETE /bucketname HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**示例**

响应结果如下：

```lang-rest
HTTP/1.1 204 No Content
Date: Fri, 16 Aug 2019 10:11:53 GMT
```


### HEAD Bucket

检查一个存储桶是否存在

**请求语法**

```lang-rest
HEAD /bucketname HTTP/1.1
Date: date
Authorization: authorization string
Host: ip:port
```

**示例**

响应结果如下：

```lang-rest
HTTP/1.1 200 OK
Date: Fri, 16 Aug 2019 10:10:53 GMT
```


### PUT Bucket versioning

修改存储桶的版本控制状态

**请求语法**

```lang-rest
PUT /bucketname?versioning HTTP/1.1
Host: ip:port
Content-Length: length
Date: date
Authorization: authorization string

<VersioningConfiguration>
  <Status>VersioningState</Status>
</VersioningConfiguration>
```

**参数说明**

| 参数名 | 说明 |
| ----   | ---- |
| versioning | 表示该请求为修改存储桶的版本控制状态 |

**请求元素**

在请求消息体中使用 XML 形式指定存储桶的版本控制状态。

| 元素 | 说明 |
| ---- | ---- |
| VersioningConfiguration | 包含 Status |
| Status | 版本控制状态，有效值为 Suspended\|Enabled，属于 VersioningConfiguration |

**示例**

响应结果如下：

```lang-rest
HTTP/1.1 200 OK
Date: Wed, 01 Mar  2006 12:00:00 GMT
```


### GET Bucket versioning

查询桶的版本控制状态

**请求语法**

```lang-rest
GET /bucketname?versioning HTTP/1.1
Host: ip:port
Content-Length: length
Date: date
Authorization: authorization string
```

**结果解析**

查询结果以 XML 形式在响应消息头中显示。

| 元素 | 说明 |
| ---- | ---- |
| VersioningConfiguration | 包含 Status |
| Status | 版本控制状态，有效值为 Suspended|Enabled，属于 VersioningConfiguration |

**示例**

- 打开版本控制开关，查询结果如下：

    ```lang-xml
    <VersioningConfiguration>
      <Status>Enabled</Status>
    </VersioningConfiguration>
    ```

- 禁用版本控制，查询结果如下：

    ```lang-xml
    <VersioningConfiguration>
      <Status>Suspended</Status>
    </VersioningConfiguration>
    ```

- 从未开启或禁用过版本控制，查询结果如下：

    ```lang-xml
    <VersioningConfiguration/>
    ```

### GET Bucket location

查询桶所在的区域

**请求语法**

```lang-rest
GET /bucketname?location HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**结果解析**

查询结果以 XML 格式在响应消息体中显示。

| 元素 | 说明 |
| ---- | ---- |
| LocationConstraint | 桶所在的区域 |

**示例**

- 已经配置了区域的存储桶，查询结果如下：

    ```lang-xml
    <LocationConstraint>region</LocationConstraint>
    ```

- 未配置区域的存储桶，查询结果如下：

    ```lang-xml
    <LocationConstraint/>
    ```


### GET Bucket (List Objects) Version 1

查询存储桶内对象列表

**请求语法**

```lang-rest
GET /bucketname HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| prefix | 前缀，类型为 string，返回具有前缀的对象列表， |
| delimiter | 分隔符，类型为 string，如果指定 prefix，则 prefix 后第一次出现的分隔符之间包含相同字符串的所有键都被分组在一个 CommonPrefixes；如果未指定 prefix 参数，则子字符串从对象名称的开头开始|
| marker | 指定在存储桶中列出对象要开始的键，类型为 string，返回对象键按照 UTF-8 二进制顺序从该标记后的键开始按顺序排列 |
|max-keys | 设置响应中返回的最大键数，类型为 string，默认值为 1000，如果要查询返回数量少于 1000，可以填写其他值，填写超过 1000 的值，仍然按照  1000 条返回 | 
| encoding-type | 响应结果编码类型，只支持 url，由于对象名称可以包含任意字符，但是 XML 对某些特别的字符无法解析，所以需要对响应中的对象名称进行编码 |

**结果解析**

查询结果以 XML 形式在响应消息体中显示。

| 元素 | 说明 |
| ---- | ---- |
| ListBucketResult | 包含存储桶信息、查询条件和查询的对象信息 |
| Name | 存储桶名称 |
| Prefix | 查询的 prefix 条件 |
| Delimiter | 查询的 delimiter 条件 |
| Marker | 查询的 marker 条件 |
| MaxKeys | 查询的 maxKeys 条件 |
| Encoding-Type | 查询的 encoding-type 条件 |
| IsTruncated | 如果该字段为 true，说明由于条数限制，本次没有查询完所有符合条件的结果，可以使用 NextMarker 作为下一次查询的 Marker 条件继续查询剩余内容 |
| NextMarker | 当 IsTruncated 为 true 时，该字段返回的是本次查询的最后一条的记录 |
| CommonPrefixes | 当查询条件指定了 Delimter 时，Prefix 后面第一次出现 Delimiter 的位置（包括 Delimiter）之前的内容作为 CommonPrefix，当有多个对象具有相同的 CommonPrefix 时，只返回一条 CommonPrefix，计数一次，对象信息不返回 |
| Prefix | CommonPrefix 包含的前缀，属于 ListBucketResult.CommonPrefixes |
| Contents | 包含对象的元数据 | 
| Key | 对象的名称，属于 ListBucketResult.Contents |
| LastModified | 创建对象的时间，属于 ListBucketResult.Contents |
| ETag | 对象的 MD5 值，属于 ListBucketResult.Contents |
| Size | 对象的大小，单位为字节，属于 ListBucketResult.Contents |
| Owner | 存储桶的所有者，属于 ListBucketResult.Contents |
| ID | 存储桶所有者的 ID，属于 ListBucketResult.Contents.Owner |
| DisplayName | 桶所有者的名字，属于 ListBucketResult.Contents.Owner |


**示例**

- 不携带查询条件，查询存储桶内所有记录

    ```lang-rest
    GET /bucketname HTTP/1.1
    Host: ip:port
    Date: date
    Authorization: authorization string
    ```
   
    查询结果如下：
   
    ```lang-xml
    <ListBucketResult>
        <Name>bucketname</Name>
        <Prefix/>
        <Marker/>
        <MaxKeys>1000</MaxKeys>
        <IsTruncated>false</IsTruncated>
        <Contents>
            <Key>my-image.jpg</Key>
            <LastModified>2019-08-12T17:50:30.000Z</LastModified>
            <ETag>"fba9dede5f27731c9771645a39863328"</ETag>
            <Size>434234</Size>
            <Owner>
                <ID>125664</ID>
                <DisplayName>username</DisplayName>
            </Owner>
        </Contents>
        <Contents>
           <Key>my-third-image.jpg</Key>
             <LastModified>2019-08-12T17:51:30.000Z</LastModified>
             <ETag>"1b2cf535f27731c974343645a3985328"</ETag>
             <Size>64994</Size>
             <Owner>
                <ID>125664</ID>
                <DisplayName>username</DisplayName>
            </Owner>
        </Contents>
    </ListBucketResult>
    ```

- 本次请求指定 prefix 为 N，起始位置为 Ned，并只返回 100 条记录

    ```lang-rest
    GET /mybucket?prefix=N&marker=Ned&max-keys=100 HTTP/1.1
    Host: iP:port
    Date: date
    Authorization: authorization string
    ```
    
    查询结果如下：
    
    ```lang-xml
    <ListBucketResult>
      <Name>mybucket</Name>
      <Prefix>N</Prefix>
      <Marker>Ned</Marker>
      <MaxKeys>100</MaxKeys>
      <IsTruncated>false</IsTruncated>
      <Contents>
        <Key>Nelson</Key>
        <LastModified>2019-08-12T12:00:00.000Z</LastModified>
        <ETag>"828ef3fdfa96f00ad9f27c383fc9ac7f"</ETag>
        <Size>5</Size>
        <Owner>
          <ID>125664</ID>
          <DisplayName>username</DisplayName>
         </Owner>
      </Contents>
      <Contents>
        <Key>Neo</Key>
        <LastModified>2019-08-12T12:01:00.000Z</LastModified>
        <ETag>"828ef3fdfa96f00ad9f27c383fc9ac7f"</ETag>
        <Size>4</Size>
         <Owner>
          <ID>125664</ID>
          <DisplayName>username</DisplayName>
        </Owner>
     </Contents>
    </ListBucketResult>
    ```

- 桶内已经有如下对象：

   ```lang-text
   sample.jpg
    photos/2006/January/sample.jpg
    photos/2006/February/sample2.jpg
    photos/2006/February/sample3.jpg
    photos/2006/February/sample4.jpg
    ```
    
    本次请求携带分隔符/
    
    ```lang-test
    GET /mybucket-2?delimiter=/ HTTP/1.1
    Host: ip:port
    Date: date
    Authorization: authorization string
    ```
    
    查询结果如下：
    
    ```lang-xml
    <ListBucketResult>
      <Name>mybucket-2</Name>
      <Prefix/>
      <Marker/>
      <MaxKeys>1000</MaxKeys>
      <Delimiter>/</Delimiter>
      <IsTruncated>false</IsTruncated>
      <Contents>
        <Key>sample.jpg</Key>
        <LastModified>2019-08-12T12:01:00.000Z</LastModified>
        <ETag>"bf1d737a4d46a19f3bced6905cc8b902"</ETag>
        <Size>142863</Size>
        <Owner>
          <ID>canonical-user-id</ID>
          <DisplayName>display-name</DisplayName>
        </Owner>
      </Contents>
      <CommonPrefixes>
        <Prefix>photos/</Prefix>
      </CommonPrefixes>
    </ListBucketResult>
    ```

### GET Bucket (List Objects) Version 2

查询桶内对象列表

**请求语法**

```
GET /bucketname?list-type=2 HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| list-type | 固定设置为 2，List Objects 的第二个版本 |
| prefix | 前缀，类型为 string，返回具有前缀的对象列表 | 
| delimiter | 分隔符，类型为 string，如果指定 prefix，则 prefix 后第一次出现的分隔符之间包含相同字符串的所有键都被分组在一个 CommonPrefixes；如果未指定 prefix 参数，则子字符串从对象名称的开头开始 |
| start-after | 指定在存储桶中列出对象要开始的键，类型为 string，返回对象键按照 UTF-8 二进制顺序从该标记后的键开始按顺序排列 |
| max-keys | 设置响应中返回的最大键数，类型为 string，默认值 1000，如果要查询返回数量少于 1000，可以填写其他值，填写超过 1000 的值，仍然按照 1000 条返回 |
| encoding-type | 响应结果编码类型，只支持 url，由于对象名称可以包含任意字符，但是 XML 对某些特别的字符无法解析，所以需要对响应中的对象名称进行编码 |
| continuation-token | 当响应结果被截断，还有部分未返回时，响应结果中会包含 NextContinuationToken，要列出下一组对象，可以使用 NextContinuationToken 下一个请求中的元素作为 continuation-token |
| fetch-owner | 默认情况下，结果中不会返回 Owner 信息，如果要在响应中包含 Owner 信息，将该参数置为 true |

**结果解析**

查询结果以 XML 形式在响应消息体中显示。

| 元素 | 说明 |
| ---- | ---- |
| ListBucketResult | 包含存储桶信息、查询条件和查询的对象信息 |
| Name | 存储桶名称 |
| Prefix | 查询的 prefix 条件 |
| Delimiter | 查询的 delimiter 条件 |
| StartAfter | 查询的 start-after 条件 |
| ContinuationToken | 查询的 continuation-token 条件 |
| MaxKeys | 查询的 maxKeys 条件 |
| Encoding-Type | 查询的 encoding-type 条件 |
| KeyCount | 本次查询返回的记录数 |
| IsTruncated | 如果该字段为 true，说明由于条数限制，本次没有查询完所有符合条件的结果，可以使用 NextMarker 作为下一次查询的 Marker 条件继续查询剩余内容 |
| NextContinuationToken | 当 IsTruncated 为 true 时，NextContinuationToken 记录位置，下一次请求在 continuation-token 携带该令牌继续查询下一组记录 |
| CommonPrefixes | 当查询条件指定了 Delimter 时，Prefix 后面第一次出现 Delimiter 的位置（包括 Delimiter）之前的内容作为 CommonPrefix，当有多个对象具有相同的 CommonPrefix 时，只返回一条 CommonPrefix，计数一次，对象信息不返回 |
| Prefix | CommonPrefix 包含的前缀，属于 ListBucketResult.CommonPrefixes |
| Contents | 包含对象的元数据 |
| Key | 对象的名称，属于 ListBucketResult.Contents |
| LastModified | 创建对象的时间，属于 ListBucketResult.Contents |
| ETag | 对象的 MD5 值，属于 ListBucketResult.Contents |
| Size | 对象的大小，单位为字节，属于 ListBucketResult.Contents |
| Owner | 存储桶的所有者，属于 ListBucketResult.Contents |
| ID | 存储桶所有者的 ID，属于 ListBucketResult.Contents.Owner |
| DisplayName | 桶所有者的名字，属于 ListBucketResult.Contents.Owner |

**示例**

- 不携带查询条件，查询存储桶内所有记录

    ```lang-rest
    GET /bucketname?list-type=2 HTTP/1.1
    Host: ip:port
    Date: Sat, 17 Aug 2019 17:51:00 GMT
    Authorization: authorization string
    ```
    
    查询结果如下：
    
    ```lang-xml
    <ListBucketResult>
        <Name>bucketname</Name>
        <Prefix/>
        <Marker/>
        <KeyCount>205</KeyCount>
        <MaxKeys>1000</MaxKeys>
        <IsTruncated>false</IsTruncated>
        <Contents>
            <Key>my-image.jpg</Key>
            <LastModified>2019-08-12T17:50:30.000Z</LastModified>
            <ETag>"fba9dede5f27731c9771645a39863328"</ETag>
            <Size>434234</Size>
            <Owner>
                <ID>125664</ID>
                <DisplayName>username</DisplayName>
            </Owner>
        </Contents>
        <Contents>
           ...
        </Contents>
        ...
    </ListBucketResult>
    ```

- 指定 prefix 为 N，起始位置为 Ned，并只返回 100 条记录

    ```lang-rest
    GET /mybucket?list-type=2&prefix=N&start-after=Ned&max-keys=100 HTTP/1.1
    Host: iP:port
    Date: Sat, 17 Aug 2019 17:45:00 GMT
    Authorization: authorization string
    ```
    
    查询结果如下，实际查询到两条符合条件的记录：
    
    ```lang-xml
    <ListBucketResult>
      <Name>mybucket</Name>
      <Prefix>N</Prefix>
      <Marker>Ned</Marker>
      <KeyCount>2</KeyCount>
      <MaxKeys>100</MaxKeys>
      <IsTruncated>false</IsTruncated>
      <Contents>
        <Key>Nelson</Key>
        <LastModified>2019-08-12T12:00:00.000Z</LastModified>
        <ETag>"828ef3fdfa96f00ad9f27c383fc9ac7f"</ETag>
        <Size>5</Size>
        <Owner>
          <ID>125664</ID>
          <DisplayName>username</DisplayName>
         </Owner>
      </Contents>
      <Contents>
        <Key>Neo</Key>
        <LastModified>2019-08-12T12:01:00.000Z</LastModified>
        <ETag>"828ef3fdfa96f00ad9f27c383fc9ac7f"</ETag>
        <Size>4</Size>
         <Owner>
          <ID>125664</ID>
          <DisplayName>username</DisplayName>
        </Owner>
     </Contents>
    </ListBucketResult>
    ```

- 桶内已经有如下对象

   ```lang-text
   sample.jpg
   photos/2006/January/sample.jpg
    photos/2006/February/sample2.jpg
    photos/2006/February/sample3.jpg
    photos/2006/February/sample4.jpg
    ```
    
    本次请求携带分隔符/
    
    ```lang-rest
    GET /mybucket-2?list-type=2&delimiter=/ HTTP/1.1
    Host: ip:port
    Date: date
    Authorization: authorization string
    ```
    
    查询结果如下：
    
    ```lang-xml
    <ListBucketResult>
      <Name>mybucket-2</Name>
      <Prefix/>
      <Marker/>
      <KeyCount>2</KeyCount>
      <MaxKeys>1000</MaxKeys>
      <Delimiter>/</Delimiter>
      <IsTruncated>false</IsTruncated>
      <Contents>
        <Key>sample.jpg</Key>
        <LastModified>2019-08-12T12:01:00.000Z</LastModified>
        <ETag>"bf1d737a4d46a19f3bced6905cc8b902"</ETag>
        <Size>142863</Size>
        <Owner>
          <ID>canonical-user-id</ID>
          <DisplayName>display-name</DisplayName>
        </Owner>
      </Contents>
      <CommonPrefixes>
        <Prefix>photos/</Prefix>
      </CommonPrefixes>
    </ListBucketResult>
    ```

### GET Bucket Object versions

查询桶内对象的所有版本

**请求语法**

```lang-rest
GET /bucketname?versions HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| prefix | 前缀，类型为 string，返回具有前缀的对象列表 |
| delimiter | 分隔符，类型为 string，如果指定 prefix，则 prefix 后第一次出现的分隔符之间包含相同字符串的所有键都被分组在一个 CommonPrefixes；如果未指定 prefix 参数，则子字符串从对象名称的开头开始 |
| key-marker | 指定在存储桶中列出对象要开始的键，类型为 string，返回对象键按照 UTF-8 二进制顺序从该标记后的键开始按顺序排列 |
| version-id-marker | 指定起始位置的 version，仅在指定了 key-marker 的情况下有效 |
| max-keys | 设置响应中返回的最大键数，类型为 string，默认值 1000，如果要查询返回数量少于 1000，可以填写其他值，填写超过 1000 的值，仍然按照 1000 条返回  |
| encoding-type | 响应结果编码类型，只支持 url，由于对象名称可以包含任意字符，但是 XML 对某些特别的字符无法解析，所以需要对响应中的对象名称进行编码 |

**结果解析**

查询结果以 XML 形式在响应消息体中显示。

| 元素 | 说明 |
| ---- | ---- |
| ListVersionsResult | 包含存储桶信息、查询条件和查询的对象版本信息 |
| Name | 存储桶名称 |
| Prefix | 查询的 prefix 条件 |
| Delimiter | 查询的 delimiter 条件 |
| KeyMarker | 查询的 key-marker 条件 |
| VersionIDMarker | 查询的 version-id-marker 条件 |
| MaxKeys | 查询的 maxKeys 条件 |
| Encoding-Type | 查询的 encoding-type 条件 |
| IsTruncated | 如果该字段为 true，说明由于条数限制，本次没有查询完所有符合条件的结果，可以使用 NextMarker 作为下一次查询的 Marker 条件继续查询剩余内容 |
| NextKeyMarker | 当 IsTruncated 为 true 时，NextKeyMarker 记录本次返回的最后一个对象或者 CommonPrefix |
| NextVersionIdMarker | 当 IsTruncated 为 true 时，NextVersionIdMarker 记录本次返回的最后一条记录的 version |
| CommonPrefixes | 当查询条件指定了 Delimter 时，Prefix 后面第一次出现 Delimiter 的位置（包括 Delimiter）之前的内容作为 CommonPrefix，当有多个对象具有相同的 CommonPrefix 时，只返回一条 CommonPrefix，计数一次，对象信息不返回 |
| Prefix | CommonPrefix 包含的前缀，属于 ListVersionsResult.CommonPrefixes |
| Version | 包含对象版本的元数据 |
| DeleteMarker | 包含删除标记 |
| Key | 对象的名称，属于 ListVersionsResult.Version\|ListVersionsResult.DeleteMarker |
| VersionId | 对象的版本号，属于 ListVersionsResult.Version\|ListVersionsResult.DeleteMarker |
| IsLatest | 是否是最新版本，属于 ListVersionsResult.Version\|ListVersionsResult.DeleteMarker |
| LastModified | 创建对象的时间，属于 ListVersionsResult.Version\|ListVersionsResult.DeleteMarker |
| ETag | 对象的 MD5 值，属于 ListVersionsResult.Version |
| Size | 对象的大小，单位为字节，属于 ListVersionsResult.Version |
| Owner | 存储桶的所有者，属于 ListVersionsResult.Version\|ListVersionsResult.DeleteMarker |
| ID | 存储桶所有者的 ID，属于 ListVersionsResult.Version.Owner\|ListVersionsResult.DeleteMarker.Owner |
| DisplayName | 桶所有者的名字，属于 ListVersionsResult.Version.Owner\|ListVersionsResult.DeleteMarker.Owner |

**示例**

- 查询存储桶内所有版本

    ```lang-rest
    GET /bucketname?versions HTTP/1.1
    Host: ip:port
    Date: date
    Authorization: authorization string
    ```
    
    查询结果如下：
    
    ```lang-xml
    <ListVersionsResult>
        <Name>bucket</Name>
        <Prefix>my</Prefix>
        <KeyMarker/>
        <VersionIdMarker/>
        <MaxKeys>1000</MaxKeys>
        <IsTruncated>false</IsTruncated>
        <Version>
            <Key>my-image.jpg</Key>
            <VersionId>234</VersionId>
            <IsLatest>true</IsLatest>
            <LastModified>2019-08-16T17:50:32.000Z</LastModified>
            <ETag>"fba9dede5f27731c9771645a39863328"</ETag>
            <Size>434234</Size>
            <Owner>
                <ID>125664</ID>
                <DisplayName>username</DisplayName>
            </Owner>
        </Version>
        <DeleteMarker>
            <Key>my-second-image.jpg</Key>
            <VersionId>55566666</VersionId>
            <IsLatest>true</IsLatest>
            <LastModified>2019-08-16T17:50:31.000Z</LastModified>
            <Owner>
                <ID>125664</ID>
                <DisplayName>username</DisplayName>
            </Owner>
        </DeleteMarker>
        <Version>
            <Key>my-second-image.jpg</Key>
            <VersionId>45667</VersionId>
            <IsLatest>false</IsLatest>
            <LastModified>2019-08-16T17:50:30.000Z</LastModified>
            <ETag>"9b2cf535f27731c974343645a3985328"</ETag>
            <Size>166434</Size>
            <Owner>
                <ID>125664</ID>
                <DisplayName>username</DisplayName>
            </Owner>
        </Version>
    </ListVersionsResult>
    ```

- 携带分隔符/进行查询

    ```lang-rest
    GET /mybucket-2?versions&delimiter=/ HTTP/1.1
    Host: ip:port
    Date: date
    Authorization: authorization string
    ```
    
    响应结果如下：
    
    ```lang-xml
    <ListVersionsResult>
      <Name>mvbucketwithversionon1</Name>
      <Prefix/>
      <KeyMarker/>
      <VersionIdMarker/>
      <MaxKeys>1000</MaxKeys>
      <Delimiter>/</Delimiter>
      <IsTruncated>false</IsTruncated>
      <Version>
        <Key>Sample.jpg</Key>
        <VersionId>toxMzQlBsGyGCz1YuMWMp90cdXLzqOCH</VersionId>
        <IsLatest>true</IsLatest>
        <LastModified>2019-02-02T18:46:20.000Z</LastModified>
        <ETag>"3305f2cfc46c0f04559748bb039d69ae"</ETag>
        <Size>3191</Size>
        <Owner>
            <ID>125664</ID>
            <DisplayName>username</DisplayName>
        </Owner>
      </Version>
      <CommonPrefixes>
        <Prefix>photos/</Prefix>
      </CommonPrefixes>
      <CommonPrefixes>
        <Prefix>videos/</Prefix>
      </CommonPrefixes>
    </ListVersionsResult>
    ```

### List Multipart Uploads

查询桶内所有已初始化未完成的分段上传请求

**请求语法**

```lang-rest
GET /bucketname?uploads HTTP/1.1
Host: ip:port
Date: Date
Authorization: authorization string
```

**请求参数**

| 参数 | 说明 |
| ---- | ---- |
| prefix | 前缀，类型为 string，返回具有前缀的对象列表 |
| delimiter | 分隔符，类型为 string，如果指定 prefix，则 prefix 后第一次出现的分隔符之间包含相同字符串的所有键都被分组在一个 CommonPrefixes；如果未指定 prefix 参数，则子字符串从对象名称的开头开始 |
| key-marker | 指定在存储桶中列出对象要开始的键，类型为 string，返回对象键按照 UTF-8 二进制顺序从该标记后的键开始按顺序排列 |
| upload-id-marker | 指定起始位置的 uploadId，仅在指定了 key-marker 的情况下有效 |
| max-uploads | 设置响应中返回的最大键数，类型为 string，默认值 1000，如果要查询返回数量少于 1000，可以填写其他值，填写超过 1000 的值，仍然按照 1000 条返回 |
| encoding-type | 对响应内容进行的编码方法，只支持 url，由于对象名称可以包含任意字符，但是 XML 对某些特别的字符无法解析，所以需要对响应中的对象名称进行编码 |

**结果解析**

查询结果在响应消息体中以 XML 形式体现。

| 元素 | 说明 |
| ---- | ---- |
| ListMultipartUploadsResult | 包含桶信息、查询条件和未完成的分段上传信息 |
| Bucket | 存储桶名称 |
| Prefix | 查询的 prefix 条件 |
| Delimiter | 查询的 delimiter 条件 |
| KeyMarker | 查询的 key-marker 条件 |
| UploadIdMarker | 查询的 upload-id-marker 条件 |
| MaxUploads | 查询的 maxUploads 条件 |
| Encoding-Type | 查询的 encoding-type 条件 |
| IsTruncated | 如果该字段为 true，说明由于条数限制，本次没有查询完所有符合条件的结果，可以使用 NextMarker 作为下一次查询的 Marker 条件继续查询剩余内容 |
| NextKeyMarker | 当 IsTruncated 为 true 时，NextKeyMarker 记录本次返回的最后一个对象或者 CommonPrefix |
| NextUploadIdMarker | 当 IsTruncated 为 true 时，NextUploadIdMarker 记录本次返回的最后一条记录的 uploadId |
| CommonPrefixes | 当查询条件指定了 Delimter 时，Prefix 后面第一次出现 Delimiter 的位置（包括 Delimiter）之前的内容作为 CommonPrefix，当有多个对象具有相同的 CommonPrefix 时，只返回一条 CommonPrefix，计数一次，对象信息不返回 |
| Prefix | CommonPrefix 包含的前缀，属于 ListMultipartUploadsResult.CommonPrefixes |
| Upload | 包含分段上传信息 |
| Key | 对象的名称，属于 ListMultipartUploadsResult.Upload | 
| UploadId | 分段上传的 uploadId，属于 ListMultipartUploadsResult.Upload |
| Initiated | 分段上传的的初始化时间，属于 ListMultipartUploadsResult.Upload | 
| Owner | 存储桶的所有者，属于 ListMultipartUploadsResult.Upload |
| ID | 存储桶所有者的 ID，属于 ListMultipartUploadsResult.Upload.Owner | ListMultipartUploadsResult.Upload.Initiated |
| DisplayName | 桶所有者的名字，属于 ListMultipartUploadsResult.Upload.Owner |ListMultipartUploadsResult.Upload.Initiated |

**示例**

查询 uploads，携带分隔符/

```lang-rest
GET /example-bucket?uploads&delimiter=/ HTTP/1.1
Host: ip:port
Date: Sat, 17 Aug 2019 20:34:56 GMT
Authorization: authorization string
```

查询结果如下：

```lang-xml
<ListMultipartUploadsResult>
  <Bucket>example-bucket</Bucket>
  <KeyMarker/>
  <UploadIdMarker/>
  <NextKeyMarker>sample.jpg</NextKeyMarker>
  <NextUploadIdMarker>4444</NextUploadIdMarker>
  <Delimiter>/</Delimiter>
  <Prefix/>
  <MaxUploads>1000</MaxUploads>
  <IsTruncated>false</IsTruncated>
  <Upload>
    <Key>sample.jpg</Key>
    <UploadId>4444</UploadId>
    <Initiator>
      <ID>2234</ID>
      <DisplayName>s3-nickname</DisplayName>
    </Initiator>
    <Owner>
      <ID>2234</ID>
      <DisplayName>s3-nickname</DisplayName>
    </Owner>
    <Initiated>2019-08-16T19:24:17.000Z</Initiated>
  </Upload>
  <CommonPrefixes>
    <Prefix>photos/</Prefix>
  </CommonPrefixes>
  <CommonPrefixes>
    <Prefix>videos/</Prefix>
  </CommonPrefixes>
</ListMultipartUploadsResult>
```

## 对象API

下述将介绍对象相关的接口。

### PUT Object

上传一个对象到桶中，如果已有则覆盖

> **Note:**
>
> 当开启了版本控制，同一个名称的对象可以在系统中保留多个版本。系统会为每次上传的对象生成一个 version ID，并保留每个版本。

**请求语法**

```lang-rest
PUT /bucketname/ObjectName HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**请求头部**

| 头域 | 说明 |
| ---- | ---- |
| Cache-Control | 指定请求/响应链中的缓存属性 |
| Content-Disposition | 当获取对象时，该属性提示将对象保存为的文件名 |
| Content-Encoding | 对象的附加编码类型，例如压缩文档使用的 gzip 类型 |
| Content-MD5 | 对象内容（不包含头部）的 MD5 值经过 BASE64 编码后得到字符串，服务端收到对象后也会做同样的计算，比较 Content-MD5 和服务端计算得出的结果，可以防止上传的对象内容被篡改或不完整 |
| Content-Type | 请求内容的 MIME 类型 |
| Expect | 当 expect 设置为 100-continue，发送 put object 的请求时并不立刻发送对象内容，而是等收到 100 临时响应或等待超时再发送 |
| Expires | 缓存的超时时间 |
| x-amz-meta- | 自定义元数据 |

**结果解析**

响应信息通过 header 返回。

| 头域 | 说明 |
| ---- | ---- |
| ETag | 对象内容的 MD5 值转换为 16 进制之后生成的字符串 |
| x-amz-version-id | 版本号，当版本控制状态为 Enabled 时，该字段返回此次上传对象的版本号；当版本控制状态为 Suspended 时，该字段返回 null；当未开启或禁用版本控制，该字段不返回 | 

**示例**

上传一个对象

```lang-rest
PUT /bucketname/my-image.jpg HTTP/1.1
Host: ip:port
Date: Sat, 17 Aug 2019 17:50:00 GMT
Authorization: authorization string
Content-Type: text/plain
Content-Length: 11434
Expect: 100-continue
[11434 bytes of object data]
```

响应结果如下：

```lang-rest
HTTP/1.1 100 Continue

HTTP/1.1 200 OK
Date: Sat, 17 Aug 2019 17:50:00 GMT
ETag: "1b2cf535f27731c974343645a3985328"
Content-Length: 0
```

### PUT Object - Copy

从系统中已有的对象拷贝到目标对象

> **Note:**
>
> 该操作不需要从本地上传对象内容。

**请求语法**

```lang-rest
PUT /destinationbucket/destinationObject HTTP/1.1
Host: ip:port
x-amz-copy-source: /source_bucket/sourceObject
x-amz-metadata-directive: metadata_directive
x-amz-copy-source-if-match: etag
x-amz-copy-source-if-none-match: etag
x-amz-copy-source-if-unmodified-since: time_stamp
x-amz-copy-source-if-modified-since: time_stamp
<request metadata>
Authorization: authorization string
```
**请求头部**

| 头域 | 说明 |
| ---- | ---- |
| x-amz-copy-source | 必须携带的头部，复制对象的源对象地址，包含源存储桶和源对象，例如：`/source_bucket/sourceObject` ，默认复制源对象的最新版本；如果要指定版本复制，则需要增加版本号，例如：`/source_bucket/sourceObject?versionId=3344` |
|  x-amz-metadata-directive | 指定是否从源对象复制元数据到目标对象，取值包括"COPY"和"REPLACE"，默认值为"COPY" <br> 当指定为"COPY"时，从源对象复制元数据到目标对象；当指定为"REPLACE"时，源对象的元数据都不会复制到目标对象，目标对象使用复制对象请求中携带的元数据 |
| x-amz-copy-if-modified-since | 时间，只有当源对象的创建时间在此时间后才进行复制 |
| x-amz-copy-if-unmodified-since | 时间，只有当源对象的创建时间在此之前才进行复制 |
| x-amz-copy-if-match | ETag，只有当源对象的 ETag 与此 ETag 匹配才进行复制 |
| x-amz-copy-if-none-match | ETag，只有当源对象的 ETag 与此 ETag 不匹配才进行复制 |
| Cache-Control | 指定请求/响应链中的缓存属性 |
| Content-Disposition | 当获取对象时，该属性提示将对象保存为的文件名 |
| Content-Encoding | 对象的附加编码类型，例如压缩文档使用的 gzip 类型 |
| Content-MD5 | 对象内容（不包含头部）计算 MD5 值后经过 BASE64 编码得到字符串，服务端收到对象后也会做同样的计算，比较 Content-MD5 和服务端计算得出的结果是否一致，可以防止上传的对象内容被篡改或不完整 |
| Expires | 缓存的超时时间 |
| x-amz-meta- | 自定义元数据 |

**结果解析**

版本号在响应 header 中体现。

| 头域 | 说明 |
| ---- | ---- |
| x-amz-version-id | 复制后生成对象的版本号 |
| x-amz-copy-source-version-id | 源对象的版本号，复制后对象的 ETag 和创建时间在消息体中以 XML 形式体现 |
| CopyObjectResult | 包含 ETag 和 LastModified |
| ETag | 新对象内容计算 MD5 值后转换为 16 进制得到的字符串，与源对象一致 |
| LastModified | 新对象的创建时间 |

**示例**

复制指定的版本

```lang-rest
PUT /bucketname/my-second-image.jpg HTTP/1.1
Host: ip:port
Date: Sat, 17 Aug 2019 17:50:00 GMT
x-amz-copy-source: /bucketname/my-image.jpg?versionId=3344
Authorization: authorization string
```

响应结果如下：

```lang-rest
HTTP/1.1 200 OK
x-amz-version-id:5656
x-amz-copy-source-version-id:3344
Date: Sat, 17 Aug 2019 17:50:00 GMT

<CopyObjectResult>
   <LastModified>2019-08-17T17:50:00</LastModified>
   <ETag>"9b2cf535f27731c974343645a3985328"</ETag>
</CopyObjectResult>
```

### GET Object

获取对象内容

**请求语法**

```lang-rest
GET /bucketname/ObjectName HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| versionId | 获取指定版本的对象时通过此参数指定版本号 |
| response-content-type | 指定响应消息中的 Content-Type 头部值 | 
| response-content-language | 指定响应消息中的 Content-Language 头部值 |
| response-expires | 指定响应消息中的 Expires 头部值 |
| response-cache-control | 指定响应消息中的 Cache-Control 头部值 |
| response-content-disposition | 指定响应消息中的 Content-Disposition 头部值 |
| response-content-encoding | 指定响应消息中的 Content-Encoding 头部值 |

**请求头部**

| 头域 | 说明 |
| ---- | ---- |
| Range | 下载指定位置的字节数 |
| If-Modified-Since | 指定时间，只有在指定时间之后更新过，才返回对象，否则返回 304 |
| If-Unmodified-Since | 指定时间，只有在指定时间之前未更新，才返回对象，否则返回 412 | 
| If-Match | 指定 ETag，只有对象的 ETag 和 ETag 匹配，才返回对象，否则返回 412 |
| If-None-Match | 指定 ETag，只有对象的 ETag 和 ETag 不匹配，才返回对象，否则返回 304 |

**结果解析**

响应信息通过 header 返回。

| 头域 | 说明 |
| ---- | ---- |
| x-amz-version-id | 获取的对象的版本号 |
| x-amz-meta- | 对象的自定义元数据，与上传对象时的设置一致 |
| x-amz-delete-marker | 当获取的对象是一个删除标记，响应中会携带该头部且值为 true；当获取的对象不是删除标记，则不会携带该头部 |

**示例**

- 获取一个对象

    ```lang-rest
    GET /bucketname/ObjectName HTTP/1.1
    Host: ip:port
    Date: Sat, 17 Aug 2019 17:50:00 GMT
    Authorization: authorization string
    ```
    
    响应结果如下：
    
    ```lang-rest
    HTTP/1.1 200 OK
    Date: Sat, 17 Aug 2019 17:50:00 GMT
    Last-Modified: Sat, 17 Aug 2019 17:40:00 GMT
    ETag: "fba9dede5f27731c9771645a39863328"
    Content-Length: 434234
    
    [434234 bytes of object data]
    ```

- 指定版本号获取一个对象

    ```lang-rest
    GET /bucketname/myObject?versionId=4433 HTTP/1.1
    Host: ip:port
    Date: Sat, 17 Aug 2019 17:50:00 GMT
    Authorization: authorization string
    ```
    
    响应结果如下：
    
    ```lang-rest
    HTTP/1.1 200 OK
    Date: Sat, 17 Aug 2019 17:50:00 GMT
    Last-Modified: Sat, 17 Aug 2019 17:40:00 GMT
    x-amz-version-id: 4433
    ETag: "fba9dede5f27731c9771645a39863328"
    Content-Length: 434234
    Content-Type: text/plain
    
    [434234 bytes of object data]
    ```

### HEAD Object

获取对象的元数据信息，不获取对象内容 

**请求语法**

```lang-rest
HEAD /bucketname/ObjectName HTTP/1.1
Host: ip:port
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| versionId | 获取指定版本的对象时通过此参数指定版本号 |

**请求头部**

| 头域 | 说明 |
| ---- | ---- |
| Range | 下载指定位置的字节数 |
| If-Modified-Since | 指定时间，只有在指定时间之后更新才返回对象，否则返回 304 |
| If-Unmodified-Since | 指定时间，只有在指定时间之前未更新才返回对象，否则返回 412 |
| If-Match | 指定 ETag，只有对象的 ETag 和 ETag 匹配才返回对象，否则返回 412 |
| If-None-Match | 指定 ETag，只有对象的 ETag 和 ETag 不匹配，才返回对象，否则返回 304 |

**结果解析**

响应信息通过 header 返回。

| 头域 | 说明 |
| ---- | ---- |
| x-amz-version-id | 获取的对象的版本号 | 
| x-amz-meta- | 对象的自定义元数据，与上传对象时的设置一致 |

**示例**

- 获取对象的元数据

    ```lang-rest
    HEAD /bucketname/my-image.jpg HTTP/1.1
    Host: ip:port
    Date: Sat, 17 Aug 2019 17:50:00 GMT
    Authorization: authorization string
    ```
    
    响应结果如下：
    
    ```lang-rest
    HTTP/1.1 200 OK
    x-amz-version-id: 3344
    Date: Sat, 17 Aug 2019 17:50:00 GMT
    Last-Modified: Sat, 17 Aug 2019 17:40:00 GMT
    ETag: "fba9dede5f27731c9771645a39863328"
    Content-Length: 434234
    Content-Type: text/plain
    ```

- 获取指定版本对象的元数据

    ```lang-rest
    HEAD /bucketname/my-image.jpg?versionId=3344 HTTP/1.1
    Host: ip:port
    Date: Sat, 17 Aug 2019 17:55:00 GMT
    Authorization: authorization string
    ```
    
    响应结果如下：
    
    ```lang-rest
    HTTP/1.1 200 OK
    x-amz-version-id: 3344
    Date: Sat, 17 Aug 2019 17:55:00 GMT
    Last-Modified: Sat, 17 Aug 2019 17:40:00 GMT
    ETag: "fba9dede5f27731c9771645a39863328"
    Content-Length: 434234
    Content-Type: text/plain
    ```

### DELETE Object

删除对象

> **Note:**
>
> 当用户打开了版本控制，删除对象时会生成一个删除标记，原来的对象还保存在系统中。如果用户需要永久删除对应版本，可以指定版本号进行删除。

**请求语法**

```lang-rest
DELETE /bucketname/ObjectName HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| versionId | 指定版本号，用于删除指定版本的对象 |

**结果解析**

响应信息通过 header 返回。

| 头域 | 说明 |
| ---- | ---- |
| x-amz-delete-marker | 1. 当删除操作生成一个删除标记时，会返回该头部且值为 true <br> 2. 当通过指定版本号删除对象时，如果删除的是一个删除标记，则会返回该头部且值为 true |
| x-amz-version-id | 1. 当删除操作生成一个删除标记时，该头部记录删除标记的版本号 <br> 2. 当通过指定版本号删除对象时，该头部记录被删除的版本号 |

**示例**

- 删除一个未开启版本控制的桶内的对象

    ```lang-rest
    DELETE /bucketname/my-second-image.jpg HTTP/1.1
    Host: ip:port
    Date: Sat, 17 Aug 2019 17:55:00 GMT
    Authorization: authorization string
    Content-Type: text/plain
    ```
    
    响应结果如下：
    
    ```lang-rest
    HTTP/1.1 204 NoContent
    Date: Sat, 17 Aug 2019 17:55:00 GMT
    Content-Length: 0
    ```

- 删除指定版本对象

    ```lang-rest
    DELETE /bucketname/my-third-image.jpg?versionId=4455 HTTP/1.1
    Host: ip:port
    Date: Sat, 17 Aug 2019 17:58:00 GMT
    Authorization: authorization string
    ```
    
    响应结果如下：
    
    ```lang-rest
    HTTP/1.1 204 NoContent
    x-amz-version-id: 4455
    Date: Sat, 17 Aug 2019 17:58:00 GMT
    Content-Length: 0
    ```

### DELETE Objects

删除多个对象

**请求语法**

```lang-rest
POST /bucketname?delete HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string

<Delete>
   <Object>
      <Key>string</Key>
      <VersionId>string</VersionId>
   </Object>
   <Quiet>boolean</Quiet>
</Delete>
```

**请求元素**



用户需要在请求消息体中使用 XML 形式指定待删除的对象列表，系统对每个待删除的对象都会执行单独的删除操作，并将删除结果返回，单个对象的删除操作参考 DELETE object。

| 元素 | 说明 |
| ---- | ---- |
| Delete| 包含 Object 和 Quiet |
| Object | 待删除的对象，包含 Key 和 VersionId |
| Key | 对象名称 |
| VersionId | 对象版本号，可选 |
| Quiet | 静默标志，boolean类型，当请求中包含该字段且值为true时，返回结果中仅包含删除失败的对象的结果。默认不携带此标志，返回结果中包含全部删除对象的操作结果。 |

**结果解析**

响应消息体中返回 XML 形式的结果，包含各个对象的删除结果。

| 元素 | 说明 |
| ---- | ---- |
| DeleteResult | 包含 Deleted 和 Error |
| Deleted | 删除成功的记录，包含 Key，VersionId，DeleteMarker 和 DeleteMarkerVersionId |
| Key | 已删除的对象名称 |
| VersionId | 指定删除的对象版本号 |
| DeleteMarker | 当删除操作产生了 DeleteMarker 或指定版本号删除的对象是一个DeleteMarker 时，该字段为true。  | 
| DeleteMarkerVersionId | 当删除操作产生了 DeleteMarker 或指定版本号删除的对象是一个DeleteMarker 时，该字段为新产生或指定的 VersionId |
| Error | 删除失败的记录，包括Code, Key, Message, VersionId |
| Code | 错误码 | 
| Key | 删除失败的对象名称 | 
| Message | 失败的描述 |
| VersionId | 指定删除的对象版本号 |

**示例**

- 在未开启版本控制的桶内删除多个对象

   ```lang-rest
   POST /bucketname?delete HTTP/1.1
   Host: ip:port
   Date: date
   Authorization: authorization string

   <Delete>
      <Object>
         <Key>key1</Key>
      </Object>
      <Object>
         <Key>key2</Key>
      </Object>
   </Delete>  
   ```

   删除成功，响应结果如下：

   ```lang-rest
   <DeleteResult>
      <Deleted>
         <Key>key1</Key>
      </Deleted>
      <Deleted>
         <Key>key2</Key>
      </Deleted>
   </DeleteResult>
   ```

- 在开启版本控制的桶内删除多个对象

   ```lang-rest
   POST /bucketname?delete HTTP/1.1
   Host: ip:port
   Date: date
   Authorization: authorization string

   <Delete>
      <Object>
         <Key>key1</Key>
      </Object>
      <Object>
         <Key>key2</Key>
      </Object>
   </Delete>  
   ```

   生成DeleteMarker，响应结果如下：

   ```lang-rest
   <DeleteResult>
      <Deleted>
         <Key>key1</Key>
         <DeleteMarker>true</DeleteMarker>
         <DeleteMarkerVersionId>1</DeleteMarkerVersionId>
      </Deleted>
      <Deleted>
         <Key>key2</Key>
         <DeleteMarker>true</DeleteMarker>
         <DeleteMarkerVersionId>1</DeleteMarkerVersionId>
      </Deleted>
   </DeleteResult>
   ```
- 指定版本删除多个对象

   ```lang-rest
   POST /bucketname?delete HTTP/1.1
   Host: ip:port
   Date: date
   Authorization: authorization string

   <Delete>
      <Object>
         <Key>key1</Key>
         <VersionId>0</VersionId>
      </Object>
      <Object>
         <Key>key2</Key>
         <VersionId>0</VersionId>
      </Object>
      <Object>
         <Key>key1</Key>
         <VersionId>1</VersionId>
      </Object>
   </Delete> 
   ```

   删除成功，响应结果如下：

   ```lang-rest
   <DeleteResult>
      <Deleted>
         <Key>key1</Key>
         <VersionId>0</VersionId>
      </Deleted>
      <Deleted>
         <Key>key2</Key>
         <VersionId>0</VersionId>
      </Deleted>
      <Deleted>
         <Key>key1</Key>
         <VersionId>1</VersionId>
         <DeleteMarker>true</DeleteMarker>
         <DeleteMarkerVersionId>1</DeleteMarkerVersionId>
      </Deleted>
   </DeleteResult>
   ```


### Initiate Multipart Upload

初始化分段上传，获得 upload ID

**请求语法**

```lang-rest
POST /bucketname/ObjectName?uploads HTTP/1.1
Host: ip:port
Date: date
Authorization: authorization string
```

**请求头部**

初始化时携带的元数据，在合并分段上传生成一个完整对象时作为对象的元数据。

| 头域 | 说明 |
| ---- | ---- |
| Cache-Control | 指定请求/响应链中的缓存属性 |
| Content-Disposition | 当获取对象时，该属性提示将对象保存为的文件名 |
| Content-Encoding | 对象的附加编码类型，例如压缩文档使用的 gzip 类型 |
| Content-Type | 请求内容的 MIME 类型 |
| Expires | 缓存的超时时间 |
| x-amz-meta- | 自定义元数据 |

**结果解析**

响应消息体中返回 XML 形式的结果，包含 upload ID。

| 元素 | 说明 |
| ---- | ---- |
| InitiateMultipartUploadResult | 包含初始化分段的结果 |
| Bucket | 初始化分段上传对象所在存储桶 |
| Key | 初始化分段上传的对象名称 |
| UploadId | 初始化分段上传的 ID，用来唯一标识一个分段上传请求 | 

**示例**

初始化分段上传后，响应结果如下：

```lang-xml
HTTP/1.1 200 OK
Date: Sat, 17 Aug 2019 17:59:00 GMT
Content-Length: 151

<InitiateMultipartUploadResult>
  <Bucket>bucketname</Bucket>
  <Key>ObjectName</Key>
  <UploadId>56778</UploadId>
</InitiateMultipartUploadResult>
```

### Upload Part

上传分段

**请求语法**

```lang-rest
PUT /bucketname/ObjectName?partNumber=PartNumber&uploadId=UploadId HTTP/1.1
Host: ip:port
Date: date
Content-Length: Size
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| partNumber | 分段编号，有效范围为 1~10000 |
| uploadId | upload ID，可以在 Initiate Multipart Upload 获得 |

**示例**

上传一个分段

```lang-rest
PUT /bucketname/ObjectName?partNumber=1&uploadId=56778 HTTP/1.1
Host: ip:port
Date: Sat, 17 Aug 2019 18:05:00 GMT
Content-Length: 10485760
Content-MD5: pUNXr/BjKK5G2UKvaRRrOA==
Authorization: authorization string

[10485760 bytes of object data]
```

响应结果如下：

```lang-rest
HTTP/1.1 200 OK
Date: Sat, 17 Aug 2019 18:05:00 GMT
ETag: "b54357faf0632cce46e942fa68356b38"
Content-Length: 0
```

### List Parts

查询分段列表

**请求语法**

```lang-rest
GET /bucketname/ObjectName?uploadId=UploadId HTTP/1.1
Host: ip:port
Date: Date
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| uploadId | upload ID，可以在 Initiate Multipart Upload 获得 |
| max-parts | 一次返回的最大分段数 |
| part-number?-marker | 查询的起始位置 |
| encoding-type | 响应结果编码类型，只支持 url；由于对象名称可以包含任意字符，但是 XML 对某些特别的字符无法解析，所以需要对响应中的对象名称进行编码 | 

**结果解析**

查询结果以 XML 形式返回。

| 元素 | 说明 |
| ---- | ---- |
| ListPartsResult | 包含查询分段列表结果 |
| Bucket | 分段上传的存储桶名称 |
| Key | 分段上传的对象名称 |
| UploadId | 分段上传请求的 upload ID |
| Initiator | 分段上传的发起者 |
| Owner | 存储桶的拥有者 |
| DisplayName | 用户的名称 |
| ID | 用户 ID | 
| PartNumberMarker | 查询的 part-number?-marker 条件 |
| MaxParts | 查询的 max-parts 条件 |
| IsTruncated | 是否被截断 | 
| NextPartNumberMarker | 当 IsTruncated 为 true 时，该字段记录下一次查询的起始位置 |
| Encoding-Type | 查询的 encoding-type 条件 |
| Part | 包含分段内容 |
| PartNumber | 分段编号 | 
| LastModified | 分段的最新修改时间 |
| ETag | 分段的 ETag |
| Size | 分段的大小 |

**示例**

查询 upload ID 为 56778 的分段列表，指定 max-parts 为 2，part-number-marker 为 1

```lang-rest
GET /bucketname/ObjectName?uploadId=56778&max-parts=2&part-number-marker=1 HTTP/1.1
Host: ip:port
Date: Sat, 17 Aug 2019 18:10:00 GMT
Authorization: authorization string
```

响应结果如下：

```lang-xml
HTTP/1.1 200 OK
Date: Sat, 17 Aug 2019 18:10:00 GMT
Content-Length: 838

<ListPartsResult>
  <Bucket>bucketname</Bucket>
  <Key>ObjectName</Key>
  <UploadId>56778</UploadId>
  <Initiator>
    <DisplayName>username</DisplayName>
    <ID>34455</ID>
  </Initiator>
  <Owner>
    <DisplayName>username</DisplayName>
    <ID>34455</ID>
  </Owner>
  <PartNumberMarker>1</PartNumberMarker>
  <NextPartNumberMarker>3</NextPartNumberMarker>
  <MaxParts>2</MaxParts>
  <IsTruncated>true</IsTruncated>
  <Part>
    <PartNumber>2</PartNumber>
    <LastModified>2019-08-1T17:06:06.000Z</LastModified>
    <ETag>"7778aef83f66abc1fa1e8477f296d394"</ETag>
    <Size>10485760</Size>
  </Part>
  <Part>
    <PartNumber>3</PartNumber>
    <LastModified>2019-08-1T17:06:23.000Z</LastModified>
    <ETag>"aaaa18db4cc2f85cedef654fccc4a4x8"</ETag>
    <Size>10485760</Size>
  </Part>
</ListPartsResult>
```

### Complete Multipart Upload

完成分段上传，合并分段

**请求语法**

```lang-rest
POST /bucketname/ObjectName?uploadId=UploadId HTTP/1.1
Host: ip:port
Date: Date
Content-Length: Size
Authorization: authorization string

<CompleteMultipartUpload>
  <Part>
    <PartNumber>PartNumber</PartNumber>
    <ETag>ETag</ETag>
  </Part>
  ...
</CompleteMultipartUpload>
```

**请求元素**

| 元素 | 说明 |
| ---- | ---- |
| CompleteMultipartUpload | 包含所有要合并的的分段信息 |
| Part | 一个分段 |
| PartNumber | 分段编码 |
| ETag | 分段的 ETag |

**结果解析**

响应消息体中包含 XML 形式的合并结果，包含合并后对象的 ETag。

| 元素 | 说明 |
| ---- | ---- |
| CompleteMultipartUploadResult | 合并分段结果 |
| Location | 合并后对象的地址 |
| Bucket | 存储桶名称 |
| Key | 对象名称 |
| ETag | 合并后对象的 ETag，不一定是合并后完整对象的 MD5 值 |

**示例**

完成分段上传，合并分段

```lang-rest
POST /bucketname/ObjectName?uploadId=56778 HTTP/1.1
Host: ip:port
Date: Sat, 17 Aug 2019 18:10:30 GMT
Content-Length: 391
Authorization: authorization string

<CompleteMultipartUpload>
  <Part>
    <PartNumber>1</PartNumber>
    <ETag>"a54357aff0632cce46d942af68356b38"</ETag>
  </Part>
  <Part>
    <PartNumber>2</PartNumber>
    <ETag>"0c78aef83f66abc1fa1e8477f296d394"</ETag>
  </Part>
  <Part>
    <PartNumber>3</PartNumber>
    <ETag>"acbd18db4cc2f85cedef654fccc4a4d8"</ETag>
  </Part>
</CompleteMultipartUpload>
```

响应结果如下：

```lang-xml
HTTP/1.1 200 OK
Date: Sat, 17 Aug 2019 18:10:30 GMT

<CompleteMultipartUploadResult>
  <Location>http://ip:port/bucketname/ObjectName</Location>
  <Bucket>bucketname</Bucket>
  <Key>ObjectName</Key>
  <ETag>"3858f62230ac3c915f300c664312c11f-9"</ETag>
</CompleteMultipartUploadResult>
```

### Abort Multipart Upload

取消分段上传

**请求语法**

```lang-rest
DELETE /bucketname/ObjectName?uploadId=UploadId HTTP/1.1
Host: ip:port
Date: Date
Authorization: authorization string
```

**请求参数**

| 参数名 | 说明 |
| ----   | ---- |
| uploadId | 待取消的 upload ID |

**示例**

取消分段上传响应结果如下：

```lang-rest
HTTP/1.1 204 OK
Date: Sat, 17 Aug 2019 18:15:30 GMT
Content-Length: 0
```

## 区域API

下述将介绍区域相关的接口。

### Create Region

增加一个区域或更新区域配置

> **Note:**
>
> 用户创建区域时可对区域内数据存储位置进行配置，即指定集合空间的生成方式，生成方式分为指定模式和自动创建模式。不能够同时指定两种模式，更新区域配置是也不能修改模式，更新区域配置只能够修改自动创建模式下的集合空间生成规则。

**请求语法**

```lang-rest
POST /region/?Action=CreateRegion&RegionName={regionname} HTTP/1.1
Host: ip:port
Content-Length: length
Date: date
Authorization: authorization string

<RegionConfiguration>
  <DataCSShardingType>year</DataCSShardingType>
  <DataCLShardingType>month</DataCLShardingType>
  <DataDomain>domain1</DataDomain>
  <MetaDomain>domain2</MetaDomain>
</RegionConfiguration>
```

**参数说明**

| 参数名 | 说明 |
| ----   | ---- |
| Action | 固定为 CreateRegion，表示该操作为创建一个区域 |
| RegionName | 指定区域名称 |

**请求元素**

用户可以在请求消息体中使用 XML 形式指定区域的配置。

| 元素 | 说明 |
| ---- | ---- |
| RegionConfiguration | 包含区域配置内容 |
| DataCSShardingType | 对象数据集合空间的生成规则，按照设定的时间生成指定的集合空间，类型为 string，默认值为"year"，有效值包括"year"、"quarter"和"month" |
| DataCLShardingType | 对象数据集合的生成规则，按照设定的时间生成指定的集合，类型为 string，默认值为"quarter"，有效值包括"year"、"quarter"和"month" |
| DataCSRange | 对象数据集合的生成规则，在设定时间段内能够生成的集合空间数量，类型为 int32 |
| DataDomain | 对象数据集合空间所属域，类型为 string，域必须已在 SequoiaDB 中定义，如果不填写域名称，则对象数据集合空间建立在系统域上 |
| DataLobPageSize | 对象数据集合空间的 LobPageSize，默认值为 262144，有效值包括 0、4096、8192、16384、32768、65536、131072、262144、524288 之一，0 表示选择默认值|
| DataReplSize | 对象集合的ReplSize，写操作同步的副本数，默认值为 -1，有效值包括 -1、0、1~7 |
| MetaDomain | 元数据集合空间所属域，类型为 string，域必须已在 SequoiaDB 中定义，若不填写则元数据集合空间建在系统域上 |
| DataLocation | 指定模式为对象数据的集合空间.集合名称，类型为 string，如 CS.CL |
| MetaLocation | 指定模式为元数据的集合空间.集合名称，类型为 string，如 CS.CL |
| MetaHisLocation | 指定模式为历史元数据的集合空间.集合名称，类型 string，如 CS.CL |

**示例**

创建区域的请求，指定对象数据集合空间和元数据集合空间的域，指定对象数据集合空间和对象数据集合的生成规则

```lang-rest
POST /region/?Action=CreateRegion&RegionName=region1 HTTP/1.1
Host: ip:port
Content-Length: length
Date: date
Authorization: authorization string

<RegionConfiguration>
  <DataCSShardingType>year</DataCSShardingType>
  <DataCLShardingType>month</DataCLShardingType>
  <DataDomain>domain1</DataDomain>
  <MetaDomain>domain2</MetaDomain>
</RegionConfiguration>
```

响应结果如下：

```lang-rest
HTTP/1.1 200 OK
Date: date
Content-Length: 0
```

### GetRegion

获取一个区域的配置

**请求语法**

```lang-rest
POST /region/?Action=GetRegion&RegionName={regionname} HTTP/1.1 
Host: ip:port
Date: date 
Authorization: authorization string 
```

**参数说明**

| 参数名 | 说明 |
| ----   | ---- |
| Action | 固定为 GetRegion，表示该操作为删除一个区域 |
| RegionName | 指定区域名称 |

**示例**

查询一个区域的配置信息

```lang-rest
POST /region/?Action=GetRegion&RegionName=region1 HTTP/1.1
Host: ip:port
Date: Wed, 12 Oct 2009 17:50:00 GMT
Authorization: authorization string
```

响应结果如下：

```lang-xml
<RegionConfiguration>
  <Name>region1</Name>
  <DataCSShardingType>year</DataCSShardingType>
  <DataCLShardingType>month</DataCLShardingType>
  <DataCSRange>1</DataCSRange>
  <DataDomain>domain1</DataDomain>
  <MetaDomain>domain2</MetaDomain>
  <DataLobPageSize>262144</DataLobPageSize>
  <DataReplSize>-1</DataReplSize>
  <DataLocation/>
  <MetaLocation/>
  <MetaHisLocation/>
  <Buckets>
    <Bucket>bucketname1</Bucket>
    <Bucket>bucketname2</Bucket>
  </Buckets>
</RegionConfiguration>

```

### DeleteRegion

删除一个区域

**请求语法**

```lang-rest
POST /region/?Action=DeleteRegion&RegionName={regionname} HTTP/1.1
Host: ip:port
Date: date 
Authorization: authorization string
```

**参数说明**

| 参数名 | 说明 |
| ----   | ---- |
| Action | 固定为 DeleteRegion，表示该操作为删除一个区域 |
| RegionName | 指定区域名称 |

**示例**

删除一个区域的请求

```lang-rest
POST /region/?Action=DeleteRegion&RegionName=region1 HTTP/1.1
Host: ip:port
Date: date 
Authorization: authorization string
```

响应结果如下：

```lang-rest
HTTP/1.1 204 No Content 
Date: date 
```

### ListRegions

查询区域列表，可以查询当前系统中所有区域名称

**请求语法**

```lang-rest
POST /region/?Action=ListRegions HTTP/1.1 
Host: ip:port
Date: date 
Authorization: authorization string 
```

**参数说明**

| 参数名 | 说明 |
| ----   | ---- |
| Action | 固定为 ListRegions，表示该操作为查询区域列表 |

**结果解析**

查询结果以 XML 形式在响应消息头中显示。

| 元素 | 说明 |
| ---- | ---- |
| ListAllRegionsResult | 包含一个到多个 Region | 
| Region | 区域名称 |

**示例**

查询区域列表的响应结果如下：

```lang-rest
<ListAllRegionsResult>
  <Region>region1</Region>
  <Region>region1</Region>
  <Region>region1</Region>
</ListAllRegionsResult>
```

### HeadRegion

查询区域是否存在

**请求语法**

```lang-rest
POST /region/?Action=HeadRegion&RegionName={regionname} HTTP/1.1
Host: ip:port
Date: date 
Authorization: authorization string 
```

**示例**

响应结果如下：

```lang-rest
HTTP/1.1 200 OK
Date: date
```

## 用户API

下述将介绍用户相关的接口。

### Create User

创建用户

> **Note:**
>
> 该操作需管理员用户权限，系统默认生成一个管理员用户，可用该用户创建其他用户。

**请求语法**

```lang-rest
POST /users/?Action=CreateUser&UserName=username&Role=admin HTTP/1.1 
Host: ip:port
Date: date 
Authorization: authorization string
```

**参数说明**

| 参数名 | 说明 |
| ----   | ---- |
| Action | 固定为 CreateUser，表示该操作为创建一个用户
| UserName | 指定用户名称 | 
| Role | 指定用户的角色，可选管理员 admin 用户或普通 normal 用户，管理员用户可以管理用户也可以使用 S3 业务，普通用户可以使用 S3 业务但不能管理用户 |

**结果解析**

创建用户成功后，会收到 AccessKeys，AccessKeys 用于访问 SequoiaS3 系统的用户验证。

| 元素 | 说明 |
| ---- | ---- |
| AccessKeys | 包含用户 Key 值 |
| AccessKeyID | 用户的 Access Key ID，代表用户身份 | 
| SecretAccessKey | 新用户的 Secret Access Key 类似密码，用于计算签名，和 Access Key ID 一起在请求消息中携带，用来验证用户身份 |

**示例**

创建用户后，响应结果如下：

```lang-rest
HTTP/1.1 200 OK
Date: Wed, 01 Mar  2006 12:00:00 GMT

<AccessKeys>
  <AccessKeyID>AKIAIC6UQBTBIW7THT5A</AccessKeyID>
  <SecretAccessKey>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</SecretAccessKey>
</AccessKeys>
```

### Create AccessKey

更新用户的访问密钥

> **Note:**
>
> 该操作需要管理员用户权限。生成新的密钥之后，旧密钥失效。

**请求语法**

```lang-rest
POST /users/?Action=CreateAccessKey&UserName=username HTTP/1.1 
Host: ip:port
Date: date 
Authorization: authorization string
```

**参数说明**

| 参数名 | 说明 |
| ----   | ---- |
| Action | 固定为 CreateAccessKey，表示该操作为更新 AccessKeys |
| UserName | 指定用户名称 |

**示例**

更新密钥后，响应结果如下：

```lang-rest
HTTP/1.1 200 OK
Date: Wed, 01 Mar  2006 12:00:00 GMT

<AccessKeys>
  <AccessKeyID>AKIAIC6UQBCDGW7TH35T</AccessKeyID>
  <SecretAccessKey>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</SecretAccessKey>
</AccessKeys>
```

### Delete User

删除一个用户

> **Note:**
>
> - 该操作需要管理员用户权限。
> - 当该用户还有桶未清理时，不允许删除用户。
> - 当请求携带了 Force 标识时，可以强制删除未清理桶的用户，并将该用户拥有的桶以及桶内对象全部清理。

**请求语法**

```lang-rest
POST /users/?Action=DeleteUser&UserName=username HTTP/1.1 
Host: ip:port
Date: date 
Authorization: authorization string
```

**参数说明**

| 参数名 | 说明 |
| ----   | ---- |
| Action | 固定为 DeleteUser，表示该操作为删除用户 |
| UserName | 指定用户名称 |
| Force | 强制删除标记，当请求携带 Force 参数且值为 true 时删除用户，并清理该用户拥有的桶及桶内对象 |

**示例**

- 删除用户

    ```lang-rest
    POST /users/?Action=DeleteUser&UserName=user1 HTTP/1.1 
    Host: ip:port
    Date: date 
    Authorization: authorization string
    ```
    
    响应结果如下：
    
    ```lang-rest
    HTTP/1.1 200 OK
    Date: date
    ```

- 强制删除用户

    ```lang-rest
    POST/users/?Action=DeleteUser&UserName=username&Force=true HTTP/1.1 
    Host: ip:port
    Date: date 
    Authorization: authorization string
    ```
    
    响应结果如下：
    
    ```lang-rest
    HTTP/1.1 200 OK
    Date: date
    ```

### Get AccessKey

获取用户的访问密钥

> **Note:**
>
> 该操作需要管理员用户权限。

**请求语法**

```lang-rest
POST /users/?Action=GetAccessKey&UserName=username HTTP/1.1 
Host: ip:port
Date: date 
Authorization: authorization string
```

**参数说明**

| 参数名 | 说明 |
| ----   | ---- |
| Action | 固定为 GetAccessKey，表示该操作为获取用户的访问密钥 |
| UserName | 指定用户名称 |

**示例**

获取 user1 的访问密钥请求

```lang-rest
POST /users/?Action=GetAccessKey&UserName=user1 HTTP/1.1 
Host: ip:port
Date: date 
Authorization: authorization string
```

响应结果如下：

```lang-rest
HTTP/1.1 200 OK
Date: date

<AccessKeys>
  <AccessKeyID>AKIAIC6UQBTBIW7THT5A</AccessKeyID>
  <SecretAccessKey>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</SecretAccessKey>
</AccessKeys>
```



