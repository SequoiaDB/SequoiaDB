//@ sourceURL=Deploy.AddHost.HostInfo.Ctrl.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.AddHost.HostInfo.Ctrl', function( $scope, $rootScope, SdbSignal ){

      $scope.HostInfo = {} ;

      $scope.CpuChart      = { 'percent': 0, 'text': '' } ;
      $scope.MemoryChart   = { 'percent': 0, 'text': '' } ;
      $scope.DiskChart     = { 'percent': 0, 'text': '' } ;

      $scope.MySQLTableHeight = 32 ;
      $scope.PGTableHeight = 32 ;
      $scope.DiskTableHeight = 32 ;
      $scope.PortTableHeight = 32 ;
      $scope.ServiceTableHeight = 32 ;
      $scope.DiskTable = {
         'title': {
            'IsUse':    '',
            'Name':     $scope.autoLanguage( '磁盘' ),
            'Mount':    $scope.autoLanguage( '挂载路径' ),
            'IsLocal':  $scope.autoLanguage( '本地磁盘' ),
            'Chart':    $scope.autoLanguage( '容量' )
         },
         'options': {
            'width': {
               'IsUse':    '40px',
               'Name':     '30%',
               'Mount':    '30%',
               'IsLocal':  '80px',
               'Chart':    '40%'
            },
            'sort': {
               'Name':     true,
               'Mount':    true,
               'IsLocal':  true
            },
            'max': 10000,
            'tools': false
         },
         'body': []
      } ;
      $scope.PortTable = {
         'title': {
            'Port':  $scope.autoLanguage( '端口' ),
            'CanUse': $scope.autoLanguage( '状态' )
         },
         'options': {
            'width': {
               'Port':     '40%',
               'CanUse':   '60%'
            },
            'max': 10000,
            'tools': false
         },
         'body': []
      } ;
      $scope.ServiceTable = {
         'title': {
            'Name':        $scope.autoLanguage( '服务名' ),
            'IsRunning':   $scope.autoLanguage( '状态' ),
            'Version':     $scope.autoLanguage( '版本' )
         },
         'options': {
            'width': {
               'Name':        '40%',
               'IsRunning':   '30%',
               'Version':     '30%'
            },
            'max': 10000,
            'tools': false
         },
         'body': []
      } ;
      //添加自定义路径 弹窗
      $scope.AddCustomPathWindow = {
         'config': {
            'inputList': [
               {
                  "name": "Name",
                  "webName": $scope.autoLanguage( '磁盘名' ),
                  "required": true,
                  "type": "string",
                  "value": '',
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": "Mount",
                  "webName": $scope.autoLanguage( '挂载路径' ),
                  "required": true,
                  "type": "string",
                  "value": '',
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": "Free",
                  "webName": $scope.autoLanguage( '可用容量(MB)' ),
                  "required": true,
                  "type": "int",
                  "value": '',
                  "valid": {
                     "min": 1
                  }
               },
               {
                  "name": "Size",
                  "webName": $scope.autoLanguage( '总容量(MB)' ),
                  "required": true,
                  "type": "int",
                  "value": '',
                  "valid": {
                     "min": 1
                  }
               }
            ]
         },
         'callback': {}
      } ;

      //选择颜色
      function switchColor( percent )
      {
         if( percent <= 60 )
         {
            return '#00CC66' ;
         }
         else if( percent <= 80 )
         {
            return '#FF9933' ;
         }
         else
         {
            return '#D9534F' ;
         }
      }

      //计算表格高度
      function getTableHeight( num )
      {
         if( num <= 5 )
         {
            return ( num + 1 ) * 31 + 1 ;
         }
         else
         {
            return 218 ;
         }
      }

      //计算普通表格高度
      function getTableHeight2( num )
      {
         if( num <= 5 )
         {
            return num * 30 + 30 ;
         }
         else
         {
            return 210 ;
         }
      }

      //磁盘
      function checkDiskList( diskList )
      {
         var diskFree = 0 ;
         var diskSize = 0 ;
         $.each( diskList, function( index, diskInfo ){
            if( diskInfo['IsLocal'] == true )
            {
               diskSize += diskInfo['Size'] ;
               diskFree += diskInfo['Free'] ;
            }
            var tmpDiskPercent = diskInfo['Size'] == 0 ? 0 : parseInt( ( diskInfo['Size'] - diskInfo['Free'] ) * 100 / diskInfo['Size'] ) ;
            diskInfo['Chart'] = {
               'percent': tmpDiskPercent,
               'style': {
                  'progress': {
                     'background': switchColor( tmpDiskPercent )
                  }
               },
               'text': sprintf( '? / ?', sizeConvert( diskInfo['Size'] - diskInfo['Free'] ), sizeConvert( diskInfo['Size'] ) )
            } ;
         } ) ;
         var disk = parseInt( ( diskSize - diskFree ) * 100 / diskSize ) ;
         $scope.DiskChart = {
            'percent': disk,
            'style': {
               'progress': {
                  'background': switchColor( disk )
               }
            },
            'text': sprintf( '? / ?', sizeConvert( diskSize - diskFree ), sizeConvert( diskSize ) )
         } ;

         $scope.DiskTable['body'] = diskList ;
         $scope.DiskTableHeight = getTableHeight( diskList.length ) ;
      }

      //端口
      function checkPortList( portList )
      {
         $scope.PortTable['body'] = [] ;
         $.each( portList, function( index, portInfo ){
            if( portInfo['Port'].length > 0 )
            {
               $scope.PortTable['body'].push( portInfo ) ;
            }
         } ) ;
         $scope.PortTableHeight = getTableHeight( $scope.PortTable['body'].length ) ;
      }

      //服务
      function checkServiceList( serviceList )
      {
         $scope.ServiceTable['body'] = [] ;
         $.each( serviceList, function( index, serviceInfo ){
            if( serviceInfo['Name'].length > 0 )
            {
               $scope.ServiceTable['body'].push( serviceInfo ) ;
            }
         } ) ;
         $scope.ServiceTableHeight = getTableHeight( $scope.ServiceTable['body'].length ) ;
      }

      //cpu
      function checkCpu( cpuInfo )
      {
         var cpu = parseInt( ( cpuInfo['Other'] + cpuInfo['Sys'] + cpuInfo['User'] ) * 100 / ( cpuInfo['Idle'] + cpuInfo['Other'] + cpuInfo['Sys'] + cpuInfo['User'] ) ) ;
         $scope.CpuChart = { 'percent': cpu, 'style': { 'progress': { 'background': switchColor( cpu ) } } } ;
      }

      //内存
      function checkMemory( memoryInfo )
      {
         var memory = parseInt( ( memoryInfo['Size'] - memoryInfo['Free'] ) * 100 / memoryInfo['Size'] ) ;
         $scope.MemoryChart = {
            'percent': memory,
            'style': {
               'progress': {
                  'background': switchColor( memory )
               }
            },
            'text': sprintf( '? / ?', sizeConvert( memoryInfo['Size'] - memoryInfo['Free'] ), sizeConvert( memoryInfo['Size'] ) )
         } ;
      }

      //mysql
      $scope.MySQLTable = {
         'title': {
            'IsUse':    '',
            'Version':  $scope.autoLanguage( '版本' ),
            'SdbUser':  $scope.autoLanguage( '用户' ),
            'Path':     $scope.autoLanguage( '安装路径' )
         },
         'options': {
            'width': {
               'IsUse':    '40px',
               'Version':  '20%',
               'SdbUser':  '30%',
               'Path':     '50%'
            },
            'max': 10000,
            'tools': false
         },
         'body': []
      } ;

      function checkMySQL( mysqlInfo )
      {
         if ( mysqlInfo.length > 0 )
         {
            $scope.MySQLTable['body'] = mysqlInfo ;
            $scope.MySQLTableHeight = getTableHeight2( mysqlInfo.length ) ;
         }
         else
         {
            $scope.MySQLTable['body'] = [] ;
            $scope.HostInfo['MYSQL'] = {} ;
         }
      }

      $scope.onMySQLChange = function( index ){
         $.each( $scope.MySQLTable['body'], function( index2, info ){
            $scope.MySQLTable['body'][index2]['IsUse'] = index == index2 ;
         } ) ;

         $scope.HostInfo['MYSQL'] = {
            'Version':  $scope.MySQLTable['body'][index]['Version'],
            'SdbUser':  $scope.MySQLTable['body'][index]['SdbUser'],
            'Path':     $scope.MySQLTable['body'][index]['Path']
         } ;
      }

      //postgresql
      $scope.PGTable = {
         'title': {
            'IsUse':    '',
            'Version':  $scope.autoLanguage( '版本' ),
            'SdbUser':  $scope.autoLanguage( '用户' ),
            'Path':     $scope.autoLanguage( '安装路径' )
         },
         'options': {
            'width': {
               'IsUse':    '40px',
               'Version':  '20%',
               'SdbUser':  '30%',
               'Path':     '50%'
            },
            'max': 10000,
            'tools': false
         },
         'body': []
      } ;

      function checkPGSQL( pgInfo )
      {
         if ( pgInfo.length > 0 )
         {
            $scope.PGTable['body'] = pgInfo ;
            $scope.PGTableHeight = getTableHeight2( pgInfo.length ) ;
         }
         else
         {
            $scope.PGTable['body'] = [] ;
            $scope.HostInfo['POSTGRESQL'] = {} ;
         }
      }

      $scope.onPGChange = function( index ){
         $.each( $scope.PGTable['body'], function( index2, info ){
            $scope.PGTable['body'][index2]['IsUse'] = index == index2 ;
         } ) ;

         $scope.HostInfo['POSTGRESQL'] = {
            'Version':  $scope.PGTable['body'][index]['Version'],
            'SdbUser':  $scope.PGTable['body'][index]['SdbUser'],
            'Path':     $scope.PGTable['body'][index]['Path']
         } ;
      }

      SdbSignal.on( 'UpdateHostConfig', function( hostInfo ){
         $scope.HostInfo = hostInfo ;
         if( typeof( hostInfo['errno'] ) == 'undefined' || hostInfo['errno'] == 0 )
         {
            checkMySQL( hostInfo['MySQLList'] ) ;
            checkPGSQL( hostInfo['PGList'] ) ;
            checkCpu( hostInfo['CPU'] ) ;
            checkMemory( hostInfo['Memory'] ) ;
            checkDiskList( hostInfo['Disk'] ) ;
            checkPortList( hostInfo['Port'] ) ;
            checkServiceList( hostInfo['Service'] ) ;
         }
         else
         {
            $scope.CpuChart      = { 'percent': 0 } ;
            $scope.MemoryChart   = { 'percent': 0 } ;
            $scope.DiskChart     = { 'percent': 0 } ;
            $scope.DiskTable['body']    = [] ;
            $scope.PortTable['body']    = [] ;
            $scope.ServiceTable['body'] = [] ;
         }
         $rootScope.bindResize() ;
      } ) ;

      //打开 添加自定义路径 弹窗
      $scope.ShowAddCustomPath = function(){
         $.each( $scope.AddCustomPathWindow['config']['inputList'], function( index, inputInfo ){
            inputInfo['value'] = '' ;
         } ) ;
         $scope.AddCustomPathWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.AddCustomPathWindow['config'].check( function( formVal ){
               var error = [] ;
               if( formVal['Free'] > formVal['Size'] )
               {
                  error.push( { 'name': 'Free', 'error': sprintf( $scope.autoLanguage( '?的值不能大于?。' ), $scope.autoLanguage( '可用容量' ), $scope.autoLanguage( '总容量' ) ) } ) ;
                  error.push( { 'name': 'Size', 'error': sprintf( $scope.autoLanguage( '?的值不能小于?。' ), $scope.autoLanguage( '总容量' ), $scope.autoLanguage( '可用容量' ) ) } ) ;
               }
               else
               {
                  if( formVal['Free'] < 600 )
                  {
                     error.push( { 'name': 'Free', 'error': sprintf( $scope.autoLanguage( '?的值不能小于?。' ), $scope.autoLanguage( '可用容量' ), '600MB' ) } ) ;
                  }
                  if( formVal['Size'] < 600 )
                  {
                     error.push( { 'name': 'Size', 'error': sprintf( $scope.autoLanguage( '?的值不能小于?。' ), $scope.autoLanguage( '总容量' ), '600MB' ) } ) ;
                  }
               }
               return error ;
            } ) ;
            if( isAllClear )
            {
               var formVal = $scope.AddCustomPathWindow['config'].getValue() ;
               formVal['IsUse']   = true ;
               formVal['CanUse']  = true ;
               formVal['IsLocal'] = true ;
               var tmpDiskPercent = formVal['Size'] == 0 ? 0 : parseInt( ( formVal['Size'] - formVal['Free'] ) * 100 / formVal['Size'] ) ;
               formVal['Chart'] = {
                  'percent': tmpDiskPercent,
                  'style': {
                     'progress': {
                        'background': switchColor( tmpDiskPercent )
                     }
                  },
                  'text': sprintf( '? / ?', sizeConvert( formVal['Size'] - formVal['Free'] ), sizeConvert( formVal['Size'] ) )
               } ;
               $scope.HostInfo['Disk'].push( formVal ) ;
               ++$scope.HostInfo['IsUseNum'] ;
               $scope.HostInfo['CanUse'] = true ;
               checkDiskList( $scope.HostInfo['Disk'] ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.AddCustomPathWindow['callback']['SetTitle']( $scope.autoLanguage( '添加自定义路径' ) ) ;
         $scope.AddCustomPathWindow['callback']['Open']() ;
      }

      //计算磁盘
      $scope.CountCheckedDisk = function(){
         setTimeout( function(){
            $scope.HostInfo['IsUseNum'] = 0 ;
            $.each( $scope.HostInfo['Disk'], function( index, diskInfo ){
               if( diskInfo['IsUse'] == true )
               {
                  ++$scope.HostInfo['IsUseNum'] ;
                  return false ;
               }
            } ) ;
            if( $scope.HostInfo['IsUseNum'] > 0 )
            {
               $scope.HostInfo['CanUse'] = true ;
               $scope.HostInfo['background'] = '#fff' ;
            }
            else
            {
               if( $scope.HostInfo['IsUse'] == true )
               {
                  $scope.HostInfo['IsUse'] = false ;
               }
               $scope.HostInfo['CanUse'] = false ;
               $scope.HostInfo['background'] = '#eee' ;
            }
            SdbSignal.commit( 'CheckedHostNum', null ) ;
            $scope.$apply() ;
         } ) ;
      }
   } ) ;
}());