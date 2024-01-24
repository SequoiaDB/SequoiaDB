//@ sourceURL=Structure.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Data.PostgreSQL.Structure.Ctrl', function( $scope, $location, $compile, SdbFunction, SdbRest, SdbSwap, SdbSignal ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      $scope.ModuleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      SdbSwap.dbName = SdbFunction.LocalData( 'PgsqlDbName' ) ;
      SdbSwap.tbName = SdbFunction.LocalData( 'PgsqlTbName' ) ;
      if( clusterName == null || moduleType != 'sequoiasql-postgresql' || $scope.ModuleName == null || SdbSwap.tbName == null || SdbSwap.dbName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }
      $scope.FullName   = SdbSwap.dbName + '.' + SdbSwap.tbName ;
      SdbSwap.tbType    = SdbFunction.LocalData( 'PgsqlTbType' ) ;
      var indexList = [] ;
      var primaryKey = '' ;

      //获取字段信息
      $scope.QueryTableStruct = function(){
         var sql = sprintf("SELECT \
                   ordinal_position, \
                   column_name, \
                   data_type, \
                   character_maximum_length, \
                   numeric_precision, \
                   numeric_scale, \
                   is_nullable, \
                   column_default \
                   FROM \
                   information_schema.columns \
                   WHERE \
                   table_name = '?'", SdbSwap.tbName ) ;
         var data = { 'Sql': sql, 'DbName': SdbSwap.dbName, 'IsAll': 'true' } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( fieldList ){
               SdbSignal.commit( 'setTableData', fieldList ) ;
               if( SdbSwap.tbType == 'table' )
               {
                  SdbSignal.commit( 'setPrimarySelect', fieldList ) ;
                  var sql = sprintf( 'SELECT A.SCHEMANAME,A.TABLENAME,A.INDEXNAME,C.INDISUNIQUE,C.INDISPRIMARY FROM PG_AM B LEFT JOIN PG_CLASS F ON B.OID = F.RELAM LEFT JOIN PG_STAT_ALL_INDEXES E ON F.OID = E.INDEXRELID LEFT JOIN PG_INDEX C ON E.INDEXRELID = C.INDEXRELID LEFT OUTER JOIN PG_DESCRIPTION D ON C.INDEXRELID = D.OBJOID, PG_INDEXES A WHERE A.SCHEMANAME = E.SCHEMANAME AND A.TABLENAME = E.RELNAME AND A.INDEXNAME = E.INDEXRELNAME AND E.SCHEMANAME = \'public\' AND E.RELNAME = \'?\'', SdbSwap.tbName ) ;
                  execSql( sql, true ) ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  $scope.QueryTableStruct() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      $scope.QueryTableStruct() ;
      
      //执行sql
      var execSql = function( sql, isQueryIndex ){
         var data = { 'Sql': sql, 'DbName': SdbSwap.dbName } ;
         SdbRest.DataOperationV2( '/sql', data, {
            'success': function( result ){
               //重新查询字段列表
               if( isQueryIndex )
               {
                  primaryKey = '' ;
                  indexList = [] ;
                  if( result.length > 0 )
                  {
                     $.each( result, function( index, info ){
                        if( info['indisprimary'] == 't' )
                        {
                           primaryKey = info['indexname'] ;
                           return ;
                        }
                        else
                        {
                           indexList.push( { 'indexname': info['indexname'] } ) ;
                        }
                     } ) ;
                     //$scope.PrimaryKey = result[0]['pk_name'] ;
                  }
               }
               else
               {
                  $scope.QueryTableStruct() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  execSql( sql ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //添加字段 弹窗
      $scope.AddFieldWindow = {
         'config': {},
         'callback': {}
      }
      var appendOnly = false ;

      //打开 添加字段 弹窗
      $scope.ShowAddField = function(){
         $scope.AddFieldWindow['config'] = {
            'inputList': [
               {
                  "name": "fields",
                  "webName": $scope.pAutoLanguage( '字段' ),
                  "required": true,
                  "type": "list",
                  "child":[
                     [
                        {
                           "name": "name",
                           "webName": $scope.pAutoLanguage( "字段名" ),
                           "placeholder": $scope.pAutoLanguage( "字段名" ),
                           "type": "string",
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
                           "webName": $scope.pAutoLanguage( "长度" ),
                           "placeholder": $scope.pAutoLanguage( "长度" ),
                           "type": "int",
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
                           "type": "string",
                           "valid": ""
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
         $scope.AddFieldWindow['callback']['SetOkButton']( $scope.pAutoLanguage('确定'), function(){
            var isClear = $scope.AddFieldWindow['config'].check() ;
            if( isClear )
            {
               var value = $scope.AddFieldWindow['config'].getValue() ;
               var sql = sprintf( 'ALTER TABLE ? ', addQuotes( SdbSwap.tbName ) ) ;
               $.each( value['fields'], function( index, fieldInfo ){
                  var subSql = '' ;
                  if( index > 0 )
                  {
                     subSql += ', ' ;
                  }
                  subSql += 'ADD ' + addQuotes( fieldInfo['name'] ) + ' ' + fieldInfo['type'] ;
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
               execSql( sql ) ;
               $scope.AddFieldWindow['callback']['Close']() ;
            }
         } ) ;
         $scope.AddFieldWindow['callback']['SetIcon']( '' ) ;
         $scope.AddFieldWindow['callback']['SetTitle']( $scope.pAutoLanguage( '添加字段' ) ) ;
         $scope.AddFieldWindow['callback']['Open']() ;
      } ;

      $scope.FieldsSelect = [] ;

      //设置主键 弹窗
      $scope.SetPrimaryWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 设置主键 弹窗
      var showSetPrimary = function(){
         if( primaryKey.length == 0 )
         {
            $scope.SetPrimaryWindow['config'] = {
               'inputList': [
                  {
                     "name": "fields",
                     "webName": $scope.pAutoLanguage( '字段' ),
                     "required": true,
                     "type": "list",
                     "child":[
                        [
                           {
                              "name": "field",
                              "type": "select",
                              "value": $scope.FieldsSelect[0]['key'],
                              "default": $scope.FieldsSelect[0]['key'],
                              "valid": $scope.FieldsSelect
                           }
                        ]
                     ]
                  }
               ]
            } ;
            $scope.SetPrimaryWindow['callback']['SetOkButton']( $scope.pAutoLanguage('确定'), function(){
               var sql = sprintf( 'ALTER TABLE ? ADD PRIMARY KEY ', addQuotes( SdbSwap.tbName ) ) ;
               var formValue = $scope.SetPrimaryWindow['config'].getValue() ;
               var isFrist = true ;
               var existList = {} ;
               $.each( formValue['fields'], function( index, field ){
                  if( typeof( existList[field['field']] ) == 'undefined' )
                  {
                     existList[field['field']] = true ;
                     if( isFrist )
                     {
                        sql += '(' ;
                        isFrist = false ;
                     }
                     else
                     {
                        if( existList[field['field']] )
                        sql += ',' ;
                     }
                     sql += addQuotes( field['field'] ) ;
                  }
               } ) ;
               sql += ')' ;
               execSql( sql ) ;
               $scope.SetPrimaryWindow['callback']['Close']() ;
            } ) ;
            $scope.SetPrimaryWindow['callback']['SetIcon']( '' ) ;
            $scope.SetPrimaryWindow['callback']['SetTitle']( $scope.pAutoLanguage( '设置主键' ) ) ;
            $scope.SetPrimaryWindow['callback']['Open']() ;
         }
      }

      //移除主键
      var showDropPrimary = function(){
         if( primaryKey.length > 0 )
         {
            $scope.Components.Confirm.type = 3 ;
            $scope.Components.Confirm.context = sprintf( $scope.pAutoLanguage( '是否确定删除主键：?？' ), primaryKey) ;
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.okText = $scope.pAutoLanguage( '确定' ) ;
            $scope.Components.Confirm.ok = function(){
               var sql = sprintf( 'alter table ? drop constraint ?', addQuotes( SdbSwap.tbName ), addQuotes( primaryKey ) ) ;
               execSql( sql ) ;
               $scope.Components.Confirm.isShow = false ;
            }
         }
      }

      //删除字段 弹窗
      $scope.DropFieldWindow = {
         'config': {},
         'callback': {}
      } ;
      //索引操作 下拉菜单
      $scope.IndexDropdown = {
         'config': [],
         'callback': {}
      } ;

      //打开 索引操作
      $scope.ShowIndexDropdown = function( event ){
         $scope.IndexDropdown['config'] = [
            { 'key': $scope.pAutoLanguage( '创建索引' ) },
            { 'key': $scope.pAutoLanguage( '删除索引' ), 'disabled': indexList.length>0 ? false : true },
            { 'key': $scope.pAutoLanguage( '设置主键' ), 'disabled': primaryKey.length>0 },
            { 'key': $scope.pAutoLanguage( '移除主键' ), 'disabled': primaryKey.length==0 }
         ] ;
         $scope.IndexDropdown['OnClick'] = function( index ){
            if( index == 0 )
            {
               showCreateIndex() ;
            }
            else if( index == 1 )
            {
               showDropIndex() ;
            }
            else if( index == 2 )
            {
               showSetPrimary() ;
            }
            else
            {
               showDropPrimary() ;
            }

            $scope.IndexDropdown['callback']['Close']() ;
         }
         $scope.IndexDropdown['callback']['Open']( event.currentTarget ) ;
      }

      //创建索引 弹窗
      $scope.CreateIndexWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 创建索引
      var showCreateIndex = function(){
         $scope.CreateIndexWindow['config'] = {
            'inputList': [
               {
                  "name": "fields",
                  "webName": $scope.pAutoLanguage( '字段' ),
                  "required": true,
                  "type": "list",
                  "child":[
                     [
                        {
                           "name": "field",
                           "type": "select",
                           "value": $scope.FieldsSelect[0]['key'],
                           "default": $scope.FieldsSelect[0]['key'],
                           "valid": $scope.FieldsSelect
                        }
                     ]
                  ]
               },
               {
                  "name": "type",
                  "webName": $scope.pAutoLanguage( '索引类型' ),
                  "required": true,
                  "type": "select",
                  "value": "normal",
                  "valid": [
                     { 'key': $scope.pAutoLanguage( '普通索引' ), 'value': 'normal' },
                     { 'key': $scope.pAutoLanguage( '唯一索引' ), 'value': 'unique' }
                  ]
               }
            ]
         }
         $scope.CreateIndexWindow['callback']['SetTitle']( $scope.pAutoLanguage( '创建索引' ) ) ;
         $scope.CreateIndexWindow['callback']['SetIcon']( 'fa-edit' ) ;
         $scope.CreateIndexWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
            var sql = '' ;
            var formValue = $scope.CreateIndexWindow['config'].getValue() ;
            if( formValue['type'] == 'normal' )
            {
               sql = sprintf( 'create index on ?', addQuotes( SdbSwap.tbName ) ) ; 
            }
            else
            {
               sql = sprintf( 'create unique index on ?', addQuotes( SdbSwap.tbName ) ) ; 
            }
            var isFrist = true ;
            var existList = {} ;
            $.each( formValue['fields'], function( index, field ){
               if( typeof( existList[field['field']] ) == 'undefined' )
               {
                  existList[field['field']] = true ;
                  if( isFrist )
                  {
                     sql += '(' ;
                     isFrist = false ;
                  }
                  else
                  {
                     if( existList[field['field']] )
                     sql += ',' ;
                  }
                  sql += addQuotes( field['field'] ) ;
               }
            } ) ;
            sql += ')' ;

            execSql( sql ) ;
            $scope.CreateIndexWindow['callback']['Close']() ;
         } ) ;
         $scope.CreateIndexWindow['callback']['Open']() ;
      }

      SdbSignal.on( 'setPrimarySelect', function( fieldList ){
         $scope.FieldsSelect = [] ;
         if( fieldList.length > 0 )
         {
            $.each( fieldList, function( index, field ){
               $scope.FieldsSelect.push( { 'key': field['column_name'], 'value': field['column_name'] } ) ;
            } ) ;
         }
      } ) ;

      //删除索引 弹窗
      $scope.DropIndexWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 删除索引
      var showDropIndex = function(){
         if( indexList.length > 0 )
         {
            var selectList = [] ;
            $.each( indexList, function( index, info ){
               selectList.push( { 'key': info['indexname'], 'value': info['indexname'] } ) ;
            } ) ;
            $scope.DropIndexWindow['config'] = {
               'inputList': [
                  {
                     "name": "index",
                     "webName": $scope.pAutoLanguage( '索引名' ),
                     "required": true,
                     "type": "select",
                     "value": selectList[0]['value'],
                     "valid": selectList
                  }
               ]
            } ;
            $scope.DropIndexWindow['callback']['SetTitle']( $scope.pAutoLanguage( '删除索引' ) ) ;
            $scope.DropIndexWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
               var formValue = $scope.DropIndexWindow['config'].getValue() ;
               var sql = '' ;
               sql = sprintf( 'drop index ?', addQuotes( formValue['index'] ) ) ;
               execSql( sql ) ;
               $scope.DropIndexWindow['callback']['Close']() ;
            } ) ;
            $scope.DropIndexWindow['callback']['Open']() ;
         }
      }

      //打开 删除字段 弹窗
      SdbSignal.on( 'ShowDropFieldWindow', function( fieldName ){
         $scope.Components.Confirm.type = 3 ;
         $scope.Components.Confirm.context = sprintf( $scope.pAutoLanguage( '是否确定删除字段：?？' ), fieldName ) ;
         $scope.Components.Confirm.isShow = true ;
         $scope.Components.Confirm.okText = $scope.pAutoLanguage( '确定' ) ;
         $scope.Components.Confirm.ok = function(){
            var sql = sprintf( 'alter table ? drop column ?', addQuotes( SdbSwap.tbName ), addQuotes( fieldName ) ) ;
            execSql( sql ) ;
            $scope.Components.Confirm.isShow = false ;
         }
      } ) ;

      //重命名字段 弹窗
      $scope.RenameFieldWindow = {
         'config': {},
         'callback': {}
      } ;
      
      //打开 重命名字段 弹窗
      var showRenameField = function( fieldName ){
         $scope.RenameFieldWindow['config'] = {
            'inputList': [
               {
                  "name": "oldFieldName",
                  "webName": $scope.pAutoLanguage( '原字段名' ),
                  "type": "string",
                  "required": true,
                  "disabled": true,
                  "value": fieldName
               },
               {
                  "name": "newFieldName",
                  "webName": $scope.pAutoLanguage( '新字段名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1,
                     "max": 63,
                     "regex": "^[a-zA-Z_]+[0-9a-zA-Z_]*$",
                     "regexError": sprintf( $scope.pAutoLanguage( '?由字母和数字或\"_\"组成，并且以字母或\"_\"起头。' ), $scope.pAutoLanguage( '字段名' ) )
                  }
               }
            ]
         }
         $scope.RenameFieldWindow['callback']['SetTitle']( $scope.pAutoLanguage( '修改字段名' ) ) ;
         $scope.RenameFieldWindow['callback']['SetIcon']( 'fa-edit' ) ;
         $scope.RenameFieldWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
            var isClear = $scope.RenameFieldWindow['config'].check() ;
            if( isClear == true )
            {
               var formVal = $scope.RenameFieldWindow['config'].getValue() ;
               var sql = sprintf( 'alter table ? rename column ? to ?', addQuotes( SdbSwap.tbName ), addQuotes( fieldName ), addQuotes( formVal['newFieldName'] ) ) ;
               execSql( sql ) ;
               $scope.RenameFieldWindow['callback']['Close']() ;
            }
         } ) ;
         $scope.RenameFieldWindow['callback']['Open']() ;
      }

      //设置字段默认值 弹窗
      $scope.SetfFieldDefaultWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 设置字段默认值 弹窗
      var showSetDefault = function( fieldName ){
         $scope.SetfFieldDefaultWindow['config'] = {
            'inputList': [
               {
                  "name": "fieldName",
                  "webName": $scope.pAutoLanguage( '字段名' ),
                  "type": "string",
                  "required": true,
                  "disabled": true,
                  "value": fieldName
               },
               {
                  "name": "default",
                  "webName": $scope.pAutoLanguage( '默认值' ),
                  "type": "string",
                  "value": "",
                  "valid": {
                     "min": 1
                  }
               }
            ]
         }
         $scope.SetfFieldDefaultWindow['callback']['SetTitle']( $scope.pAutoLanguage( '设置默认值' ) ) ;
         $scope.SetfFieldDefaultWindow['callback']['SetIcon']( 'fa-edit' ) ;
         $scope.SetfFieldDefaultWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
            var isClear = $scope.SetfFieldDefaultWindow['config'].check() ;
            if( isClear == true )
            {
               var formVal = $scope.SetfFieldDefaultWindow['config'].getValue() ;
               var sql = sprintf( 'alter table ? alter column ? set default ?', addQuotes( SdbSwap.tbName ), addQuotes( fieldName ), sqlEscape( formVal['default'] ) ) ;
               execSql( sql ) ;
               $scope.SetfFieldDefaultWindow['callback']['Close']() ;
            }
         } ) ;
         $scope.SetfFieldDefaultWindow['callback']['Open']() ;
      }

      //打开 删除字段默认值 弹窗
      var shwoRemoveDefault = function( fieldName ){
         $scope.Components.Confirm.type = 3 ;
         $scope.Components.Confirm.context = sprintf( $scope.pAutoLanguage( '是否确定删除字段 ? 的默认值？' ), fieldName ) ;
         $scope.Components.Confirm.isShow = true ;
         $scope.Components.Confirm.okText = $scope.pAutoLanguage( '确定' ) ;
         $scope.Components.Confirm.ok = function(){
            var sql = sprintf( 'alter table ? alter column ? drop default', addQuotes( SdbSwap.tbName ), addQuotes( fieldName ) ) ;
            execSql( sql ) ;
            $scope.Components.Confirm.isShow = false ;
         }
      }

      //修改字段类型 弹窗
      $scope.SetfFieldTypetWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 修改字段类型 弹窗
      var showSetFieldType = function( fieldName, type, length ){
         if( length === null )
         {
            length = '' ;
         }
         $scope.SetfFieldTypetWindow['config'] = {
            'inputList': [
               {
                  "name": "fieldName",
                  "webName": $scope.pAutoLanguage( '字段名' ),
                  "type": "string",
                  "required": true,
                  "disabled": true,
                  "value": fieldName
               },
               {
                  "name": "newType",
                  "webName": $scope.pAutoLanguage( '字段类型' ),
                  "type": "select",
                  "value": type,
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
                  "webName": $scope.pAutoLanguage( "长度" ),
                  "type": "int",
                  "value": length,
                  "valid": {
                     "min": 0,
                     "empty": true
                  }
               }
            ]
         }
         $scope.SetfFieldTypetWindow['callback']['SetTitle']( $scope.pAutoLanguage( '修改字段类型' ) ) ;
         $scope.SetfFieldTypetWindow['callback']['SetIcon']( 'fa-edit' ) ;
         $scope.SetfFieldTypetWindow['callback']['SetOkButton']( $scope.pAutoLanguage( '确定' ), function(){
            var isClear = $scope.SetfFieldTypetWindow['config'].check() ;
            if( isClear == true )
            {
               var formVal = $scope.SetfFieldTypetWindow['config'].getValue() ;
               var sql = sprintf( 'alter table ? alter column ? type ?', addQuotes( SdbSwap.tbName ), addQuotes( fieldName ), formVal['newType'] ) ;
               if( formVal['length'] > 0 )
               {
                  sql = sprintf( '?(?)', sql, formVal['length'] ) ;
               }
               execSql( sql ) ;
               $scope.SetfFieldTypetWindow['callback']['Close']() ;
            }
         } ) ;
         $scope.SetfFieldTypetWindow['callback']['Open']() ;
      }

      //编辑字段 下拉菜单
      $scope.EditFieldDropdown = {
         'config': [
            { 'key': $scope.pAutoLanguage( '修改字段名' ) },
            { 'key': $scope.pAutoLanguage( '修改字段类型' ) },
            { 'key': $scope.pAutoLanguage( '设置默认值' ) },
            { 'key': $scope.pAutoLanguage( '删除默认值' ) }
         ],
         'callback': {}
      } ;

      //打开 编辑字段 下拉菜单
      SdbSignal.on( 'ShowEditFieldDropdown', function( result ){
         $scope.EditFieldDropdown['OnClick'] = function( index ){
            if( index == 0 )
            {
               showRenameField( result['field'] ) ;
            }
            else if( index == 1 )
            {
               showSetFieldType( result['field'], result['type'], result['length'] ) ;
            }
            else if( index == 2 )
            {
               showSetDefault( result['field'] ) ;
            }
            else
            {
               shwoRemoveDefault( result['field'] ) ;
            }
            $scope.EditFieldDropdown['callback']['Close']() ;
         }
         $scope.EditFieldDropdown['callback']['Open']( result['event'].currentTarget ) ;
      } ) ;

   } ) ;

   //表格 控制器
   sacApp.controllerProvider.register( 'Data.PostgreSQL.Structure.Table.Ctrl', function( $scope, SdbSwap, SdbSignal ){
      //表格
      $scope.GridTable = {
         'title': {
            'index' : '#',
            'column_name'      : $scope.pAutoLanguage( '字段名' ),
            'operation'        : '',
            'data_type'        : $scope.pAutoLanguage( '类型' ),
            'column_default'   : $scope.pAutoLanguage( '默认值' ),
            'is_nullable'      : $scope.pAutoLanguage( '空' )
         },
         'body': [],
         'options': {
            'width': {
               'index' : '35px',
               'column_name'      : '25%',
               'operation'        : '60px',
               'data_type'        : '25%',
               'column_default'   : '25%',
               'is_nullable'      : '25%'
            },
            'sort': {
               'index' : true,
               'column_name'      : true,
               'operation'        : false,
               'data_type'        : true,
               'column_default'   : true,
               'is_nullable'      : true
            },
            'max': 50
         }
      } ;


      SdbSignal.on( 'setTableData', function( result ){
         $scope.GridTable['body'] = result ;
      } ) ;

      //打开 编辑字段 下拉菜单
      $scope.ShowEditFieldDropdown = function( event, fieldName, fieldType, fieldLength ){
         SdbSignal.commit( 'ShowEditFieldDropdown', { 'event': event, 'field': fieldName, 'type': fieldType, 'length': fieldLength } ) ;
      }

      //打开 删除字段 弹窗
      $scope.ShowDropFieldWindow = function( fieldName ){
         SdbSignal.commit( 'ShowDropFieldWindow', fieldName ) ;
      }

   } ) ;

   
}());