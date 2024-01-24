//@ sourceURL=Index.js
"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //主控制器
   sacApp.controllerProvider.register( 'Data.PostgreSQL.Database.Ctrl', function( $scope, $location, SdbFunction, $timeout, SdbRest, SdbPromise, SdbSignal, SdbSwap, Loading ){
      var clusterName    = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType     = SdbFunction.LocalData( 'SdbModuleType' ) ;
      $scope.ModuleName  = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiasql-postgresql' || $scope.ModuleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      //初始化
      $scope.CurrentDbName = '' ;
      $scope.DatabaseList = [] ;
      $scope.BrowseType = 'userTable' ;
      SdbSwap.foreignList = {} ;
      SdbSwap.isShowCreateTable = SdbPromise.init( 2 ) ;

      //构造表格
      $scope.GridTable = {
         'title': {
            'Index'       : '#',
            'Name'        : $scope.pAutoLanguage( '表名' ),
            'Operation'   : '',
            'Type'        : $scope.pAutoLanguage( '类型' ),
            'SCHEMA'      : $scope.pAutoLanguage( '数据库模式' ),
            'Owner'       : $scope.pAutoLanguage( '所属用户' ),
            'Description' : $scope.pAutoLanguage( '描述' )
         },
         'body': [],
         'options': {
            'width': {
               'Index'       : '35px',
               'Name'        : '30%',
               'Operation'   : '60px',
               'Type'        : '17%',
               'SCHEMA'      : '17%',
               'Owner'       : '17%',
               'Description' : '19%'
            },
            'sort': {
               'Index'       : true,
               'Name'        : true,
               'Operation'   : false,
               'Type'        : true,
               'SCHEMA'      : true,
               'Owner'       : true,
               'Description' : true
            },
            'max': 50
         }
      } ;

      //获取table列表
      var queryTableList = function( dbName, type ){
         var sql = '' ;
         if( type == 'systemTable' )
         {
            sql = 'select schemaname as table_schema, tablename as table_name, tableowner as usename from pg_tables where schemaname = \'pg_catalog\'' ;
         }
         else
         {
            sql = 'select t.table_name,t.table_schema, t.table_type,\
                  u.usename from information_schema.tables t join\
                  pg_catalog.pg_class c on (t.table_name = c.relname)\
                  join pg_catalog.pg_user u on (c.relowner = u.usesysid)\
                  where t.table_schema=\'public\' order by t.table_type desc' ;
         }
         
         var data = { 'Sql': sql, 'DbName': dbName, 'IsAll': 'true' } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( tableList ){
               $scope.GridTable['body'] = [] ;
               if( type == 'systemTable' )
               {
                  $.each( tableList, function( index, tableInfo ){
                     tableInfo['table_type'] = 'system table' ;
                     $scope.GridTable['body'].push( {
                        'Index': index + 1,
                        'Name': tableInfo['table_name'],
                        'Operation': '',
                        'Type': tableInfo['table_type'],
                        'SCHEMA': tableInfo['table_schema'],
                        'Owner': tableInfo['usename'],
                        'Description': ''
                     } ) ;
                  } ) ;
               }
               else
               {
                  SdbSwap.deleteTableList = [] ;
                  $.each( tableList, function( index, tableInfo ){
                     if( tableInfo['table_type'] == 'FOREIGN TABLE' )
                     {
                        tableInfo['table_type'] = 'foreign table' ;
                     }
                     else if( tableInfo['table_type'] == 'BASE TABLE' )
                     {
                        tableInfo['table_type'] = 'table' ;
                     }
                     $scope.GridTable['body'].push( {
                        'Index': index + 1,
                        'Name': tableInfo['table_name'],
                        'Operation': '',
                        'Type': tableInfo['table_type'],
                        'SCHEMA': tableInfo['table_schema'],
                        'Owner': tableInfo['usename'],
                        'Description': ''
                     } ) ;

                     SdbSwap.deleteTableList.push( { 'key': tableInfo['table_name'], 'value': index, 'type': tableInfo['table_type'] } ) ;
                  } ) ;
               }

            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  queryTableList( dbName ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': true
         } ) ;
      }

      SdbSwap.counter = 0 ;
      SdbSwap.counter2 = 0 ;

      //查询foreign servers
      SdbSwap.getForeignServers = function( dbName ){
         var sql = 'SELECT srvname FROM pg_foreign_server WHERE srvfdw IN ( select oid from pg_foreign_data_wrapper where fdwname = \'sdb_fdw\' )' ;
         var data = { 'Sql': sql, 'DbName': dbName, 'IsAll': 'true' } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( result ){
               var url = $location.url() ;
               if( url.indexOf( '/Data/SequoiaSQL/PostgreSQL/Database/Index' ) > -1 )
               {
                  if( result.length > 0 )
                  {
                     $.each( result, function( index, info ){
                        SdbSwap.foreignList[dbName].push( { 'key': info['srvname'], 'value': info['srvname'] } ) ;
                     } ) ;
                  }
               }
            },
            'complete': function(){
               var url = $location.url() ;
               if( url.indexOf( '/Data/SequoiaSQL/PostgreSQL/Database/Index' ) > -1 )
               {
                  ++SdbSwap.counter ;
                  if( SdbSwap.counter2 == SdbSwap.counter )
                  {
                     SdbSwap.isShowCreateTable.resolve() ;
                  }
                  else if( SdbSwap.counter2 > SdbSwap.counter )
                  {
                     SdbSwap.getForeignServers( $scope.DatabaseList[SdbSwap.counter]['datname'] ) ;
                  }
               }
            }
         },{
            'showLoading': false
         } ) ;
      }

      //循环查询foreign servers
      var loopQueryServer = function(){
         SdbSwap.counter2 = $scope.DatabaseList.length ;
         SdbSwap.counter = 0 ;
         SdbSwap.getForeignServers( $scope.DatabaseList[0]['datname'] ) ;
      }

      //获取database列表
      var queryDbList = function(){
         SdbSwap.isShowCreateTable.clear() ;
         var data ;
         var sql = 'SELECT datname FROM pg_database WHERE datname NOT LIKE \'template0\' AND datname NOT LIKE \'template1\'' ;
         if( $scope.CurrentDbName.length == 0 )
         {
            data = { 'Sql': sql, 'IsAll': 'true' } ;
         }
         else
         {
            data = { 'Sql': sql, 'DbName': $scope.CurrentDbName, 'IsAll': 'true' } ;
         }
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( dbList ){
               if( dbList.length <= 0 )
               {
                  return ;
               }

               $scope.DatabaseList = dbList ;
               SdbSwap.foreignList = {} ;

               //查询Foreign Servers
               $.each( $scope.DatabaseList, function( index, database ){
                  SdbSwap.foreignList[database['datname']] = [] ;
               } ) ;

               //查询关联信息
               $timeout(function(){
　　               loopQueryServer() ;
               }, 0 ) ;

               //设置当前选择数据库
               if( $scope.CurrentDbName.length == 0 )
               {
                  $scope.CurrentDbName = dbList[0]['datname'] ;
               }

               //查询表
               queryTableList( $scope.CurrentDbName ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  queryDbList() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      queryDbList() ;

      //创建数据库
      SdbSignal.on( 'createDatabase', function( dbName ){
         var sql = 'create database ' + addQuotes( dbName ) ;
         var data = { 'Sql': sql, 'DbName': $scope.CurrentDbName } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( result ){
               $scope.CurrentDbName = dbName ;
               queryDbList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  SdbSignal.commit( 'createDatabase', dbName ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      } ) ;

      //删除数据库
      SdbSignal.on( 'removeDatabase', function( dbName ){
         var sql = 'drop database ' + addQuotes( dbName ) ;
         var data = { 'Sql': sql, 'DbName': $scope.CurrentDbName } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( result ){
               queryDbList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  SdbSignal.commit( 'removeDatabase', dbName ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      } ) ;

      //创建数据表
      $scope.createTable = function( sql, dbName ){
         var data = { 'Sql': sql, 'DbName': dbName } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( result ){
               queryDbList( dbName ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  $scope.createTable( sql, dbName ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //删除数据表
      SdbSignal.on( 'removeTable', function( sql ){
         var data = { 'Sql': sql, 'DbName': $scope.CurrentDbName } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( result ){
               queryTableList( $scope.CurrentDbName ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  SdbSignal.commit( 'removeTable', sql ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      } ) ;
      
      //修改表名
      SdbSignal.on( 'AlterTable', function( sql ){
         var data = { 'Sql': sql, 'DbName': $scope.CurrentDbName } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( result ){
               queryTableList( $scope.CurrentDbName ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  SdbSignal.commit( 'AlterTable', sql ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      } ) ;

      //发送打开修改表名弹窗信号
      $scope.ShowAlterTable = function( tableName ){
         SdbSignal.commit( 'OpenAlterTable', tableName ) ;
      }

      //发送打开删除数据表弹窗信号
      $scope.ShowRemoveTable = function( tableName, tableType ){
         var info = { 'Name': tableName, 'Type': tableType } ;
         SdbSignal.commit( 'OpenRemoveTable', info ) ;
      }

      //打开 创建数据表弹窗
      $scope.ShowCreateTable = function(){
         SdbSwap.isShowCreateTable.resolve() ;
         
         if( SdbSwap.counter2 != SdbSwap.counter )
         {
            Loading.create() ;
         }
      }

      //切换数据库
      $scope.ShowTableInfo = function( dbName ){
         $scope.CurrentDbName = dbName ;
         queryTableList( $scope.CurrentDbName, $scope.BrowseType ) ;
      } ;

      //切换table类型
      $scope.ChangeTableType = function( type ){
         if( $scope.BrowseType == type )
         {
            return ;
         }
         else
         {
            $scope.BrowseType = type ;
            queryTableList( $scope.CurrentDbName, type ) ;
         }
      }

      //进入数据操作页面
      $scope.GotoData = function( tableName, tableType ){
         SdbFunction.LocalData( 'PgsqlDbName', $scope.CurrentDbName ) ;
         SdbFunction.LocalData( 'PgsqlTbName', tableName ) ;
         SdbFunction.LocalData( 'PgsqlTbType', tableType ) ;
         $location.path( '/Data/SequoiaSQL/PostgreSQL/Operate/Data' ).search( { 'r': new Date().getTime() } ) ;
      }
   } ) ;

   //弹窗操作控制器
   sacApp.controllerProvider.register( 'Data.PostgreSQL.Database.Window.Ctrl', function( $scope, $timeout, SdbFunction, SdbSignal, SdbSwap, Loading ){
      //高度偏移量
      $scope.BoxHeight = { 'offsetY': -149 } ;
      //判断如果是Firefox浏览器的话，调整右侧表格高度
      var browser = SdbFunction.getBrowserInfo() ;
      if( browser[0] == 'firefox' )
      {
         $scope.BoxHeight = { 'offsetY': -164 } ;
      }
      //创建数据表 弹窗
      $scope.CreateTableWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 创建数据表 弹窗
      var showCreateTable = function(){

         //Promise计数
         SdbSwap.isShowCreateTable.resolve() ;
         
         //关闭loading
         Loading.close() ;

         var listValue = [] ;
         $.each( $scope.DatabaseList, function( index, databaseName ){
            listValue.push( { 'key': databaseName['datname'], 'value': databaseName['datname'] } ) ;
         } ) ;

         var form1 = {
            'inputList': [
               {
                  "name": "dbName",
                  "webName": $scope.pAutoLanguage( '数据库' ),
                  "type": "select",
                  "required": true,
                  "value": $scope.CurrentDbName,
                  "valid": listValue,
                  "onChange": function( name, key, value ){
                     //如果所选的数据库没有关联sdb，则只能创建普通表
                     if( SdbSwap.foreignList[key].length < 1 )
                     {
                        form1['inputList'][2]['disabled'] = true ;
                        form1['inputList'][2]['value'] = 'normal' ;
                        $scope.CreateTableWindow['config']['Form2'] = normalForm ;
                     }
                     else
                     {
                        form1['inputList'][2]['disabled'] = false ;
                        foreignForm['inputList'][0]['value'] = SdbSwap.foreignList[key][0]['key'] ;
                        foreignForm['inputList'][0]['valid'] = SdbSwap.foreignList[key] ;
                     }
                  }
               },
               {
                  "name": "tbName",
                  "webName": $scope.pAutoLanguage( '数据表名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1,
                     "max": 63,
                     "regex": "^[a-zA-Z_]+[0-9a-zA-Z_]*$",
                     "regexError": sprintf( $scope.pAutoLanguage( '?由字母和数字或\"_\"组成，并且以字母或\"_\"起头。' ), $scope.pAutoLanguage( '数据表名' ) )
                  }
               },
               {
                  "name": "type",
                  "webName": $scope.pAutoLanguage( '表类型' ),
                  "type": "select",
                  "required": true,
                  "value": 'normal',
                  "valid": [
                     { "key": $scope.pAutoLanguage( '普通表' ), "value": 'normal' },
                     { "key": $scope.pAutoLanguage( '外部表' ), "value": 'foreign' }
                  ],
                  "onChange": function( name, key, value ){
                     if( value == 'foreign' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = foreignForm ;
                     }
                     else if( value == 'normal' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = normalForm ;
                     }
                  }
               }
            ]
         } ;

         var normalForm = {
            'inputList': [
               {
                  "name": "fields",
                  "webName":  $scope.pAutoLanguage( '字段结构' ),
                  "required": true,
                  "type": "list",
                  "valid": {
                     "min": 1
                  },
                  "child":[
                     [
                        {
                           "name": "name",
                           "webName":  $scope.pAutoLanguage( '字段' ),
                           "type": "string",
                           "placeholder": $scope.pAutoLanguage( "字段名" ),
                           "value": "",
                           "valid": {
                              "min": 1,
                              "max": 63,
                              "regex": "^[a-zA-Z_]+[0-9a-zA-Z_]*$",
                              "regexError": sprintf( $scope.pAutoLanguage( '?由字母和数字或\"_\"组成，并且以字母或\"_\"起头。' ), $scope.pAutoLanguage( '字段名' ) )
                           }
                        },
                        {
                           "name": "type",
                           "type": "select",
                           "value": "integer",
                           "valid": [
                              { "key": 'smallint', "value": "smallint" },
                              { "key": 'integer', "value": "integer" },
                              { "key": 'bigint', "value": "bigint" },
                              { "key": 'decimal', "value": "decimal" },
                              { "key": 'numeric', "value": "numeric" },
                              { "key": 'real', "value": "real" },
                              { "key": 'double precision', "value": "double precision" },
                              { "key": 'serial', "value": "serial" },
                              { "key": 'bigserial', "value": "bigserial" },
                              { "key": 'money', "value": "money" },
                              { "key": 'varchar', "value": "varchar" },
                              { "key": 'character', "value": "character" },
                              { "key": 'char', "value": "char" },
                              { "key": 'text', "value": "text" },
                              { "key": 'timestamp', "value": "timestamp" },
                              { "key": 'interval', "value": "interval" },
                              { "key": 'date', "value": "date" },
                              { "key": 'time', "value": "time" },
                              { "key": 'boolean', "value": "boolean" }
                           ]
                        },
                        {
                           "name": "length",
                           "type": "int",
                           "webName": $scope.pAutoLanguage( '长度' ),
                           "placeholder": $scope.pAutoLanguage( "长度" ),
                           "value": "",
                           "valid": {
                              "min": 0,
                              "empty": true
                           }
                        },
                        {
                           "name": "default",
                           "webName": $scope.pAutoLanguage( "默认值" ),
                           "placeholder": $scope.pAutoLanguage( "默认值" ),
                           "type": "string"
                        },
                        {
                           "name": "null",
                           "webName": $scope.pAutoLanguage( "空" ),
                           "type": "checkbox",
                           "value": true
                        },
                        {
                           "name": "primary",
                           "webName": $scope.pAutoLanguage( "主键" ),
                           "type": "checkbox",
                           "value": false
                        },
                        {
                           "name": "unique",
                           "webName": $scope.pAutoLanguage( "唯一" ),
                           "type": "checkbox",
                           "value": false
                        }
                     ]
                  ]
               }
            ]
         } ;

         var foreignForm = {
            'inputList': [
               {
                  "name": "foreign_server",
                  "webName": $scope.pAutoLanguage( '服务名' ),
                  "type": "select",
                  "required": true,
                  "value": '',
                  "valid": SdbSwap.foreignList[$scope.CurrentDbName]
               },
               {
                  "name": "csName",
                  "webName": $scope.pAutoLanguage( '集合空间' ),
                  "type": "string",
                  "required": true,
                  "value": '',
                  "valid": {
                     'min': 1
                  }
               },
               {
                  "name": "clName",
                  "webName": $scope.pAutoLanguage( '集合' ),
                  "type": "string",
                  "required": true,
                  "value": '',
                  "valid": {
                     'min': 1
                  }
               },
               {
                  "name": "fields",
                  "webName":  $scope.pAutoLanguage( '字段结构' ),
                  "required": true,
                  "type": "list",
                  "valid": {
                     "min": 1
                  },
                  "child":[
                     [
                        {
                           "name": "name",
                           "webName":  $scope.pAutoLanguage( '字段' ),
                           "type": "string",
                           "placeholder": $scope.pAutoLanguage( "字段名" ),
                           "value": "",
                           "valid": {
                              "min": 1,
                              "max": 63,
                              "regex": "^[a-zA-Z_]+[0-9a-zA-Z_]*$",
                              "regexError": sprintf( $scope.pAutoLanguage( '?由字母和数字或\"_\"组成，并且以字母或\"_\"起头。' ), $scope.pAutoLanguage( '字段名' ) )
                           }
                        },
                        {
                           "name": "type",
                           "type": "select",
                           "value": "integer",
                           "valid": [
                              { "key": 'smallint', "value": "smallint" },
                              { "key": 'integer', "value": "integer" },
                              { "key": 'bigint', "value": "bigint" },
                              { "key": 'decimal', "value": "decimal" },
                              { "key": 'numeric', "value": "numeric" },
                              { "key": 'real', "value": "real" },
                              { "key": 'double precision', "value": "double precision" },
                              { "key": 'serial', "value": "serial" },
                              { "key": 'bigserial', "value": "bigserial" },
                              { "key": 'money', "value": "money" },
                              { "key": 'varchar', "value": "varchar" },
                              { "key": 'character', "value": "character" },
                              { "key": 'char', "value": "char" },
                              { "key": 'text', "value": "text" },
                              { "key": 'timestamp', "value": "timestamp" },
                              { "key": 'interval', "value": "interval" },
                              { "key": 'date', "value": "date" },
                              { "key": 'time', "value": "time" },
                              { "key": 'boolean', "value": "boolean" }
                           ]
                        },
                        {
                           "name": "length",
                           "type": "int",
                           "webName": $scope.pAutoLanguage( '长度' ),
                           "placeholder": $scope.pAutoLanguage( "长度" ),
                           "value": "",
                           "valid": {
                              "min": 0,
                              "empty": true
                           }
                        },
                        {
                           "name": "default",
                           "webName": $scope.pAutoLanguage( "默认值" ),
                           "placeholder": $scope.pAutoLanguage( "默认值" ),
                           "type": "string"
                        },
                        {
                           "name": "null",
                           "webName": $scope.pAutoLanguage( "空" ),
                           "type": "checkbox",
                           "value": true
                        }
                     ]
                  ]
               },
               {
                  "name": "decimal",
                  "webName": 'decimal',
                  "type": "select",
                  "desc": $scope.pAutoLanguage( "是否需要对接SequoiaDB的decimal字段" ),
                  "required": true,
                  "value": 'off',
                  "valid": [
                     { "key": "off", "value": "off" },
                     { "key": "on",  "value": "on" }
                  ]
               }
            ]
         } ;

         $scope.CreateTableWindow['config']['Form1'] = form1 ;
         if( SdbSwap.foreignList[$scope.CurrentDbName].length < 1 )
         {
            form1['inputList'][2]['disabled'] = true ;
            $scope.CreateTableWindow['config']['Form2'] = normalForm ;
         }
         else
         {
            form1['inputList'][2]['value'] = 'foreign' ;
            foreignForm['inputList'][0]['value'] = SdbSwap.foreignList[$scope.CurrentDbName][0]['value'] ;
            $scope.CreateTableWindow['config']['Form2'] = foreignForm ;
         }
         
         $scope.CreateTableWindow['callback']['SetTitle']( $scope.pAutoLanguage( '创建数据表' ) ) ;
         $scope.CreateTableWindow['callback']['SetIcon']( 'fa-plus' ) ;
         $scope.CreateTableWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
            var isClear1 = $scope.CreateTableWindow['config']['Form1'].check() ;
            var isClear2 = $scope.CreateTableWindow['config']['Form2'].check() ;
            if( isClear1 == true && isClear2 ==true )
            {
               var formVal1 = $scope.CreateTableWindow['config']['Form1'].getValue() ;
               var formVal2 = $scope.CreateTableWindow['config']['Form2'].getValue() ;
               //拼合sql
               if( formVal1['type'] == 'foreign' )
               {
                  var sql = 'CREATE FOREIGN TABLE ' + addQuotes( formVal1['tbName'] ) + ' ( ' ;
               }
               else
               {
                  var sql = 'CREATE TABLE ' + addQuotes( formVal1['tbName'] ) + ' ( ' ;
               }
               var primaryKey = formVal1['tbName'] ;
               var primaryKey2 = ' primary key (' ;
               $.each( formVal2['fields'], function( index, fieldInfo ){
                  var subSql = '' ;
                  if( index > 0 )
                  {
                     subSql += ', ' ;
                  }
                  subSql += addQuotes( fieldInfo['name'] ) + ' ' + fieldInfo['type'] ;
                  if( isNaN( fieldInfo['length'] ) == false )
                  {
                     switch( fieldInfo['type'] )
                     {
                     case 'varchar':
                     case 'character':
                     case 'char':
                     case 'decimal':
                     case 'numeric':
                        subSql += '(' + fieldInfo['length'] + ') ' ;
                        break ;
                     default:
                        subSql += ' ' ;
                        break ;
                     }
                  }
                  else
                  {
                     subSql += ' ' ;
                  }
                  if( fieldInfo['primary'] == true )
                  {
                     //判断是否第一个主键
                     if( primaryKey2.length > 14 )
                     {
                        primaryKey2 += ',' ;
                     }
                     primaryKey += ( '_' + fieldInfo['name'] ) ;
                     primaryKey2 += addQuotes( fieldInfo['name'] ) ;
                  }
                  if( fieldInfo['unique'] == true )
                  {
                     subSql += 'UNIQUE ' ;
                  }
                  if( fieldInfo['null'] == true )
                  {
                     subSql += 'NULL ' ;
                  }
                  else
                  {
                     subSql += 'NOT NULL ' ;
                  }
                  if( typeof( fieldInfo['default'] ) == 'string' )
                  {
                     subSql += 'DEFAULT ' + sqlEscape( fieldInfo['default'] ) + ' ' ;
                  }
                  sql += subSql ;
               } ) ;
               
               //判断是否有主键
               if( primaryKey2.length > 14 )
               {
                  sql = sql + ',constraint ' + addQuotes( primaryKey ) + primaryKey2 + ') )' ;
               }
               else
               {
                  sql += ')' ;
               }
               if( formVal1['type'] == 'foreign' )
               {
                  var subSql = ' SERVER ' + formVal2['foreign_server'] + ' OPTIONS( collectionspace \'' + formVal2['csName'] + '\', collection \'' + formVal2['clName'] + '\', decimal \'' + formVal2['decimal'] + '\' )' ;
                  sql += subSql ;
               }
               $scope.createTable( sql, formVal1['dbName'] ) ;
               $scope.CreateTableWindow['callback']['Close']() ;
            }
         } ) ;
         $scope.CreateTableWindow['callback']['Open']() ;
      }

      SdbSwap.isShowCreateTable.then( function(){
         $timeout(function(){
            showCreateTable() ;
         }, 0 ) ;
      } ) ;

      //创建数据库 弹窗
      $scope.CreateDatabaseWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 创建数据库 弹窗
      $scope.ShowCreateDatabase = function(){
         $scope.CreateDatabaseWindow['config'] = {
            inputList: [
               {
                  "name": "dbName",
                  "webName": $scope.pAutoLanguage( '数据库名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1,
                     "max": 63,
                     "regex": "^[a-zA-Z_]+[0-9a-zA-Z_]*$",
                     "regexError": sprintf( $scope.pAutoLanguage( '?由字母和数字或\"_\"组成，并且以字母或\"_\"起头。' ), $scope.pAutoLanguage( '数据库名' ) )
                  }
               }
            ]
         } ;
         $scope.CreateDatabaseWindow['callback']['SetTitle']( $scope.pAutoLanguage( '创建数据库' ) ) ;
         $scope.CreateDatabaseWindow['callback']['SetIcon']( 'fa-plus' ) ;
         $scope.CreateDatabaseWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
            var isClear = $scope.CreateDatabaseWindow['config'].check() ;
            if( isClear == true )
            {
               var formVal = $scope.CreateDatabaseWindow['config'].getValue() ;
               SdbSignal.commit( 'createDatabase', formVal['dbName'] ) ;
               $scope.CreateDatabaseWindow['callback']['Close']() ;
            }
         } ) ;
         $scope.CreateDatabaseWindow['callback']['Open']() ;
      }

      //删除数据库 弹窗
      $scope.RemoveDatabaseWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 删除数据库 弹窗
      $scope.ShowRemoveDatabase = function(){
         var listValue = [] ;
         var defaultDb = null ;
         $.each( $scope.DatabaseList, function( index, databaseName ){
            if( defaultDb == null )
            {
               defaultDb = databaseName["datname"] ;
            }
            listValue.push( { 'key': databaseName["datname"], 'value': databaseName["datname"] } ) ;
         } ) ;
         $scope.RemoveDatabaseWindow['config'] = {
            inputList: [
               {
                  "name": "dbName",
                  "webName": $scope.pAutoLanguage( '数据库名' ),
                  "type": "select",
                  "required": true,
                  "value": defaultDb,
                  "valid": listValue
               }
            ]
         } ;
         $scope.RemoveDatabaseWindow['callback']['SetTitle']( $scope.pAutoLanguage( '删除数据库' ) ) ;
         $scope.RemoveDatabaseWindow['callback']['SetIcon']( 'fa-remove' ) ;
         $scope.RemoveDatabaseWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
            var isClear = $scope.RemoveDatabaseWindow['config'].check() ;
            if( isClear == true )
            {
               var formVal = $scope.RemoveDatabaseWindow['config'].getValue() ;
               SdbSignal.commit( 'removeDatabase', formVal['dbName'] ) ;
            }
            $scope.RemoveDatabaseWindow['callback']['Close']() ;
         } ) ;
         $scope.RemoveDatabaseWindow['callback']['Open']() ;
      }

      //删除表 弹窗
      SdbSignal.on( 'OpenRemoveTable', function( info ){
         var tableName = info['Name'] ;
         var tableType = info['Type'] ;
         var sql = '' ;
         if( tableType == 'foreign table' )
         {
            sql = 'drop foreign table ' + addQuotes( tableName ) ;
         }
         else
         {
            sql = 'drop table ' + addQuotes( tableName ) ;
         }
         $scope.Components.Confirm.type = 3 ;
         $scope.Components.Confirm.context = sprintf( $scope.pAutoLanguage( '是否确定删除表：?？' ), tableName ) ;
         $scope.Components.Confirm.isShow = true ;
         $scope.Components.Confirm.okText = $scope.pAutoLanguage( '确定' ) ;
         $scope.Components.Confirm.ok = function(){
            SdbSignal.commit( 'removeTable', sql ) ;
            $scope.Components.Confirm.isShow = false ;
         }
      } ) ;

      //删除数据表 弹窗
      $scope.RemoveTableWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 删除数据表 弹窗
      $scope.ShowRemoveTable = function(){
         if( SdbSwap.deleteTableList.length > 0 )
         {
            $scope.RemoveTableWindow['config'] = {
               inputList: [
                  {
                     "name": "tbName",
                     "webName": $scope.pAutoLanguage( '数据表名' ),
                     "type": "select",
                     "required": true,
                     "value": SdbSwap.deleteTableList[0]['value'],
                     "valid": SdbSwap.deleteTableList
                  }
               ]
            } ;
            $scope.RemoveTableWindow['callback']['SetTitle']( $scope.pAutoLanguage( '删除数据表' ) ) ;
            $scope.RemoveTableWindow['callback']['SetIcon']( 'fa-remove' ) ;
            $scope.RemoveTableWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
               var formVal = $scope.RemoveTableWindow['config'].getValue() ;
               var sql = '' ;
               var index = formVal['tbName'] ;
               var tableName = SdbSwap.deleteTableList[index]['key'] ;
               var tableType = SdbSwap.deleteTableList[index]['type'] ;
               if( tableType == 'foreign table' )
               {
                  sql = 'drop foreign table ' + addQuotes( tableName ) ;
               }
               else
               {
                  sql = 'drop table ' + addQuotes( tableName ) ;
               }
               SdbSignal.commit( 'removeTable', sql ) ;
               $scope.RemoveTableWindow['callback']['Close']() ;
            } ) ;
            $scope.RemoveTableWindow['callback']['Open']() ;
         }
         
      }

      //修改表名 弹窗
      $scope.AlterTableWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 修改表名 弹窗
      SdbSignal.on( 'OpenAlterTable', function( tableName ){
         $scope.AlterTableWindow['config'] = {
            'inputList': [
               {
                  "name": "oldTbName",
                  "webName": $scope.pAutoLanguage( '原表名' ),
                  "type": "string",
                  "required": true,
                  "disabled": true,
                  "value": tableName
               },
               {
                  "name": "newTbName",
                  "webName": $scope.pAutoLanguage( '新表名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1,
                     "max": 63,
                     "regex": "^[a-zA-Z_]+[0-9a-zA-Z_]*$",
                     "regexError": sprintf( $scope.pAutoLanguage( '?由字母和数字或\"_\"组成，并且以字母或\"_\"起头。' ), $scope.pAutoLanguage( '数据表名' ) )
                  }
               }
            ]
         } ;
         $scope.AlterTableWindow['callback']['SetTitle']( $scope.pAutoLanguage( '修改表名' ) ) ;
         $scope.AlterTableWindow['callback']['SetIcon']( 'fa-edit' ) ;
         $scope.AlterTableWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
            var isClear = $scope.AlterTableWindow['config'].check() ;
            if( isClear == true )
            {
               var formVal = $scope.AlterTableWindow['config'].getValue() ;
               var sql = sprintf( 'alter table ? rename to ?', addQuotes( formVal['oldTbName'] ), addQuotes( formVal['newTbName'] ) ) ;
               SdbSignal.commit( 'AlterTable', sql ) ;
               $scope.AlterTableWindow['callback']['Close']() ;
            }
         } ) ;
         $scope.AlterTableWindow['callback']['Open']() ;
      } ) ;

   } ) ;

}());