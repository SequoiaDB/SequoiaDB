
var _operate = {} ;

_operate.condition = function( $scope, webName ){
   return {
      "name": "filter",
      "webName": $scope.autoLanguage( webName ),
      "type": "group",
      "child": [
         {
            "name": "model",
            "webName": $scope.autoLanguage( "匹配模式" ),
            "type": "select",
            "value": "and",
            "valid": [
               { "key": $scope.autoLanguage( "满足所有条件" ), "value": "and" },
               { "key": $scope.autoLanguage( "满足任意条件" ), "value": "or" }
            ]
         },
         {
            "name": "condition",
            "webName": $scope.autoLanguage( "匹配条件" ),
            "type": "list",
            "valid": {
               "min": 0
            },
            "child": [
               [
                  {
                     "name": "field",
                     "webName": $scope.autoLanguage( "字段名" ),
                     "placeholder": $scope.autoLanguage( "字段名" ),
                     "type": "string",
                     "value": "",
                     "valid": {
                        "min": 1,
                        "regex": "^[^/$].*",
                        "ban": "."
                     },
                     "selectList": $scope.fieldList
                  },
                  {
                     "name": "logic",
                     "type": "select",
                     "value": ">",
                     "default": ">",
                     "valid": [
                        { "key": ">", "value": ">" },
                        { "key": ">=", "value": ">=" },
                        { "key": "<", "value": "<" },
                        { "key": "<=", "value": "<=" },
                        { "key": "!=", "value": "!=" },
                        { "key": "=", "value": "=" },
                        { "key": "size", "value": "size" },
                        { "key": "regex", "value": "regex" },
                        { "key": "type", "value": "type" },
                        { "key": "IS NULL", "value": "null" },
                        { "key": "IS NOT NULL", "value": "notnull" },
                        { "key": "IS EXISTS", "value": "exists" },
                        { "key": "IS NOT EXISTS", "value": "notexists" }
                     ]
                  },
                  {
                     "name": "value",
                     "webName": $scope.autoLanguage( "值" ),
                     "placeholder": $scope.autoLanguage( "值" ),
                     "type": "string",
                     "value": "",
                     "valid": {}
                  }
               ]
            ]
         }
      ]
   } ;
}

_operate.selector = function( $scope ){
   return {
      "name": "selector",
      "webName": $scope.autoLanguage( "选择字段" ),
      "type": "list",
      "valid": {
         "min": 0
      },
      "child": [
         [
            {
               "name": "field",
               "webName": $scope.autoLanguage( "字段名" ), 
               "placeholder": $scope.autoLanguage( "字段名" ),
               "type": "string",
               "valid": {
                  "min": 1,
                  "regex": "^[^/$].*",
                  "ban": "."
               },
               "value": "",
               "selectList": $scope.fieldList
            }
         ]
      ]
   } ;
}

_operate.sort = function( $scope ){
   return {
      "name": "sort",
      "webName": $scope.autoLanguage( "排序字段" ),
      "type": "list",
      "valid": {
         "min": 0,
         "max": 100
      },
      "child": [
         [
            {
               "name": "field",
               "webName": $scope.autoLanguage( "字段名" ),
               "placeholder": $scope.autoLanguage( "字段名" ),
               "type": "string",
               "valid": {
                  "min": 1,
                  "regex": "^[^/$].*",
                  "ban": "."
               },
               "value": "",
               "selectList": $scope.fieldList
            },
            {
               "name": "order",
               "type": "select",
               "value": "1",
               "valid": [
                  { "key": $scope.autoLanguage( "升序" ), "value": "1" },
                  { "key": $scope.autoLanguage( "降序" ), "value": "-1" }
               ]
            }
         ]
      ]
   } ;
}

_operate.hint = function( $scope, indexList ){
   return {
      "name": "hint",
      "webName": $scope.autoLanguage( "扫描方式" ),
      "type": "select",
      "value": 0,
      "valid": indexList
   } ;
}

