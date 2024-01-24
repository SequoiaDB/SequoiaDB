import( "rest.js" ) ;

/*
   address: om svc hostname or ip
   port:    om svc port
*/
var SdbOMCtrl = function( address, port ){
   this.rest = new SdbRest() ;
   this.session = '' ;
   this.addr = address ;
   this.port = port ;
   this.url = 'http://' + address + ':' + port + '/' ;
   this.request = new SdbRequest( "POST", this.url ) ;
   this.srcData = {} ;
} ;

SdbOMCtrl.prototype = {} ;
SdbOMCtrl.prototype.constructor = SdbOMCtrl;

SdbOMCtrl.prototype._checkResponse = function( arg, response ){
   if ( response.getCode() != 200 )
   {
      throw new Error( "rest access failed: " + response.getCode() ) ;
   }

   if ( response.getDataType() == 'json' )
   {
      var data = response.getData() ;
      if ( data[0]['errno'] != 0 )
      {
         var cmd = 'exec rest command' ;
         if( arg['cmd'] )
         {
            cmd = arg['cmd'] ;
         }
         else if ( arg['sql'] )
         {
            cmd = 'sql' ;
         }

         var detail = data[0]['detail'] ;
         if( !detail || detail.length == 0 )
         {
            detail = data[0]['description'] ;
         }

         throw new Error( _sprintf( "failed to ?: rc=?, detail: ?\nRequest cmd: ?", cmd, data[0]['errno'], detail, this.request.getRequestCmd() ) ) ;
      }

      if( data.length > 1 )
      {
         data.splice( 0, 1 ) ;
         return data ;
      }

      return [] ;
   }

   return response.getData() ;
}

SdbOMCtrl.prototype._setQueryPara = function( data, filter, selector, sort, hint, skip, limit ) {
   if( typeof( filter ) == 'object' )
   {
      data['Filter'] = filter ;
   }
   if( typeof( selector ) == 'object' )
   {
      data['Selector'] = selector ;
   }
   if( typeof( sort ) == 'object' )
   {
      data['Sort'] = sort ;
   }
   if( typeof( hint ) == 'object' )
   {
      data['Hint'] = hint ;
   }
   if( typeof( skip ) == 'number' )
   {
      data['Skip'] = skip ;
   }
   if( typeof( limit ) == 'number' )
   {
      data['Limit'] = limit ;
   }
   return data ;
}

function __sendAndCheckResult( omCtrl, data ){
   omCtrl.request.setHeader( "SdbClusterName", '' ) ;
   omCtrl.request.setHeader( "SdbBusinessName", '' ) ;

   omCtrl.request.setData( data ) ;

   var response = omCtrl.rest.ajax( omCtrl.request ) ;

   omCtrl._checkResponse( data, response ) ;
}

function __sendAndGetResult( omCtrl, data ){
   omCtrl.request.setHeader( "SdbClusterName", '' ) ;
   omCtrl.request.setHeader( "SdbBusinessName", '' ) ;

   return __sendAndGetResult3( omCtrl, data ) ;
}

function __sendAndGetResult2( omCtrl, data, filter, selector, sort, hint, skip, limit ){
   omCtrl.request.setHeader( "SdbClusterName", '' ) ;
   omCtrl.request.setHeader( "SdbBusinessName", '' ) ;

   data = omCtrl._setQueryPara( data, filter, selector, sort, hint, skip, limit ) ;

   return __sendAndGetResult3( omCtrl, data ) ;
}

function __sendAndGetResult3( omCtrl, data, url ){
   omCtrl.request.setData( data ) ;

   var response = omCtrl.rest.ajax( omCtrl.request, null, url ) ;

   var result = omCtrl._checkResponse( data, response ) ;

   return result ;
}

/******************************** OM操作 **************************************/

/*
   登录OM
*/
SdbOMCtrl.prototype.login = function( user, passwd ){

   var data = {
      'cmd': 'login',
      'user': user,
      'passwd': _md5( passwd ),
      'Timestamp': Date.parse( new Date() )
   } ;

   this.request.setData( data ) ;

   var response = this.rest.ajax( this.request ) ;

   this._checkResponse( data, response ) ;

   var session = response.getHeader( "sdbsessionid" ) ;

   if ( typeof( session ) != 'string' || session.length == 0 )
   {
      throw new Error( "invalid session id" ) ;
   }

   this.session = session ;

   this.request.setHeader( "SdbSessionID", session ) ;

   println( _sprintf( "login success, session: ?", this.session ) ) ;
}

