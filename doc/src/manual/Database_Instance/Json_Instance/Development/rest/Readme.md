
协调节点（coord）和数据节点（data）对外提供 REST 接口访问，同时支持 HTTP 和 HTTPS 协议。

##通用请求头##

| 字段           | 描述                                   | 示例                 |
| -------------- | -------------------------------------- | -------------------- |
| Content-Type   | 请求的数据类型                         | application/x-www-form-urlencoded;charset=UTF-8 |
| Content-Length | 请求的长度                             | 54                   |
| Host           | 主机名（协调节点或数据节点的 REST 服务地址） | 192.168.1.214:11814  |
| Accept         | 希望应答的数据类型，如果不指定该字段，默认响应 text/html （文本格式）的数据类型 | application/json |

```lang-http
POST / HTTP/1.0
Content-Type: application/x-www-form-urlencoded;charset=UTF-8
Accept: application/json
Content-Length: 54
Host: 192.168.1.214:11814
```

##通用响应头##

| 字段           | 描述           | 示例      |
| -------------- | -------------- | --------- |
| Content-Type   | 响应内容的类型，如果请求 Accept 为 application/json，<br>应答 Content-Type 为 application/json，否则应答 Content-Type 为 text/html | application/json |
| Content-Length | 响应内容的长度 | 35        |

```lang-http
HTTP/1.1 200 Ok
Content-Length: 35
Content-Type: application/json
```
