import( "common/om.js" ) ;
import( "common/unit.js" ) ;

var OM_CTRL = new SdbOMCtrl( OMHOSTNAME, OMWEBNAME ) ;
var UNIT_TEST = new SdbUnit( "Test host" ) ;

UNIT_TEST.setUp( function(){
   OM_CTRL.login( OM_USER, OM_PASSWD ) ;
} ) ;

UNIT_TEST.test( 'query host status', function(){
   var clusterList = OM_CTRL.query_cluster() ;

   for( var i in clusterList )
   {
      var clusterName = clusterList[i]['ClusterName'] ;

      var hostList = OM_CTRL.query_host( { 'ClusterName': clusterName } ) ;
      if( hostList.length == 0 )
      {
         continue ;
      }

      var hostStatus = OM_CTRL.query_host_status( hostList ) ;
      UNIT_TEST.assert( hostList.length, hostStatus[0]['HostInfo'].length, "invalid host status" ) ;
   }
} ) ;

UNIT_TEST.start( IS_DEBUG ) ;