/*
   修改OM密码
   user: 用户名
   passwd: 原密码
   newPasswd: 新密码
*/
SdbOMCtrl.prototype.change_passwd = function( user, passwd, newPasswd ){

   var data = {
      'cmd': 'change passwd',
      'User': user,
      'Passwd': _md5( passwd ),
      'Newpasswd': _md5( newPasswd ),
      'Timestamp': Date.parse( new Date() )
   } ;

   __sendAndCheckResult( this, data )
}

/*
   获取OM系统信息

   return:
      {
          "Version": {
              "Major": 3,
              "Minor": 0,
              "Fix": 0,
              "Release": 35887,
              "Build": "2018-06-15-00.39.14(Enterprise-Debug)"
          },
          "Edition": "Enterprise"
      }
*/
SdbOMCtrl.prototype.get_system_info = function(){

   var data = { 'cmd': 'get system info' } ;

   return __sendAndGetResult( this, data ) ;
}

/*
   列出插件列表

   return:
      {
          "BusinessType": "sequoiasql-postgresql",
          "Name": "SequoiaSQL",
          "ServiceName": "51036",
          "UpdateTime": {
              "$timestamp": "2018-06-20-16.34.56.000000"
          }
      },
      {
          "BusinessType": "sequoiasql-mysql",
          "Name": "SequoiaSQL",
          "ServiceName": "51036",
          "UpdateTime": {
              "$timestamp": "2018-06-20-16.34.56.000000"
          }
      }
*/
SdbOMCtrl.prototype.list_plugins = function(){

   var data = { 'cmd': 'list plugins' } ;

   return __sendAndGetResult( this, data ) ;
}

/******************************** 集群操作 ************************************/
/*
   创建集群
   clusterInfo:
      {
          "ClusterName": "myCluster1",
          "Desc": "",
          "SdbUser": "sdbadmin",
          "SdbPasswd": "sdbadmin",
          "SdbUserGroup": "sdbadmin_group",
          "InstallPath": "/opt/sequoiadb/",
          "GrantConf": [
              {
                  "Name": "HostFile",
                  "Privilege": true
              },
              {
                  "Name": "RootUser",
                  "Privilege": true
              }
          ]
      }
*/
SdbOMCtrl.prototype.create_cluster = function( clusterInfo ){

   var data = { 'cmd': 'create cluster', 'ClusterInfo': clusterInfo } ;

   __sendAndCheckResult( this, data ) ;
}

/*
   删除集群
*/
SdbOMCtrl.prototype.remove_cluster = function( clusterName ){

   var data = { 'cmd': 'remove cluster', 'ClusterName': clusterName } ;

   __sendAndCheckResult( this, data ) ;
}

/*
   查询集群
   filter   选填
   selector 选填
   sort     选填
   hint     选填
   skip     选填
   limit    选填
*/
SdbOMCtrl.prototype.query_cluster = function( filter, selector, sort, hint, skip, limit ){

   var data = { 'cmd': 'query cluster' } ;

   return __sendAndGetResult2( this, data, filter, selector, sort, hint, skip, limit ) ;
}

/*
   设置指定集群资源的授权
   clusterName:   集群名
   name:          资源名
      HostFile | 暂时只有这个
   privilege:     是否授权
      true | false
*/
SdbOMCtrl.prototype.grant_sysconf = function( clusterName, name, privilege ){

   var data = {
      'cmd': 'grant sysconf',
      'ClusterName': clusterName,
      'name': name,
      'privilege': privilege
   } ;

   __sendAndCheckResult( this, data ) ;
}

/******************************** 主机操作 ************************************/
/*
   扫描主机
   HostInfo:
      {
         "ClusterName": "myCluster1",
         "HostInfo": [
            {
               "IP": "192.168.3.231"
            }
         ],
         "User": "root",
         "Passwd": "123",
         "SshPort": "22",
         "AgentService": "11790"
      }

   return:
      {
          "errno": 0,
          "detail": "",
          "Status": "finish",
          "IP": "192.168.3.231",
          "HostName": "ubuntu-jw-01"
      }
*/
SdbOMCtrl.prototype.scan_host = function( hostInfo ){

   var data = { 'cmd': 'scan host', 'HostInfo': hostInfo } ;

   return __sendAndGetResult( this, data ) ;
}

