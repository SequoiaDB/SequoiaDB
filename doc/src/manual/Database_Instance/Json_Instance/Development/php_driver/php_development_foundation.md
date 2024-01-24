本文档主要介绍如何使用 PHP 驱动接口编写使用 SequoiaDB 巨杉数据库的程序。完整的示例代码请参考 SequoiaDB 安装目录下的  `samples/PHP`

##连接数据库##

连接 SequoiaDB 数据库，之后即可访问、操作数据库。

  ```lang-php
  // 创建 SequoiaDB 对象
  $db = new SequoiaDB();
  // 连接数据库
  $arr = $db -> connect( "sdbserver:11810" );
  // 打印连接结果，返回的默认是 php 数组类型，返回值是 array(0){"errno"=>0}
  var_dump($arr);
  // 如果 errno 为 0，那么连接成功
  if( $arr['errno'] !=0 )
  {
     exit();
  }
  ```

##数据库操作##

* 选择集合空间

  ```lang-php
  // 选择集合空间 sample，如果不存在则自动创建
  // 返回 SequoiaCS 对象
  $cs = $db -> selectCS( "sample" );
  // 检验结果，如果成功返回对象，失败返回 NULL
  if( empty($cs) )
  {
     exit();
  }
  ```

* 选择集合

  ```lang-php
  // 选择集合 employee，如果不存在则自动创建
  // 返回 SequoiaCollection 对象
  $cl = $cs -> selectCollection( "employee" );
  // 检验结果，如果成功返回对象，失败返回 NULL
  if( empty($cl) )
  {
     exit();
  }
  ```

* 插入

  ```lang-php
  // json 方式插入
  $arr = $cl -> insert( '{"name":"Tom", "age":24}' );
  // 检测结果
  if( $arr['errno'] !=0 )
  {
     exit();
  }
  // 查看插入的结果
  var_dump($arr);

  // 数组方式插入
  $rec = array ( 'name' => 'Peter', 'age' => 22 ) ;
  $arr = $cl -> insert( $rec );
  // 检测结果
  if( $arr['errno'] !=0 )
  {
     exit();
  }
  // 查看插入的结果
  var_dump($arr);
  ```

* 查询

  ```lang-php
  // 查询集合中的所有记录
  // 返回 SequoiaCursor 对象
  $cursor = $cl -> find();
  // 遍历所有记录
  while( $record = $cursor -> getNext() )
  {
     var_dump( $record );
  }
  ```

* 更新

  ```lang-php
  // 更新集合中的所有记录
  $arr = $cl -> update( '{"$set":{"age":19}}' );
  // 检测结果
  if( $arr['errno'] !=0 )
  {
     exit();
  }
  // 查看更新的结果
  var_dump($arr);
  ```

* 删除

  ```lang-php
  // 删除集合中的所有记录
  $arr = $cl -> remove();
  // 检测结果
  if( $arr['errno'] !=0 )
  {
     exit();
  }
  // 查看删除的结果
  var_dump($arr);
  ```

## 集群操作##

集群操作主要涉及复制组与节点。如下以选择、启动复制组、获取节点为例

* 选择组

  ```lang-php
  // 选择名称为"group"的组，如果不存在则自动创建
  // 返回 SequoiaGroup 对象
  $group = $db -> selectGroup("db1");
  // 检验结果，如果成功返回对象，失败返回 NULL
  if( empty($group) )
  {
     exit();
  }
  ```

* 启动复制组

  ```lang-php
  // 启动复制组，首次会自动激活
  // 返回操作信息
  $arr = $group -> start();
  // 检查结果
  if( $arr['errno'] != 0 ) 
  {
     exit();
  }
  ```

* 获取节点

  ```lang-php
  // 获取名称为"node"的节点
  // 返回 SequoiaNode 对象
  $node = $group -> getNode( 'sdbserver:11820' );
  // 检查对象是否空
  if( empty( $node ) )
  {
     exit();
  }
  ```
