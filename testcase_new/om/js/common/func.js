import( "loadConf.js" ) ;

if( typeof( IS_DEBUG )  == 'undefined' ){ IS_DEBUG = false ; }
if( typeof( OMHOSTNAME  )  == 'undefined' ){ OMHOSTNAME = System.getHostName() ; }
if( typeof( OMWEBNAME )   == 'undefined' ){ OMWEBNAME = "8000" ; }
if( typeof( OMSVCNAME )   == 'undefined' ){ OMSVCNAME = "11780" ; }
if( typeof( OM_USER )   == 'undefined' ){ OM_USER = "admin" ; }
if( typeof( OM_PASSWD ) == 'undefined' ){ OM_PASSWD = "admin" ; }
if( typeof( TESTCASE_CONF ) == 'undefined' ){ TESTCASE_CONF = "testcase.conf" ; }

var SDB_TESTCASE_CONF = new SdbTestcaseConf( catPath( getSelfPath(), "../" + TESTCASE_CONF ) ) ;
var HOST_LIST = SDB_TESTCASE_CONF.getHostList() ;