/*
   检查主机
   HostInfo:
      {
         "ClusterName": "myCluster1",
         "HostInfo": [
            {
               "HostName": "ubuntu-jw-01",
               "IP": "192.168.3.231",
               "User": "root",
               "Passwd": "123",
               "SshPort": "22",
               "AgentService": "11790"
            }
         ],
         "User": "-",
         "Passwd": "-",
         "SshPort": "-",
         "AgentService": "-"
      }

   return:
      {
          "IP": "192.168.3.231",
          "HostName": "ubuntu-jw-01",
          "OS": {
              "Distributor": "Ubuntu",
              "Release": "16.04",
              "Description": "Ubuntu 16.04.4 LTS",
              "Bit": 64
          },
          "OMA": {
              "Version": "3.0",
              "SdbUser": "sdbadmin",
              "Path": "/opt/sequoiadb/",
              "Service": "11790",
              "Release": 35826
          },
          "CPU": [
              {
                  "ID": "",
                  "Model": "Intel(R) Core(TM) i5-5200U CPU @ 2.20GHz",
                  "Core": 1,
                  "Freq": "2.1949141GHz"
              }
          ],
          "Memory": {
              "Model": "",
              "Size": 992,
              "Free": 68
          },
          "Net": [
              {
                  "Name": "lo",
                  "Model": "",
                  "Bandwidth": "",
                  "IP": "127.0.0.1"
              },
              {
                  "Name": "enp0s3",
                  "Model": "",
                  "Bandwidth": "",
                  "IP": "192.168.3.231"
              }
          ],
          "Port": [
              {
                  "Port": "",
                  "CanUse": false
              }
          ],
          "Service": [
              {
                  "Name": "",
                  "IsRunning": false,
                  "Version": ""
              }
          ],
          "Safety": {
              "Name": "",
              "Context": "",
              "IsRunning": false
          },
          "Disk": [
              {
                  "Name": "/dev/mapper/ubuntu--jw--01--vg-root",
                  "Mount": "/",
                  "Size": 38716,
                  "Free": 25898,
                  "IsLocal": true,
                  "CanUse": true
              },
              {
                  "Name": "systemd-1",
                  "Mount": "/proc/sys/fs/binfmt_misc",
                  "Size": 0,
                  "Free": 0,
                  "IsLocal": false,
                  "CanUse": false
              },
              {
                  "Name": "lxcfs",
                  "Mount": "/var/lib/lxcfs",
                  "Size": 0,
                  "Free": 0,
                  "IsLocal": false,
                  "CanUse": false
              },
              {
                  "Name": "sac",
                  "Mount": "/mnt/shared",
                  "Size": 476835,
                  "Free": 295402,
                  "IsLocal": false,
                  "CanUse": true
              }
          ]
      }
*/
SdbOMCtrl.prototype.check_host = function( hostInfo ){

   var data = { 'cmd': 'check host', 'HostInfo': hostInfo } ;

   return __sendAndGetResult( this, data ) ;
}

