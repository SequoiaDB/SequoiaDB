/************************************************************************
@Description : create index, Insert index key length is maximum,eg: pagesize is 4096,the maximum length of the index key is 1024
@Modify list :
               2019-3-27  wuyan
************************************************************************/

main( test );

function test ()
{
   var csName = CHANGEDPREFIX + "_indexcs18049";
   var clName = CHANGEDPREFIX + "_indexcl18049";
   commDropCS( db, csName, true, "clear cs in the beginning" );

   var options = { PageSize: 4096 };
   db.createCS( csName, options );
   var dbcl = commCreateCL( db, csName, clName );
   dbcl.createIndex( "idxa", { 'stra': -1 }, true );

   var maxLen = 1011;
   var strValue = getRandomString( maxLen );
   dbcl.insert( { 'stra': strValue, 'no': 1 } );

   checkResult( dbcl, { 'stra': strValue, 'no': 1 } );

   commDropCS( db, csName, true, "clear cs in the ending" );;
}

function getRandomString ( len )
{
   var str = "abcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()_+";
   var tmp = "";
   var l = str.length;
   for( var i = 0; i < len; i++ )
   {
      tmp += str.charAt( Math.floor( Math.random() * l ) );
   }
   return tmp;
}