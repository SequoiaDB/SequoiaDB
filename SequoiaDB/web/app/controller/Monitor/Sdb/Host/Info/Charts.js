//@ sourceURL=Charts.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Performance.Charts.Index.Ctrl', function( $scope, SdbRest, $location, SdbFunction ){
      
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var hostName = SdbFunction.LocalData( 'SdbHostName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      if( hostName == null )
      {
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      $scope.HostName = hostName ;
      $scope.HostStatus = null ;
      var lastCPU = null ;
      var diskSize = 0 ;
      var diskUsed = 0 ;
      var diskPercent = 0 ;
      var networkIn = [] ;
      var networkOut = [] ;
      var networkInValue = [] ;
      var networkOutValue = [] ;
      var netInChart = 0 ;
      var netOutChart = 0 ;

      $scope.charts = {};
      $scope.charts['Storage'] = {} ;
      $scope.charts['Storage']['options'] = window.SdbSacManagerConf.DiskStorageEchart ;
      $scope.charts['Storage']['options']['title']['text'] = $scope.autoLanguage( '磁盘利用率' ) ;

      $scope.charts['network'] = {} ;
      $scope.charts['network']['options'] = window.SdbSacManagerConf.NetwordIOEchart ;
      $scope.charts['network']['options']['title']['text'] = $scope.autoLanguage( '网络流量' ) ;

      $scope.charts['memory'] = {} ;
      $scope.charts['memory']['options'] = window.SdbSacManagerConf.MemoryEchart ;
      $scope.charts['memory']['options']['title']['text'] = $scope.autoLanguage( '内存利用率' ) ;

      $scope.charts['Cpu'] = {} ;
      $scope.charts['Cpu']['options'] = window.SdbSacManagerConf.CpuEchart ;
      $scope.charts['Cpu']['options']['title']['text'] = $scope.autoLanguage( 'CPU利用率' ) ;

      var getChartInfo = function(){
         var data = {
            'cmd':'query host status',
            'HostInfo': JSON.stringify( { "HostInfo": [ { 'HostName': hostName } ] } )
         } ;
         SdbRest.OmOperation( data, {
            'success': function( hostList ){
               if( hostList.length > 0 )
               {
                  chartDetail = hostList[0]['HostInfo'][0] ;
                  if( isNaN( chartDetail['errno'] ) == false && chartDetail['errno'] != 0 )
                  {
                     $scope.HostStatus = false ;
                     return ;
                  }
                  $scope.HostStatus = true ;

                  //当前磁盘占用率
                  if( isArray( chartDetail['Disk'] ) )
                  {
                     $.each( chartDetail['Disk'], function( index, diskInfo ){
                        diskUsed += diskInfo['Size'] - diskInfo['Free'] ;
                        diskSize += diskInfo['Size'] ;
                     } ) ;
                  }
                  diskPercent = fixedNumber( ( diskUsed/diskSize ) * 100, 2 ) ;

                  //网卡
                  netInChart = 0 ;
                  netOutChart = 0 ;
                  $.each( chartDetail['Net']['Net'], function( index, netInfo){
                     if( typeof( networkIn[index] ) == 'undefined' &&  typeof( networkOut[index] ) == 'undefined' )
                     {
                        networkIn[index] = netInfo['RXBytes']['Megabit'] * 1024  + netInfo['RXBytes']['Unit'] / 1024 ;
                        networkOut[index] = netInfo['TXBytes']['Megabit'] * 1024  + netInfo['TXBytes']['Unit'] / 1024 ;
                     }
                     else
                     {
                        networkInValue[index] = netInfo['RXBytes']['Megabit'] * 1024 + netInfo['RXBytes']['Unit'] / 1024 - networkIn[index]  ;
                        networkOutValue[index] = netInfo['TXBytes']['Megabit'] * 1024 + netInfo['TXBytes']['Unit'] / 1024 - networkOut[index]  ;

                        networkIn[index] = netInfo['RXBytes']['Megabit'] * 1024  + netInfo['RXBytes']['Unit'] / 1024 ;
                        networkOut[index] = netInfo['TXBytes']['Megabit'] * 1024  + netInfo['TXBytes']['Unit'] / 1024 ;
                     }

                  } ) ;
                  $.each( networkInValue, function( inIndex, inValue ){
                     netInChart += inValue ;
                  } ) ;
                  $.each( networkOutValue, function( outIndex, outValue ){
                     netOutChart += outValue ;
                  } ) ;

                  netInChart  = fixedNumber( netInChart  / 5, 2 ) ;
                  netOutChart = fixedNumber( netOutChart / 5, 2 ) ;

                  $scope.charts['network']['value'] = [ [ 0, netInChart, true, false ], [ 1, netOutChart, true, false ] ] ;
                  
                  //计算CPU
                  if( lastCPU === null )
                  {
                     lastCPU = getCpuUsePercent( [ chartDetail ] ) ;
                     $scope.charts['Cpu']['value'] = [ [ 0, 0, true, false ] ] ;
                  }
                  else
                  {
                     $scope.charts['Cpu']['value'] = [ [ 0, getCpuUsePercent( [ chartDetail ], lastCPU ), true, false ] ] ;
                  }

                  //内存占用率
                  memoryPencent = fixedNumber( chartDetail['Memory']['Used'] / chartDetail['Memory']['Size'] * 100, 2 ) ;
                  $scope.charts['memory']['value'] = [ [ 0, memoryPencent, true, false ] ] ;

                  //磁盘利用率
                  $scope.charts['Storage']['value'] = [ [ 0, diskPercent, true, false ] ] ;
               }
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': true
         } ) ;
      }

      getChartInfo() ;
      
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