/*
   添加主机
   HostInfo:
      {
          "ClusterName": "myCluster1",
          "HostInfo": [
              {
                  "User": "root",
                  "Passwd": "123",
                  "SshPort": "22",
                  "AgentService": "11790",
                  "HostName": "ubuntu-jw-01",
                  "IP": "192.168.3.231",
                  "CPU": [
                      {
                          "ID": "",
                          "Model": "Intel(R) Core(TM) i5-5200U CPU @ 2.20GHz",
                          "Core": 1,
                          "Freq": "2.1949141GHz"
                      }
                  ],
                  "Memory": {
                      "Model": "",
                      "Size": 992,
                      "Free": 68
                  },
                  "Net": [
                      {
                          "Name": "lo",
                          "Model": "",
                          "Bandwidth": "",
                          "IP": "127.0.0.1"
                      },
                      {
                          "Name": "enp0s3",
                          "Model": "",
                          "Bandwidth": "",
                          "IP": "192.168.3.231"
                      }
                  ],
                  "Port": [
                      {
                          "Port": "",
                          "CanUse": false
                      }
                  ],
                  "Service": [
                      {
                          "Name": "",
                          "IsRunning": false,
                          "Version": ""
                      }
                  ],
                  "OMA": {
                      "Version": "3.0",
                      "SdbUser": "sdbadmin",
                      "Path": "/opt/sequoiadb/",
                      "Service": "11790",
                      "Release": 35826
                  },
                  "Safety": {
                      "Name": "",
                      "Context": "",
                      "IsRunning": false
                  },
                  "OS": {
                      "Distributor": "Ubuntu",
                      "Release": "16.04",
                      "Description": "Ubuntu 16.04.4 LTS",
                      "Bit": 64
                  },
                  "InstallPath": "/opt/sequoiadb/",
                  "Disk": [
                      {
                          "Name": "/dev/mapper/ubuntu--jw--01--vg-root",
                          "Mount": "/",
                          "Size": 38716,
                          "Free": 25898,
                          "IsLocal": true
                      }
                  ]
              }
          ],
          "User": "-",
          "Passwd": "-",
          "SshPort": "-",
          "AgentService": "-"
      }

   return:
      {
          "TaskID": 45
      }
*/
SdbOMCtrl.prototype.add_host = function( hostInfo ){

   var data = { 'cmd': 'add host', 'HostInfo': hostInfo } ;

   return __sendAndGetResult( this, data ) ;
}

/*
   删除主机
   HostInfo:
      {
          "ClusterName": "testCluster1",
          "HostInfo": [
              {
                  "HostName": "h1"
              },
              {
                  "HostName": "h2"
              },
              {
                  "HostName": "h3"
              }
          ]
      }

   return:
      {
          "TaskID": 46
      }
*/
SdbOMCtrl.prototype.remove_host = function( hostInfo ){

   var data = { 'cmd': 'remove host', 'HostInfo': hostInfo } ;

   return __sendAndGetResult( this, data ) ;
}

/*
   解绑主机
   clusterName:  集群名
   hostInfo: 主机列表
      [
          {
              "HostName": "h1"
          },
          {
              "HostName": "h2"
          }
      ]
*/
SdbOMCtrl.prototype.unbind_host = function( clusterName, hostInfo ){
   var data = {
      'cmd': 'unbind host',
      'ClusterName': clusterName,
      'HostInfo': { 'HostInfo': hostInfo }
   } ;

   __sendAndCheckResult( this, data ) ;
}

/*
   查询主机
   filter   选填
   selector 选填
   sort     选填
   hint     选填
   skip     选填
   limit    选填
*/
SdbOMCtrl.prototype.query_host = function( filter, selector, sort, hint, skip, limit ){

   var data = { 'cmd': 'query host' } ;

   return __sendAndGetResult2( this, data, filter, selector, sort, hint, skip, limit ) ;
}

/*
   部署run包
   ClusterName: 主机所在集群
   PackageName: sequoiasql-postgresql | sequoiasql-mysql 要安装的包名
   InstallPath: /opt/sequoiasql/postgresql/ 安装路径
   hostList:   主机信息
      [
         {
            "HostName": "h3",
            "User": "root",
            "Passwd": "sequoiadb"
         },
         ...
      ]
   User: 管理员用户名
   Passwd: 管理员密码
   Enforced: true | false 是否强制安装

   return:
      {
          "TaskID": 46
      }
*/
SdbOMCtrl.prototype.deploy_package = function( clusterName, packageName, installPath, hostList, enforced ){
   var data = {
      'cmd': 'deploy package',
      'ClusterName': clusterName,
      'PackageName': packageName,
      'InstallPath': installPath,
      'HostInfo': { 'HostInfo': hostList },
      'User': '-',
      'Passwd': '-',
      'Enforced': enforced
   } ;

   return __sendAndGetResult( this, data ) ;
}

/*
   查询主机状态
   hostList:   主机信息
      [
         { "HostName": "h1" },
         { "HostName": "h2" },
         { "HostName": "h3" }
      ]

   return:
      {
          "TaskID": 46
      }
*/
SdbOMCtrl.prototype.query_host_status = function( hostList ){
   var data = {
      'cmd': 'query host status',
      'HostInfo': { 'HostInfo': hostList }
   } ;

   return __sendAndGetResult( this, data ) ;
}

/******************************** 业务操作 ************************************/