_operate.returnNum = function( $scope ){
   return {
      "name": "returnnum",
      "webName": $scope.autoLanguage( "返回记录数" ),
      "required": true,
      "type": "int",
      "valid": {
         "min": 1,
         "max": 100
      },
      "value": 30
   } ;
}

_operate.skip = function( $scope ){
      return {
         "name": "skip",
         "webName": $scope.autoLanguage( "跳过记录数" ),
         "required": true,
         "type": "int",
         "valid": {
            "min": 0,
            //2^53 - 1
            "max": 9007199254740991
         },
         "value": 0
      } ;
   }

_operate.updator = function( $scope ){
   return {
      "name": "updator",
      "webName": $scope.autoLanguage( "更新操作" ),
      "type": "list",
      "desc": "",
      "required": true,
      "valid": {
         "min": 1
      },
      "child": [
         [
            {
               "name": "field",
               "webName": $scope.autoLanguage( "字段名" ),
               "placeholder": $scope.autoLanguage( "字段名" ),
               "type": "string",
               "value": "",
               "valid": {
                  "min": 1,
                  "regex": "^[^/$].*",
                  "ban": "."
               },
               "selectList": $scope.fieldList
            },
            {
               "name": "value",
               "webName": $scope.autoLanguage( "值" ),
               "placeholder": $scope.autoLanguage( "值" ),
               "type": "string",
               "value": "",
               "valid": {}
            }
         ]
      ]
   } ;
}

_operate.selectCL = function( $scope, clValid ){
   return {
      "name": "clName",
      "webName": $scope.autoLanguage( '集合' ),
      "type": "select",
      "value": $scope.clID,
      "valid": clValid
   }
}

_operate.indexName = function( $scope ){
   return {
      "name": "indexName",
      "webName": $scope.autoLanguage( '索引名' ),
      "type": "string",
      "desc": $scope.autoLanguage( '索引名，同一个集合中的索引名必须唯一。' ),
      "required": true,
      "value": "",
      "valid": {
         "min": 1,
         "max": 127,
         "ban": [
            ".",
            "$"
         ]
      }
   } ;
}

_operate.indexDef = function( $scope ){
   return {
      "name":"indexKey",
      "webName": $scope.autoLanguage( '索引键' ),
      "required": true,
      "desc": $scope.autoLanguage( '索引键，包含一个或多个指定索引字段与方向的对象。' ),
      "type": "list",
      "child":[
         [
            {
               "name": "field",
               "webName": $scope.autoLanguage( "字段名" ),
               "placeholder": $scope.autoLanguage( "字段名" ),
               "type": "string",
               "value": "",
               "valid": {
                  "min": 1,
                  "regex": "^[^/$].*",
                  "ban": "."
               }
            },
            {
               "name": "order",
               "type": "select",
               "value": "1",
               "valid": [
                  { "key": $scope.autoLanguage( '升序' ), "value": "1" },
                  { "key": $scope.autoLanguage( '降序' ), "value": "-1" }
               ]
            }
         ]
      ]
   } ;
}

_operate.unique = function( $scope ){
   return {
      "name": "isUnique",
      "webName": $scope.autoLanguage( '索引是否唯一' ),
      "type": "select",
      "value": false,
      "valid": [
         { "key": $scope.autoLanguage( '是' ), "value": true },
         { "key": $scope.autoLanguage( '否' ), "value": false }
      ]
   } ;
}

_operate.enforced = function( $scope ){
   return {
      "name": "enforced",
      "webName": $scope.autoLanguage( '索引是否强制唯一' ),
      "type": "select",
      "desc": "",
      "value": false,
      "valid": [
         { "key": $scope.autoLanguage( '是' ), "value": true },
         { "key": $scope.autoLanguage( '否' ), "value": false }
      ]
   } ;
}

_operate.sortbuffersize = function( $scope ){
   return {
      "name": "sortbuffersize",
      "webName": $scope.autoLanguage( '索引排序大小' ) + '(MB)',
      "type": "int",
      "value": 64,
      "valid": {
         "min": 0
      }
   } ;
}