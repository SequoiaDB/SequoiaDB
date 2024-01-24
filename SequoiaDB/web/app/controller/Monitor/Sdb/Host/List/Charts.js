//@ sourceURL=Charts.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Monitor.HostPerformance.Index.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
      
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      var hostNameList = [] ;
      var cpuSum = 0 ;
      var memorySum = 0 ;
      var cpuUsed = 0 ;
      var memoryUsed = 0 ;
      var networkIn = 0 ;
      var networkOut = 0 ;
      var diskSum = 0 ;
      var DiskSize = 0 ;
      var DiskFree = 0 ;
      var lastCPU = [] ;
      var sumRX = [] ;
      var sumTX = [] ;
      var chartInfo = [] ;
      $scope.NewHostList = [] ;

      var getHostList = function(){
         var data = {
            'cmd':'query host status',
            'HostInfo': JSON.stringify( {"HostInfo":hostNameList } )
         } ;
         SdbRest.OmOperation( data, {
            'success': function( hostList ){
               chartInfo = {
                  'CPU': 0,
                  'MemorySize': 0,
                  'MemoryUsed': 0,
                  'NetIn': 0,
                  'NetOut': 0,
                  'DiskSize': 0,
                  'DiskUsed': 0
               } ;
               if( hostList.length > 0 )
               {
                  $.each( hostList[0]['HostInfo'], function( index, hostInfo ){
                     if( isNaN( hostInfo['errno'] ) == false && hostInfo['errno'] != 0 )
                     {
                        return true ;
                     }

                     //计算CPU
                     if( lastCPU.length < hostList[0]['HostInfo'].length )
                     {
                        lastCPU[index] = getCpuUsePercent( [ hostInfo ] ) ;
                        $scope.NewHostList[index]['CPUUsed'] = 0 ;
                     }
                     else
                     {
                        $scope.NewHostList[index]['CPUUsed'] = getCpuUsePercent( [ hostInfo ], lastCPU[index] ) ;
                     }

                     //计算内存
                     $scope.NewHostList[index]['MemorySize'] = hostInfo['Memory']['Size'] / 1024 ;
                     $scope.NewHostList[index]['MemoryUsed'] = hostInfo['Memory']['Used'] / 1024 ;

                     //计算磁盘
                     $.each( hostInfo['Disk'], function( diskIndex, diskInfo ){
                        $scope.NewHostList[index]['DiskSize'] += ( diskInfo['Size']/1024 ) ;
                        $scope.NewHostList[index]['DiskUsed'] += ( diskInfo['Size']/1024 - diskInfo['Free']/1024 ) ;
                     } ) ;

                     //计算网络
                     $.each( hostInfo['Net']['Net'], function( netIndex, netInfo ){
                        if( typeof( sumTX[index][netIndex] ) == 'undefined' || typeof( sumRX[index][netIndex] ) == 'undefined' )
                        {
                           $scope.NewHostList[index]['NetIn'][netIndex] = 0 ;
                           $scope.NewHostList[index]['NetOut'][netIndex] = 0 ;
                           sumRX[index][netIndex] = 0 ;
                           sumTX[index][netIndex] = 0 ;
                        }
                        else
                        {
                           
                           $scope.NewHostList[index]['NetIn'][netIndex] = netInfo['RXBytes']['Megabit'] * 1024 + netInfo['RXBytes']['Unit']/ 1024 - sumRX[index][netIndex]  ;
                           $scope.NewHostList[index]['NetOut'][netIndex] = netInfo['TXBytes']['Megabit'] * 1024 + netInfo['TXBytes']['Unit']/ 1024 - sumTX[index][netIndex]  ;
                        }
                          
                        sumRX[index][netIndex] = netInfo['RXBytes']['Megabit'] * 1024 + netInfo['RXBytes']['Unit'] / 1024  ;
                        sumTX[index][netIndex] = netInfo['TXBytes']['Megabit'] * 1024 + netInfo['TXBytes']['Unit'] / 1024  ;
                     } ) ;
                  } ) ;

                  $.each( $scope.NewHostList, function( HostIndex, HostInfo ){
                     //磁盘
                     chartInfo['DiskSize'] += HostInfo['DiskSize'] ;
                     chartInfo['DiskUsed'] += HostInfo['DiskUsed'] ;
                     //内存
                     chartInfo['MemorySize'] += HostInfo['MemorySize'] ;
                     chartInfo['MemoryUsed'] += HostInfo['MemoryUsed'] ;
                     //CPU
                     chartInfo['CPU'] += HostInfo['CPUUsed'] ;
                     //网络
                     $.each( HostInfo['NetIn'], function( netInIndex, netInValue ){
                        chartInfo['NetIn'] += netInValue ;
                     } ) ;
                     $.each( HostInfo['NetOut'], function( netOutIndex, netOutValue ){
                        chartInfo['NetOut'] += netOutValue ;
                     } ) ;
                  } ) ;
                  //CPU图表数据
                  $scope.Charts['Cpu']['value'] = [ [ 0, fixedNumber( chartInfo['CPU'] / $scope.NewHostList.length, 2 ), true, false ] ] ;
                  //内存图表数据
                  $scope.Charts['Memory']['value'] = [ [ 0, fixedNumber( chartInfo['MemoryUsed'] / chartInfo['MemorySize'] * 100, 2 ), true, false ] ] ;
                  //磁盘图表数据
                  $scope.Charts['Disk']['value'] = [ [ 0, fixedNumber( chartInfo['DiskUsed'] / chartInfo['DiskSize'] * 100, 2 ), true, false ] ] ;
                  //网络图表数据
                  $scope.Charts['Network']['value'] = [ [ 0, fixedNumber( chartInfo['NetIn'] / 5, 2 ), true, false ], [ 1, fixedNumber( chartInfo['NetOut'] / 5, 2 ), true, false ] ] ;

               }
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': true
         } ) ;
      } ;
      
      var getModuleInfo = function(){
         var data = {
            'cmd': 'query business',
            'filter' : JSON.stringify( { 'BusinessName': moduleName } )
         } ;
         SdbRest.OmOperation( data, {
            'success': function( moduleInfo ){
               $.each( moduleInfo[0]['Location'], function( index, hostName ){
                  hostNameList.push( { 'HostName': hostName['HostName'] } ) ;
                  sumTX[index] = [] ;
                  sumRX[index] = [] ;
                  $scope.NewHostList.push(
                     {
                        'DiskSize': 0,
                        'DiskUsed': 0,
                        'MemorySize': 0,
                        'MemoryUsed': 0,
                        'CPUUsed': 0,
                        'NetInValue' : 0,
                        'NetOutValue' : 0,
                        'NetIn': [],
                        'NetOut': []
                     }
                  ) ;
               } ) ;
               getHostList() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getModuleInfo() ;
                  return true ;
               } ) ;
            }
         },{
            'showLoading': false
         } ) ;
      } ;
      getModuleInfo() ;



      $scope.Charts = {}; 
      $scope.getData = function(){
         var s = 0 ;
         var d = 0 ;
         SdbFunction.Interval(function(){
            $scope.Charts['Disk']['value'] = [ [ 0, d, true, false ] ] ;

            $scope.Charts['Network']['value'] = [ [ 0, 0, true, false ],[ 1, 0, true, false ] ] ;
            

            $scope.Charts['Memory']['value'] = [ [ 0, 0, true, false ],[ 1, 0, true, false ] ] ;

         },5000)
      }
      //$scope.getData() ;

      $scope.Charts['Disk'] = {} ;
      $scope.Charts['Disk']['options'] = window.SdbSacManagerConf.DiskStorageEchart ;
      $scope.Charts['Disk']['options']['title']['text'] = $scope.autoLanguage( '磁盘利用率' ) ;

      $scope.Charts['Network'] = {} ;
      $scope.Charts['Network']['options'] = window.SdbSacManagerConf.NetworkIOEchart ;
      $scope.Charts['Network']['options']['title']['text'] = $scope.autoLanguage( '网络流量' ) ;

      $scope.Charts['Memory'] = {} ;
      $scope.Charts['Memory']['options'] = window.SdbSacManagerConf.MemoryEchart ;
      $scope.Charts['Memory']['options']['title']['text'] = $scope.autoLanguage( '内存利用率' ) ;

      $scope.Charts['Cpu'] = {} ;
      $scope.Charts['Cpu']['options'] = window.SdbSacManagerConf.CpuEchart ;
      $scope.Charts['Cpu']['options']['title']['text'] = $scope.autoLanguage( 'CPU利用率' ) ;


      //跳转至资源
      $scope.GotoResources = function(){
         $location.path( '/Monitor/SDB-Resources/Session' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至主机列表
      $scope.GotoHosts = function(){
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;
      
      
      //跳转至节点列表
      $scope.GotoNodes = function(){
         if( moduleMode == 'distribution' )
         {
            $location.path( '/Monitor/SDB-Nodes/Nodes' ).search( { 'r': new Date().getTime() } ) ;
         }
         else
         {
            $location.path( '/Monitor/SDB-Nodes/Node/Index' ).search( { 'r': new Date().getTime() } ) ;
         }
      } ;
   } ) ;

}());