/*
   列出支持创建业务的业务类型

   return:
   {
      "BusinessType": "sequoiadb",
      "BusinessDesc": "SequoiaDB数据库"
   } 
   {
      "BusinessType": "sequoiasql-postgresql",
      "BusinessDesc": "SequoiaSQL-PostgreSQL"
   }
*/
SdbOMCtrl.prototype.list_business_type = function(){

   var data = { 'cmd': 'list business type' } ;

   return __sendAndGetResult( this, data ) ;
}

/*
   获取业务配置模板
   businessType: 业务类型 sequoiadb | pg、mysql暂不需要

   return:
   {
       "DeployMod": "distribution",
       "WebName": "集群模式",
       "BusinessType": "sequoiadb",
       "Property": [
           {
               "Name": "replicanum",
               "Type": "int",
               "Default": "3",
               "Valid": "1,3,5,7",
               "Display": "select box",
               "Edit": "true",
               "Desc": "数据拷贝数",
               "Level": "0",
               "WebName": "副本数"
           },
           {
               "Name": "datagroupnum",
               "Type": "int",
               "Default": "1",
               "Valid": "1-",
               "Display": "edit box",
               "Edit": "true",
               "Desc": "数据分区组的数量",
               "Level": "0",
               "WebName": "分区组数"
           },
           {
               "Name": "catalognum",
               "Type": "int",
               "Default": "3",
               "Valid": "1,3,5,7",
               "Display": "select box",
               "Edit": "true",
               "Desc": "编目节点的数量",
               "Level": "0",
               "WebName": "编目节点数"
           },
           {
               "Name": "coordnum",
               "Type": "int",
               "Default": "0",
               "Valid": "0-993",
               "Display": "edit box",
               "Edit": "true",
               "Desc": "协调节点的数量，填0则所有主机都安装一个协调节点",
               "Level": "0",
               "WebName": "协调节点数"
           }
       ]
   }
   ...
*/
SdbOMCtrl.prototype.get_business_template = function( businessType ){

   var data = { 'cmd': 'get business template', 'BusinessType': businessType } ;

   return __sendAndGetResult( this, data ) ;
}

/*
   获取业务配置
   templateInfo: 
      {
          "ClusterName": "testCluster1",
          "BusinessName": "myModule1",
          "DeployMod": "distribution",
          "BusinessType": "sequoiadb",
          "Property": [
              {
                  "Name": "replicanum",
                  "Value": "3"
              },
              {
                  "Name": "datagroupnum",
                  "Value": "1"
              },
              {
                  "Name": "catalognum",
                  "Value": "3"
              },
              {
                  "Name": "coordnum",
                  "Value": "0"
              }
          ],
          "HostInfo": [
              {
                  "HostName": "h1"
              },
              ...
          ]
      }
   operationType: 操作类型，deploy | extend，默认deploy

   return:
      {
          "Config": [
              {
                  "HostName": "h1",
                  "datagroupname": "",
                  "dbpath": "/mnt/sequoiadb/database/coord/11810",
                  "svcname": "11810",
                  "role": "coord",
                  ...
              },
              {
                  "HostName": "h2",
                  "datagroupname": "",
                  "dbpath": "/mnt/sequoiadb/database/coord/11810",
                  "svcname": "11810",
                  "role": "coord",
                  ...
              },
              ...
          ],
          "BusinessName": "myModule1",
          "BusinessType": "sequoiadb",
          "DeployMod": "distribution",
          "ClusterName": "testCluster1",
          "Property": [
              {
                  "Name": "dbpath",
                  "WebName": "数据路径",
                  "Type": "path",
                  "Default": "/opt/sequoiadb/database/standalone",
                  "Valid": "",
                  "Display": "text box",
                  "Edit": "true",
                  "Desc": "数据存储路径",
                  "Level": "0",
                  "DynamicEdit": "false",
                  "BatchEdit": "false"
              },
              ...
          ]
      }
*/
SdbOMCtrl.prototype.get_business_config = function( templateInfo, operationType ){
   var data = { 'cmd': 'get business config', 'TemplateInfo': templateInfo } ;
   if( typeof( operationType ) == 'string' )
   {
      data['OperationType'] = operationType ;
   }
   return __sendAndGetResult( this, data ) ;
}

