/******************************************************************************
 * @Description   : seqDB-22859:修改数据源属性，包含不可修改参数 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.02.04
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22859";

   clearDataSource( "nocs", dataSrcName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dataSource = db.getDataSource( dataSrcName );
   var options = { "ID": 300 };
   Alter( dataSource, options );

   var options = { "Type": "MySQL" };
   Alter( dataSource, options );

   var options = { "Version": 50 };
   Alter( dataSource, options );

   // jira SEQUOIADBMAINSTREAM-6686
   // var options = { "DSVersion": "3.4.1" };
   // Alter( dataSource, options );

   var options = { "AccessMode": "READ", "Type": "MySQL" };
   Alter( dataSource, options );

   clearDataSource( "nocs", dataSrcName );
}

function Alter ( dataSource, options, errorCode )
{
   if( typeof ( errorCode ) == "undefined" ) { errorCode = SDB_OPTION_NOT_SUPPORT; }
   assert.tryThrow( [errorCode], function() 
   {
      dataSource.alter( options );
   } );
}