//@ sourceURL=Index.js
"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //主控制器
   sacApp.controllerProvider.register( 'Data.MariaDB.Database.Ctrl', function( $scope, $location, SdbFunction, $timeout, SdbRest, SdbPromise, SdbSignal, SdbSwap, Loading ){
      var clusterName    = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType     = SdbFunction.LocalData( 'SdbModuleType' ) ;
      $scope.ModuleName  = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiasql-mariadb' || $scope.ModuleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      //初始化
      $scope.CurrentDbName = 'mysql' ;
      $scope.DatabaseList = [] ;
      SdbSwap.engineList = [] ;
      $scope.TableNum = 0 ;

      //构造表格
      $scope.GridTable = {
         'title': {
            'Index'        : '#',
            'Name'         : $scope.pAutoLanguage( '表名' ),
            'Operation'    : '',
            'Engine'       : $scope.pAutoLanguage( '存储引擎' ),
            'DataNum'      : $scope.pAutoLanguage( '行数' ),
            'TableSize'    : $scope.pAutoLanguage( '表大小' ),
            'CreateTime'   : $scope.pAutoLanguage( '创建时间' ),
            'TableComment' : $scope.pAutoLanguage( '描述' )
         },
         'body': [],
         'options': {
            'width': {
               'Index'        : '35px',
               'Name'         : '20%',
               'Operation'    : '60px',
               'Engine'       : '16%',
               'DataNum'      : '16%',
               'TableSize'    : '16%',
               'CreateTime'   : '16%',
               'TableComment' : '16%'
            },
            'sort': {
               'Index'        : true,
               'Name'         : true,
               'Operation'    : false,
               'Engine'       : true,
               'DataNum'      : true,
               'TableSize'    : false,
               'CreateTime'   : true,
               'TableComment' : true
            },
            'max': 50
         }
      } ;

      //获取table列表
      var queryTableList = function( dbName ){
         var sql = '' ;
         sql = sprintf( "select * from information_schema.TABLES where TABLE_SCHEMA = '?'", dbName ) ;
         var data = { 'Sql': sql, 'DbName': dbName, 'Type': 'mysql', 'IsAll': 'true' } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( tableList ){
               $scope.GridTable['body'] = [] ;
               SdbSwap.deleteTableList = [] ;
               $.each( tableList, function( index, tableInfo ){
                  tableInfo['TableSize'] = sizeConvert2(  parseInt( tableInfo['DATA_LENGTH'] ) + parseInt( tableInfo['INDEX_LENGTH'] )  ) ;
                  $scope.GridTable['body'].push( {
                     'Index': index + 1,
                     'Name': tableInfo['TABLE_NAME'],
                     'Operation': '',
                     'Engine': tableInfo['ENGINE'],
                     'DataNum': tableInfo['TABLE_ROWS'],
                     'TableSize': tableInfo['TableSize'],
                     'CreateTime': tableInfo['CREATE_TIME'],
                     'TableComment': tableInfo['TABLE_COMMENT']
                  } ) ;

                  SdbSwap.deleteTableList.push( { 'key': tableInfo['TABLE_NAME'], 'value': index} ) ;
               } ) ;
               $scope.TableNum = SdbSwap.deleteTableList.length ;
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

      //获取database列表
      var queryDbList = function(){
         var sql = "select * from information_schema.SCHEMATA where SCHEMA_NAME != 'information_schema' and SCHEMA_NAME != 'performance_schema' and SCHEMA_NAME != 'mysql'" ;
         var data = { 'Sql': sql, 'DbName': $scope.CurrentDbName, 'Type': 'mysql', 'IsAll': 'true' } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( dbList ){
               //数据库列表补全系统数据库
               dbList.push( { 'SCHEMA_NAME': 'information_schema' }, { 'SCHEMA_NAME': 'mysql' }, { 'SCHEMA_NAME': 'performance_schema' } ) ;

               $scope.DatabaseList = dbList ;

               //设置当前选择数据库
               if( $scope.CurrentDbName == 'mysql' )
               {
                  $scope.CurrentDbName = dbList[0]['SCHEMA_NAME'] ;
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

      //获取存储引擎列表
      var getEngineList = function(){
         var sql = 'select * from information_schema.engines' ;
         var data = { 'Sql': sql, 'DbName': $scope.CurrentDbName, 'Type': 'mysql', 'IsAll': 'true' } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( result ){
               $.each( result, function( index, info ){
                  SdbSwap.engineList.push( { 'key': info['ENGINE'], value: info['ENGINE'] } ) ;
               } ) ;
               queryDbList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getEngineList() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      getEngineList() ;

      //创建数据库
      SdbSignal.on( 'createDatabase', function( dbName ){
         var sql = sprintf( 'create database `?`', dbName ) ;
         var data = { 'Sql': sql, 'DbName': 'mysql', 'Type': 'mysql' } ;
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
         var sql = sprintf( 'drop database `?`', dbName ) ;
         var data = { 'Sql': sql, 'DbName': $scope.CurrentDbName, 'Type': 'mysql' } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( result ){
               if( dbName == $scope.CurrentDbName )
               {
                  $scope.CurrentDbName = 'mysql' ;
               }
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
         var data = { 'Sql': sql, 'DbName': dbName, 'Type': 'mysql' } ;
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
         var data = { 'Sql': sql, 'DbName': $scope.CurrentDbName, 'Type': 'mysql' } ;
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
         var data = { 'Sql': sql, 'DbName': $scope.CurrentDbName, 'Type': 'mysql' } ;
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
      $scope.ShowRemoveTable = function( tableName ){
         var info = { 'Name': tableName } ;
         SdbSignal.commit( 'OpenRemoveTable', info ) ;
      }

      //打开 创建数据表弹窗
      $scope.ShowCreateTable = function(){
         SdbSignal.commit( 'showCreateTable' ) ;
      }

      //切换数据库
      $scope.ShowTableInfo = function( dbName ){
         $scope.CurrentDbName = dbName ;
         queryTableList( $scope.CurrentDbName ) ;
      } ;

      //进入数据操作页面
      $scope.GotoData = function( tableName ){
         SdbFunction.LocalData( 'MariaDBDbName', $scope.CurrentDbName ) ;
         SdbFunction.LocalData( 'MariaDBTbName', tableName ) ;
         $location.path( '/Data/SequoiaSQL/MariaDB/Operate/Data' ).search( { 'r': new Date().getTime() } ) ;
      }
   } ) ;

   //弹窗操作控制器
   sacApp.controllerProvider.register( 'Data.MariaDB.Database.Window.Ctrl', function( $scope, $timeout, SdbFunction, SdbSignal, SdbSwap, Loading ){
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

      var modalValue2Create = function( valueJson ){
         var rv = { 'table_options': {} } ;
         if( valueJson['type'] == 'normal' )
         {
            rv['table_options']['Compressed'] = valueJson['Compressed'] == 0 ? false : true ;
            if( rv['table_options']['Compressed'] == true )
            {
               if( valueJson['Compressed'] == 1 )
               {
                  rv['table_options']['CompressionType'] = 'snappy' ;
               }
               else if( valueJson['Compressed'] == 2 )
               {
                  rv['table_options']['CompressionType'] = 'lzw' ;
               }
            }
            rv['table_options']['ReplSize'] = valueJson['ReplSize'] ;
            if( valueJson['Group'] > 0 )
            {
               rv['table_options']['Group'] = $scope.GroupList[ valueJson['Group'] - 1 ]['GroupName'] ;
            }
         }
         else if( valueJson['type'] == 'range' )
         {
            rv['table_options']['ShardingType'] = 'range' ;
            rv['table_options']['ShardingKey'] = {} ;
            $.each( valueJson['ShardingKey'], function( index, field ){
               rv['table_options']['ShardingKey'][ field['field'] ] = field['sort'] ;
            } ) ;
            rv['table_options']['ReplSize'] = valueJson['ReplSize'] ;
            rv['table_options']['Compressed'] = valueJson['Compressed'] == 0 ? false : true ;
            if( rv['table_options']['Compressed'] == true )
            {
               if( valueJson['Compressed'] == 1 )
               {
                  rv['table_options']['CompressionType'] = 'snappy' ;
               }
               else if( valueJson['Compressed'] == 2 )
               {
                  rv['table_options']['CompressionType'] = 'lzw' ;
               }
            }
            if( valueJson['Group'] > 0 )
            {
               rv['table_options']['Group'] = $scope.GroupList[ valueJson['Group'] - 1 ]['GroupName'] ;
            }
            rv['table_options']['EnsureShardingIndex'] = valueJson['EnsureShardingIndex'] ;
         }
         else if( valueJson['type'] == 'hash' )
         {
            rv['table_options']['ShardingType'] = 'hash' ;
            rv['table_options']['ShardingKey'] = {} ;
            $.each( valueJson['ShardingKey'], function( index, field ){
               rv['table_options']['ShardingKey'][ field['field'] ] = field['sort'] ;
            } ) ;
            rv['table_options']['Partition'] = valueJson['Partition'] ;
            rv['table_options']['ReplSize'] = valueJson['ReplSize'] ;
            rv['table_options']['Compressed'] = valueJson['Compressed'] == 0 ? false : true ;
            if( rv['table_options']['Compressed'] == true )
            {
               if( valueJson['Compressed'] == 1 )
               {
                  rv['table_options']['CompressionType'] = 'snappy' ;
               }
               else if( valueJson['Compressed'] == 2 )
               {
                  rv['table_options']['CompressionType'] = 'lzw' ;
               }
            }
            rv['table_options']['AutoSplit'] = valueJson['AutoSplit'] ;
            if( valueJson['Group'] > 0 )
            {
               rv['table_options']['Group'] = $scope.GroupList[ valueJson['Group'] - 1 ]['GroupName'] ;
            }
            rv['table_options']['EnsureShardingIndex'] = valueJson['EnsureShardingIndex'] ;
         }
         else if( valueJson['type'] == 'main' )
         {
            rv['table_options']['IsMainCL'] = true ;
            rv['table_options']['ShardingKey'] = {} ;
            $.each( valueJson['ShardingKey'], function( index, field ){
               rv['table_options']['ShardingKey'][ field['field'] ] = field['sort'] ;
            } ) ;
            rv['table_options']['ReplSize'] = valueJson['ReplSize'] ;
         }
         return rv ;
      }

      //打开 创建数据表 弹窗
      SdbSignal.on( 'showCreateTable', function(){
         var listValue = [] ;
         $.each( $scope.DatabaseList, function( index, databaseName ){
            listValue.push( { 'key': databaseName['SCHEMA_NAME'], 'value': databaseName['SCHEMA_NAME'] } ) ;
         } ) ;

         var form2 = {
            'inputList': [
               {
                  "name": "type",
                  "webName": $scope.autoLanguage( '集合类型' ),
                  "type": "select",
                  "value": "normal",
                  "valid": [
                     { "key": $scope.autoLanguage( '普通' ), "value": "normal" },
                     { "key": $scope.autoLanguage( '水平范围分区' ), "value": "range" },
                     { "key": $scope.autoLanguage( '水平散列分区' ), "value": "hash" },
                     { "key": $scope.autoLanguage( '垂直分区' ), "value": "main" }         
                  ],
                  "onChange": function( name, key, value ){
                     if( value == 'normal' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form2 ;
                     }
                     else if( value == 'range' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form3 ;
                     }
                     else if( value == 'hash' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form4 ;
                     }
                     else if( value == 'main' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form5 ;
                     }
                     $scope.CreateTableWindow['config']['Form2']['inputList'][0]['value'] = value ;
                  }
               },
               {
                  "name": "ReplSize",
                  "webName":  $scope.autoLanguage( '一致性写副本数' ),
                  "type": "int",
                  "required": true,
                  "value": 1,
                  "valid": {
                     "min": -1,
                     "max": 7
                  }
               },
               {
                  "name": "Compressed",
                  "webName":  $scope.autoLanguage( '数据压缩' ),
                  "type": "select",
                  "required": false,
                  "value": 2,
                  "valid": [
                     { "key": $scope.autoLanguage( '关' ), "value": 0 },
                     { "key": 'Snappy', "value": 1 },
                     { "key": 'LZW', "value": 2 }
                  ]
               },
               {
                  "name": "Note",
                  "webName":  $scope.pAutoLanguage( '表注释' ),
                  "type": "string",
                  "value": ""
               }
            ]
         } ;

         var form3 = {
            'inputList': [
               {
                  "name": "type",
                  "webName": $scope.autoLanguage( '集合类型' ),
                  "type": "select",
                  "value": "range",
                  "valid": [
                     { "key": $scope.autoLanguage( '普通' ), "value": "normal" },
                     { "key": $scope.autoLanguage( '水平范围分区' ), "value": "range" },
                     { "key": $scope.autoLanguage( '水平散列分区' ), "value": "hash" },
                     { "key": $scope.autoLanguage( '垂直分区' ), "value": "main" }         
                  ],
                  "onChange": function( name, key, value ){
                     if( value == 'normal' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form2 ;
                     }
                     else if( value == 'range' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form3 ;
                     }
                     else if( value == 'hash' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form4 ;
                     }
                     else if( value == 'main' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form5 ;
                     }
                     $scope.CreateTableWindow['config']['Form2']['inputList'][0]['value'] = value ;
                  }
               },
               {
                  "name": "ShardingKey",
                  "webName":  $scope.autoLanguage( '分区键' ),
                  "required": true,
                  "desc": $scope.autoLanguage( '分区键是一个或多个集合列的有续集和，其中的值用来确定每个集合所述的数据分区。' ),
                  "type": "list",
                  "valid": {
                     "min": 1
                  },
                  "child":[
                     [
                        {
                           "name": "field",
                           "webName": $scope.autoLanguage( "字段名" ),
                           "type": "string",
                           "value": "",
                           "valid": {
                              "min": 1,
                              "regex": "^[^/$].*",
                              "ban": "."
                           }
                        },
                        {
                           "name": "sort",
                           "type": "select",
                           "value": 1,
                           "valid": [
                              { "key": $scope.autoLanguage( '升序' ), "value": 1 },
                              { "key": $scope.autoLanguage( '降序' ), "value": -1 }
                           ]
                        }
                     ]
                  ]
               },
               {
                  "name": "ReplSize",
                  "webName":  $scope.autoLanguage( '一致性写副本数' ),
                  "type": "int",
                  "required": true,
                  "value": 1,
                  "valid": {
                     "min": -1,
                     "max": 7
                  }
               },
               {
                  "name": "Compressed",
                  "webName":  $scope.autoLanguage( '数据压缩' ),
                  "type": "select",
                  "required": false,
                  "value": 2,
                  "valid": [
                     { "key": $scope.autoLanguage( '关' ), "value": 0 },
                     { "key": 'Snappy', "value": 1 },
                     { "key": 'LZW', "value": 2 }
                  ]
               },
               {
                  "name": "EnsureShardingIndex",
                  "webName":  $scope.autoLanguage( '分区索引' ),
                  "type": "select",
                  "required": false,
                  "desc": "",
                  "value": true,
                  "valid": [
                     { "key": $scope.autoLanguage( '开' ), "value": true },
                     { "key": $scope.autoLanguage( '关' ), "value": false }
                  ]
               },
               {
                  "name": "Note",
                  "webName":  $scope.pAutoLanguage( '表注释' ),
                  "type": "string",
                  "value": ""
               }
            ]
         } ;

         var form4 = {
            'inputList': [
               {
                  "name": "type",
                  "webName": $scope.autoLanguage( '集合类型' ),
                  "type": "select",
                  "value": "hash",
                  "valid": [
                     { "key": $scope.autoLanguage( '普通' ), "value": "normal" },
                     { "key": $scope.autoLanguage( '水平范围分区' ), "value": "range" },
                     { "key": $scope.autoLanguage( '水平散列分区' ), "value": "hash" },
                     { "key": $scope.autoLanguage( '垂直分区' ), "value": "main" }         
                  ],
                  "onChange": function( name, key, value ){
                     if( value == 'normal' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form2 ;
                     }
                     else if( value == 'range' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form3 ;
                     }
                     else if( value == 'hash' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form4 ;
                     }
                     else if( value == 'main' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form5 ;
                     }
                     $scope.CreateTableWindow['config']['Form2']['inputList'][0]['value'] = value ;
                  }
               },
               {
                  "name": "ShardingKey",
                  "webName":  $scope.autoLanguage( '分区键' ),
                  "required": true,
                  "desc": $scope.autoLanguage( '分区键是一个或多个集合列的有续集和，其中的值用来确定每个集合所述的数据分区。' ),
                  "type": "list",
                  "valid": {
                     "min": 1
                  },
                  "child":[
                     [
                        {
                           "name": "field",
                           "webName": $scope.autoLanguage( "字段名" ),
                           "type": "string",
                           "value": "",
                           "valid": {
                              "min": 1,
                              "regex": "^[^/$].*",
                              "ban": "."
                           }
                        },
                        {
                           "name": "sort",
                           "type": "select",
                           "value": 1,
                           "valid": [
                              { "key": $scope.autoLanguage( '升序' ), "value": 1 },
                              { "key": $scope.autoLanguage( '降序' ), "value": -1 }
                           ]
                        }
                     ]
                  ]
               },
               {
                  "name": "Partition",
                  "webName":  $scope.autoLanguage( '分区数' ),
                  "type": "int",
                  "required": true,
                  "value": 4096,
                  "valid": {
                     "min": 8,
                     "max": 1048576,
                     "step": 2
                  }
               },
               {
                  "name": "ReplSize",
                  "webName":  $scope.autoLanguage( '一致性写副本数' ),
                  "type": "int",
                  "required": true,
                  "value": 1,
                  "valid": {
                     "min": -1,
                     "max": 7
                  }
               },
               {
                  "name": "Compressed",
                  "webName":  $scope.autoLanguage( '数据压缩' ),
                  "type": "select",
                  "required": false,
                  "value": 2,
                  "valid": [
                     { "key": $scope.autoLanguage( '关' ), "value": 0 },
                     { "key": 'Snappy', "value": 1 },
                     { "key": 'LZW', "value": 2 }
                  ]
               },
               {
                  "name": "AutoSplit",
                  "webName":  $scope.autoLanguage( '自动切分' ),
                  "type": "select",
                  "value": false,
                  "valid": [
                     { "key": $scope.autoLanguage( '开' ), "value": true },
                     { "key": $scope.autoLanguage( '关' ), "value": false }
                  ]
               },
               {
                  "name": "EnsureShardingIndex",
                  "webName":  $scope.autoLanguage( '分区索引' ),
                  "type": "select",
                  "required": false,
                  "desc": "",
                  "value": true,
                  "valid": [
                     { "key": $scope.autoLanguage( '开' ), "value": true },
                     { "key": $scope.autoLanguage( '关' ), "value": false }
                  ]
               },
               {
                  "name": "Note",
                  "webName":  $scope.pAutoLanguage( '表注释' ),
                  "type": "string",
                  "value": ""
               }
            ]
         } ;

         var form5 = {
            'inputList': [
               {
                  "name": "type",
                  "webName": $scope.autoLanguage( '集合类型' ),
                  "type": "select",
                  "value": "main",
                  "valid": [
                     { "key": $scope.autoLanguage( '普通' ), "value": "normal" },
                     { "key": $scope.autoLanguage( '水平范围分区' ), "value": "range" },
                     { "key": $scope.autoLanguage( '水平散列分区' ), "value": "hash" },
                     { "key": $scope.autoLanguage( '垂直分区' ), "value": "main" }         
                  ],
                  "onChange": function( name, key, value ){
                     if( value == 'normal' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form2 ;
                     }
                     else if( value == 'range' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form3 ;
                     }
                     else if( value == 'hash' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form4 ;
                     }
                     else if( value == 'main' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form5 ;
                     }
                     $scope.CreateTableWindow['config']['Form2']['inputList'][0]['value'] = value ;
                  }
               },
               {
                  "name": "ShardingKey",
                  "webName":  $scope.autoLanguage( '分区键' ),
                  "required": true,
                  "desc": $scope.autoLanguage( '分区键是一个或多个集合列的有续集和，其中的值用来确定每个集合所述的数据分区。' ),
                  "type": "list",
                  "valid": {
                     "min": 1
                  },
                  "child":[
                     [
                        {
                           "name": "field",
                           "webName": $scope.autoLanguage( "字段名" ),
                           "type": "string",
                           "value": "",
                           "valid": {
                              "min": 1,
                              "regex": "^[^/$].*",
                              "ban": "."
                           }
                        },
                        {
                           "name": "sort",
                           "type": "select",
                           "value": 1,
                           "valid": [
                              { "key": $scope.autoLanguage( '升序' ), "value": 1 },
                              { "key": $scope.autoLanguage( '降序' ), "value": -1 }
                           ]
                        }
                     ]
                  ]
               },
               {
                  "name": "ReplSize",
                  "webName":  $scope.autoLanguage( '副本数' ),
                  "type": "int",
                  "required": true,
                  "value": 1,
                  "valid": {
                     "min": -1,
                     "max": 7
                  }
               },
               {
                  "name": "Note",
                  "webName":  $scope.pAutoLanguage( '表注释' ),
                  "type": "string",
                  "value": ""
               }
            ]
         } ;

         var form6 = {
            'inputList': [
               {
                  "name": "Note",
                  "webName":  $scope.pAutoLanguage( '表注释' ),
                  "type": "string",
                  "value": ""
               }
            ]
         }

         var form1 = {
            'inputList': [
               {
                  "name": "dbName",
                  "webName": $scope.pAutoLanguage( '数据库' ),
                  "type": "select",
                  "required": true,
                  "value": $scope.CurrentDbName,
                  "valid": listValue
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
                     "ban": '`'
                  }
               },
               {
                  "name": "engine",
                  "webName": $scope.pAutoLanguage( '存储引擎' ),
                  "type": "select",
                  "required": true,
                  "value": "SequoiaDB",
                  "valid": SdbSwap.engineList,
                  "onChange": function( name, key ){
                     if( key == 'SequoiaDB' )
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form2 ;
                     }
                     else
                     {
                        $scope.CreateTableWindow['config']['Form2'] = form6 ;
                     }
                  }
               },
               {
                  "name": "fields",
                  "webName":  $scope.pAutoLanguage( '字段结构' ),
                  "required": true,
                  "type": "list",
                  "desc": $scope.pAutoLanguage( '如字段类型是set或enum时，请在“长度/值”的输入框填写枚举的值，用半角逗号(,)隔开。' ),
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
                           "value": "int",
                           "valid": [
                              { "key": 'tinyint', "value": "tinyint" },
                              { "key": 'smallint', "value": "smallint" },
                              { "key": 'mediumint', "value": "mediumint" },
                              { "key": 'int', "value": "int" },
                              { "key": 'bigint', "value": "bigint" },
                              { "key": 'float', "value": "float" },
                              { "key": 'double', "value": "double" },
                              { "key": 'decimal', "value": "decimal" },
                              { "key": 'bit', "value": "bit" },
                              { "key": 'date', "value": "date" },
                              { "key": 'datetime', "value": "datetime" },
                              { "key": 'timestamp', "value": "timestamp" },
                              { "key": 'year', "value": "year" },
                              { "key": 'time', "value": "time" },
                              { "key": 'char', "value": "char" },
                              { "key": 'varchar', "value": "varchar" },
                              { "key": 'text', "value": "text" },
                              { "key": 'tinytext', "value": "tinytext" },
                              { "key": 'mediumtext', "value": "mediumtext" },
                              { "key": 'longtext', "value": "longtext" },
                              { "key": 'binary', "value": "binary" },
                              { "key": 'blob', "value": "blob" },
                              { "key": 'tinyblob', "value": "tinyblob" },
                              { "key": 'mediumblob', "value": "mediumblob" },
                              { "key": 'longblob', "value": "longblob" },
                              { "key": 'json', "value": "json" },
                              { "key": 'set', "value": "set" },
                              { "key": 'enum', "value": "enum" }
                           ]
                        },
                        {
                           "name": "length",
                           "type": "string",
                           "webName": $scope.pAutoLanguage( "长度/值" ),
                           "placeholder": $scope.pAutoLanguage( "长度/值" ),
                           "value": ""
                        },
                        {
                           "name": "default",
                           "webName": $scope.pAutoLanguage( "默认值" ),
                           "placeholder": $scope.pAutoLanguage( "默认值" ),
                           "type": "string"
                        },
                        {
                           "name": "indexType",
                           "type": "select",
                           "value": "null",
                           "valid": [
                              { "key": $scope.pAutoLanguage( "无索引" ), "value": "null" },
                              { "key": $scope.pAutoLanguage( "主键" ), "value": "primary" },
                              { "key": $scope.pAutoLanguage( "唯一索引" ), "value": "unique" },
                              { "key": $scope.pAutoLanguage( "普通索引" ), "value": "index" }
                           ]
                        },
                        {
                           "name": "attr",
                           "type": "select",
                           "value": "",
                           "valid": [
                              { "key": $scope.pAutoLanguage( "无属性" ), "value": "" },
                              { "key": $scope.pAutoLanguage( "无符号" ), "value": "unsigned" },
                              { "key": $scope.pAutoLanguage( "零填充" ), "value": "zerofill" }
                           ]
                        },
                        {
                           "name": "null",
                           "webName": $scope.pAutoLanguage( "空" ),
                           "type": "checkbox",
                           "value": true
                        }
                     ]
                  ]
               }
            ]
         } ;

         $scope.CreateTableWindow['config']['Form1'] = form1 ;
         $scope.CreateTableWindow['config']['Form2'] = form2 ;
         $scope.CreateTableWindow['callback']['SetTitle']( $scope.pAutoLanguage( '创建数据表' ) ) ;
         $scope.CreateTableWindow['callback']['SetIcon']( 'fa-plus' ) ;
         $scope.CreateTableWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
            var isClear1 = $scope.CreateTableWindow['config']['Form1'].check( function( formVal ){
               var rv = [] ;
               $.each( formVal['fields'], function( index, info ){
                  if( info['type'] == 'enum' || info['type'] == 'set' )
                  {
                     if( info['length'].length == 0 )
                     {
                        rv.push( { 'name': 'fields', 'error': $scope.pAutoLanguage( '字段参数错误。' ) } ) ;
                     }
                  }
                  else
                  {
                     if( ( info['length'].length > 0 &&  !( parseInt( info['length'] ) > 0 ) ) || ( info['length'] < 0 || info['length'] > 4294967295 ) )
                     {
                        rv.push( { 'name': 'fields', 'error': $scope.pAutoLanguage( '字段参数错误。' ) } ) ;
                     }
                  }
               } ) ;
               return rv ;
            } ) ;
            var isClear2 = $scope.CreateTableWindow['config']['Form2'].check() ;
            if( isClear1 == true && isClear2 ==true )
            {
               var formVal1 = $scope.CreateTableWindow['config']['Form1'].getValue() ;
               
               var sql = '' ; 
               var primaryKey = formVal1['tbName'] ;
               var primaryKey2 = '' ;
               var indexName = formVal1['tbName'] ;
               var uniqueName = formVal1['tbName'] ;
               var indexCont = '' ;
               var uniqueCont = '' ;
               $.each( formVal1['fields'], function( index, fieldInfo ){
                  var subSql = '' ;
                  if( index > 0 )
                  {
                     subSql += ', ' ;
                  }
                  subSql += '`' + fieldInfo['name'] + '` ' + fieldInfo['type'] ;
                  if( fieldInfo['length'].length > 0 )
                  {
                     switch( fieldInfo['type'] )
                     {
                     case 'varchar':
                     case 'character':
                     case 'char':
                     case 'decimal':
                     case 'numeric':
                     case 'int':
                     case 'bit':
                     case 'bigint':
                     case 'tinyint':
                     case 'smallint':
                     case 'mediumint':
                     case 'datetime':
                     case 'timestamp':
                     case 'time':
                     case 'binary':
                     case 'double':
                     case 'float':
                        subSql += '(' + fieldInfo['length'] + ') ' ;
                        break ;
                     case 'set':
                     case 'enum':
                        if( fieldInfo['length'].indexOf( ',' ) >= 0 )
                        {
                           var tmpArray = fieldInfo['length'].split( ',' ) ;
                           var first = true ;
                           subSql += '(' ;
                           $.each( tmpArray, function( index, value ){
                              if( !first )
                              {
                                 subSql += ',' ;
                              }
                              subSql = subSql + "'" + value + "'" ;
                              first = false ;
                           } ) ;
                           subSql += ')' ;
                        }
                        else
                        {
                           subSql += '(\'' + fieldInfo['length'] + '\')' ;
                        }
                     default:
                        subSql += ' ' ;
                        break ;
                     }
                  }
                  else
                  {
                     subSql += ' ' ;
                  }
                  if( !isEmpty( fieldInfo['attr'] ) )
                  {
                     switch( fieldInfo['type'] )
                     {
                     case 'tinyint':
                     case 'smallint':
                     case 'mediumint':
                     case 'int':
                     case 'double':
                     case 'bigint':
                     case 'decimal':
                     case 'float':
                        subSql += fieldInfo['attr'] + ' ' ;
                        break ;
                     default:
                        subSql += ' ' ;
                        break ;
                     }
                  }
                  if( fieldInfo['indexType'] == 'primary' )
                  {
                     //判断是否第一个主键
                     if( primaryKey2.length > 0 )
                     {
                        primaryKey2 += ',' ;
                     }
                     primaryKey += ( '_' + fieldInfo['name'] ) ;
                     primaryKey2 += '`' + fieldInfo['name'] + '`' ;
                  }
                  else if( fieldInfo['indexType'] == 'unique' )
                  {
                     if( uniqueCont.length > 0 )
                     {
                        uniqueCont += ',' ;
                     }
                     uniqueName += ( '_' + fieldInfo['name'] ) ;
                     uniqueCont += '`' + fieldInfo['name'] + '`' ;
                  }
                  else if( fieldInfo['indexType'] == 'index' )
                  {
                     if( indexCont.length > 0 )
                     {
                        indexCont += ',' ;
                     }
                     indexName += ( '_' + fieldInfo['name'] ) ;
                     indexCont += '`' + fieldInfo['name'] + '`' ;
                  }
                  if( fieldInfo['null'] == true && fieldInfo['indexType'] != 'primary' )
                  {
                     subSql += 'NULL' ;
                  }
                  else
                  {
                     subSql += 'NOT NULL' ;
                  }
                  if( isString( fieldInfo['default'] ) )
                  {
                     subSql += ' DEFAULT ' + sqlEscape( fieldInfo['default'] ) + ' ' ;
                  }
                  sql += subSql ;
               } ) ;
               
               //判断是否有索引
               if( primaryKey2.length > 0 )
               {
                  sql = sql + ',constraint `' + primaryKey + '` primary key (' + primaryKey2 + ')' ;
               }
               if( indexCont.length > 0 )
               {
                  sql = sql + ',index `' + indexName + '`(' + indexCont + ')' ;
               }
               if( uniqueCont.length > 0 )
               {
                  sql = sql + ',unique index `' + uniqueName + '`(' + uniqueCont + ')' ;
               }
               sql = sprintf( 'create table `?` (', formVal1['tbName'] ) + sql + ' )' ;

               //添加引擎
               sql += ' engine = ' + formVal1['engine'] ;

               var formVal2 = $scope.CreateTableWindow['config']['Form2'].getValue() ;
               var tableNote = formVal2['Note'] ;
               //如果是SequoiaDB引擎
               if( formVal1['engine'] == 'SequoiaDB' )
               {
                  formVal2 = modalValue2Create( formVal2 ) ;
                  if( tableNote.length > 0 )
                  {
                     sql += sprintf( " comment = '?", tableNote ) ;
                     sql += sprintf( ", sequoiadb:?'", JSON.stringify( formVal2 ) ) ;
                  }
                  else
                  {
                     sql += sprintf( " comment = 'sequoiadb:?'", JSON.stringify( formVal2 ) ) ;
                  }
               }
               else
               {
                  sql += sprintf( " comment = '?'", tableNote ) ;
               }
               $scope.createTable( sql, formVal1['dbName'] ) ;
               $scope.CreateTableWindow['callback']['Close']() ;
            }
         } ) ;
         $scope.CreateTableWindow['callback']['Open']() ;
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
                     "ban": '`'
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
         if( $scope.DatabaseList.length == 3 )
         {
            return ;
         }
         var listValue = [] ;
         var defaultDb = null ;
         $.each( $scope.DatabaseList, function( index, databaseName ){
            if( defaultDb == null )
            {
               defaultDb = databaseName["SCHEMA_NAME"] ;
            }
            if( databaseName["SCHEMA_NAME"] != 'information_schema' &&
                databaseName["SCHEMA_NAME"] != 'mysql' &&
                databaseName["SCHEMA_NAME"] != 'performance_schema' )
            {
               listValue.push( { 'key': databaseName["SCHEMA_NAME"], 'value': databaseName["SCHEMA_NAME"] } ) ;
            }
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
         var sql = '' ;
         sql = sprintf( 'drop table `?`', tableName ) ;
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
      $scope.ShowRemoveTableWindow = function(){
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
               sql = sprintf( 'drop table `?`', tableName ) ;
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
                     "ban": '`'
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
               var sql = sprintf( 'alter table `?` rename to `?`', formVal['oldTbName'], formVal['newTbName'] ) ;
               SdbSignal.commit( 'AlterTable', sql ) ;
               $scope.AlterTableWindow['callback']['Close']() ;
            }
         } ) ;
         $scope.AlterTableWindow['callback']['Open']() ;
      } ) ;

   } ) ;

}());