/*
   添加业务
   config: 业务配置
      {
          "ClusterName": "testCluster1",
          "BusinessType": "sequoiadb",
          "BusinessName": "myModule1",
          "DeployMod": "distribution",
          "Config": [
              {
                  "HostName": "h1",
                  "datagroupname": "",
                  "dbpath": "/mnt/sequoiadb/database/coord/11810",
                  "svcname": "11810",
                  "role": "coord",
                  ...
              },
              {
                  "HostName": "h2",
                  "datagroupname": "",
                  "dbpath": "/mnt/sequoiadb/database/coord/11810",
                  "svcname": "11810",
                  "role": "coord",
                  ...
              },
              ...
          ]
      }
*/
SdbOMCtrl.prototype.add_business = function( config ){
   var data = { 'cmd': 'add business', 'ConfigInfo': config, 'Force': true } ;

   return __sendAndGetResult( this, data ) ;
}

/*
   卸载业务
   businessName: 业务名
*/
SdbOMCtrl.prototype.remove_business = function( businessName ){
   var data = { 'cmd': 'remove business', 'BusinessName': businessName } ;

   return __sendAndGetResult( this, data ) ;
}

/*
   业务扩容
   configInfo: 业务配置
      {
          "ClusterName": "Test_Deploy_Cluster",
          "BusinessType": "sequoiadb",
          "BusinessName": "test_sequoiadb_distribution",
          "DeployMod": "horizontal",
          "Config": [
              {
                  "HostName": "h3",
                  "datagroupname": "group3",
                  "dbpath": "/opt/sequoiadb/database/data/11850",
                  "svcname": "11850",
                  "role": "data",
                  ...
              },
              {
                  "HostName": "h1",
                  "datagroupname": "group3",
                  "dbpath": "/opt/sequoiadb/database/data/11860",
                  "svcname": "11860",
                  "role": "data",
                  ...
              },
              {
                  "HostName": "h2",
                  "datagroupname": "group3",
                  "dbpath": "/opt/sequoiadb/database/data/11850",
                  "svcname": "11850",
                  "role": "data",
                  ...
              }
          ]
      }
*/
SdbOMCtrl.prototype.extend_business = function( configInfo ){
   var data = { 'cmd': 'extend business', 'ConfigInfo': configInfo } ;

   return __sendAndGetResult( this, data ) ;
}

/*
   业务减容
   configInfo: 业务配置
      {
          "ClusterName": "Test_Deploy_Cluster",
          "BusinessName": "test_sequoiadb_distribution",
          "Config": [
              {
                  "HostName": "h1",
                  "svcname": "11860"
              },
              {
                  "HostName": "h2",
                  "svcname": "11850"
              },
              {
                  "HostName": "h3",
                  "svcname": "11850"
              }
          ]
      }
*/
SdbOMCtrl.prototype.shrink_business = function( configInfo ){
   var data = { 'cmd': 'shrink business', 'ConfigInfo': configInfo } ;

   return __sendAndGetResult( this, data ) ;
}

/*
   解除发现
   clusterName:  集群名
   businessName: 业务名
*/
SdbOMCtrl.prototype.undiscover_business = function( clusterName, businessName ){

   var data = { 'cmd': 'undiscover business', 'ClusterName': clusterName, 'BusinessName': businessName } ;

   __sendAndCheckResult( this, data ) ;
}

/*
   查询业务
   filter   选填
   selector 选填
   sort     选填
   hint     选填
   skip     选填
   limit    选填
*/
SdbOMCtrl.prototype.query_business = function( filter, selector, sort, hint, skip, limit ){

   var data = { 'cmd': 'query business' } ;

   return __sendAndGetResult2( this, data, filter, selector, sort, hint, skip, limit ) ;
}

/*
   列出所有关联
*/
SdbOMCtrl.prototype.list_relationship = function(){

   var data = { 'cmd': 'list relationship' } ;

   return __sendAndGetResult( this, data ) ;
}

/*
   查询业务鉴权
   filter   选填
   selector 选填
   sort     选填
   hint     选填
   skip     选填
   limit    选填
*/
SdbOMCtrl.prototype.query_business_authority = function( filter, selector, sort, hint, skip, limit ){

   var data = { 'cmd': 'query business authority' } ;

   return __sendAndGetResult2( this, data, filter, selector, sort, hint, skip, limit ) ;
}

