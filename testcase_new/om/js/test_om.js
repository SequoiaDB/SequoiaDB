import( "common/om.js" ) ;
import( "common/unit.js" ) ;

var OM_CTRL = new SdbOMCtrl( OMHOSTNAME, OMWEBNAME ) ;
var UNIT_TEST = new SdbUnit( "Test host" ) ;

UNIT_TEST.setUp( function(){
   OM_CTRL.login( OM_USER, OM_PASSWD ) ;
} ) ;

UNIT_TEST.test( 'get system info', function(){
   var info = OM_CTRL.get_system_info() ;
   UNIT_TEST.assert( 1, info.length, "invalid system info" ) ;
} ) ;

UNIT_TEST.test( 'list plugins', function(){
   OM_CTRL.list_plugins() ;
} ) ;

UNIT_TEST.test( 'change passwd', function(){
   OM_CTRL.change_passwd( OM_USER, OM_PASSWD, "123" ) ;

   OM_CTRL.login( OM_USER, "123" ) ;

   OM_CTRL.change_passwd( OM_USER, "123", OM_PASSWD ) ;

   OM_CTRL.login( OM_USER, OM_PASSWD ) ;
} ) ;

UNIT_TEST.start( IS_DEBUG ) ;