/*
   解绑业务
   clusterName:  集群名
   businessName: 业务名
*/
SdbOMCtrl.prototype.unbind_business = function( clusterName, businessName ){

   var data = { 'cmd': 'unbind business', 'ClusterName': clusterName, 'BusinessName': businessName } ;

   __sendAndCheckResult( this, data ) ;
}

/*
   发现业务
   ConfigInfo:
      {
          "ClusterName": "Test_Deploy_Cluster",
          "BusinessType": "sequoiadb",
          "BusinessName": "myModule1",
          "BusinessInfo": {
              "HostName": "192.168.20.151",
              "ServiceName": "11810",
              "User": "",
              "Passwd": "",
              "AgentService": "11790"
          }
      }
*/
SdbOMCtrl.prototype.discover_business = function( configInfo ){

   var data = { 'cmd': 'discover business', 'ConfigInfo': configInfo } ;

   __sendAndCheckResult( this, data ) ;
}

/*
   同步业务
   clusterName:  集群名
   businessName: 业务名
*/
SdbOMCtrl.prototype.sync_business_configure = function( clusterName, businessName ){

   var data = { 'cmd': 'sync business configure', 'ClusterName': clusterName, 'BusinessName': businessName } ;

   __sendAndCheckResult( this, data ) ;
}

/*
   关联业务
   Name: 关联名
   From: 关联的业务
   To:   被关联的业务
   Options: 选项
      {
          "transaction": "off",
          "preferedinstance": "a",
          "DbName": "postgres"
      }
*/
SdbOMCtrl.prototype.create_relationship = function( name, fromName, toName, options ){

   var data = { 'cmd': 'create relationship', 'Name': name, 'From': fromName, 'To': toName, 'Options': options } ;

   __sendAndCheckResult( this, data ) ;
}

/*
   删除关联
   Name: 关联名
*/
SdbOMCtrl.prototype.remove_relationship = function( name ){

   var data = { 'cmd': 'remove relationship', 'Name': name } ;

   __sendAndCheckResult( this, data ) ;
}

/******************************** 业务数据操作 ********************************/

/*
   通过OM向业务请求执行命令
   clusterName:   集群名
   businessName:  业务名
   data:          SequoiaDB的Rest命令
*/
SdbOMCtrl.prototype.sequoiadb_exec = function( clusterName, businessName, data ){

   this.request.setHeader( "SdbClusterName", clusterName ) ;
   this.request.setHeader( "SdbBusinessName", businessName ) ;

   var result = __sendAndGetResult3( this, data ) ;

   return result ;
}

SdbOMCtrl.prototype.postgresql_exec = function( clusterName, businessName, data ){

   this.request.setHeader( "SdbClusterName", clusterName ) ;
   this.request.setHeader( "SdbBusinessName", businessName ) ;

   var result = __sendAndGetResult3( this, data, 'sql' ) ;

   return result ;
}

/******************************** 任务操作 ************************************/

/*
   查看任务信息
*/
SdbOMCtrl.prototype.query_task1 = function( filter, selector, sort, hint, skip, limit ){

   var data = { 'cmd': 'query task' } ;

   return __sendAndGetResult2( this, data, filter, selector, sort, hint, skip, limit ) ;
}

/*
   查看指定ID的任务信息
*/
SdbOMCtrl.prototype.query_task2 = function( taskID ){

   var data = { 'cmd': 'query task', 'filter': { "TaskID": taskID } } ;

   return __sendAndGetResult( this, data ) ;
}

/*
   等待指定ID的任务完成
*/
SdbOMCtrl.prototype.query_task3 = function( taskID, isShowProgress ){
   var taskInfo ;
   var progress ;
   while( true )
   {
      var data = { 'cmd': 'query task', 'filter': { "TaskID": taskID } } ;
      taskInfo = __sendAndGetResult( this, data ) ;
      if( taskInfo.length == 0 )
      {
         break ;
      }
      if( taskInfo[0]['Status'] == 4 )
      {
         if( isShowProgress === true )
         {
            println( "===> 100%" ) ;
         }
         break ;
      }
      if( isShowProgress === true )
      {
         if( progress !== taskInfo[0]['Progress'] )
         {
            progress = taskInfo[0]['Progress'] ;
            println( "===> " + progress + "%" ) ;
         }
      }
      sleep( 1000 ) ;
   }
   return taskInfo ;
}