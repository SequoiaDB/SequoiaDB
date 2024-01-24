/******************************************************************************
@Description : seqDB-20096:rest支持response标准json格式的应答
@Modify list : 2019-10-25  luweikang 
******************************************************************************/


main( test );

function test ()
{
   var clName = "cl20096";
   commDropCL( db, COMMCSNAME, clName );

   var cmd = new Cmd();
   var content = "cmd=create collection&name=" + COMMCSNAME + "." + clName;
   var rc = sendRequest( cmd, content );
   var errno = rc[0]["errno"];
   if( errno != 0 )
   {
      throw new Error( "check errno, exp: 0, act: " + errno + ", description: " + rc[0]["description"] );
   }

   var record = "{'a': 1, " +
      "'b': 'string', " +
      "'c': 3000000000, " +
      "'d': 123.456, " +
      "'e': {'\\$decimal': '123.456'}, " +
      "'f': true, " +
      "'g': {'\\$timestamp': '2012-01-01-13.14.26.124233'}, " +
      "'h': {'\\$binary': 'aGVsbG8gd29ybGQ=', '\\$type': '1'}, " +
      "'i': {'\\$regex': '^张', '\\$options': 'i'}, " +
      "'j': {'name': 'Tom'}, " +
      "'k': ['abc', 0, 'def'], " +
      "'l': null, " +
      "'m': {'\\$minKey': 1}}";
   for( var i = 0; i < 10; i++ )
   {
      var content = "cmd=insert&name=" + COMMCSNAME + "." + clName + "&insertor=" + record;
      var rc = sendRequest( cmd, content );
      var errno = rc[0]["errno"];
      if( errno != 0 )
      {
         throw new Error( "check errno, exp: 0, act: " + errno + ", description: " + rc[0]["description"] );
      }
   }

   var content = "cmd=query&name=" + COMMCSNAME + "." + clName + "&selector={'_id': {'\\$include': 0}}";
   var rc = sendRequest( cmd, content );
   var errno = rc[0]["errno"];
   if( errno != 0 )
   {
      throw new Error( "check errno, exp: 0, act: " + errno + ", description: " + rc[0]["description"] );
   }

   for( var i = 1; i <= 10; i++ )
   {
      var actRecord = JSON.stringify( rc[i] );
      var expRecord = JSON.stringify( JSON.parse( record ) );
      if( actRecord != expRecord )
      {
         throw new Error( "check errno, \nexp: " + expRecord + ", \nact: " + actRecord );
      }
   }

   commDropCL( db, COMMCSNAME, clName );
}

function sendRequest ( cmd, content )
{
   var port = parseInt( COORDSVCNAME, 10 ) + 4;
   var url = "http://" + COORDHOSTNAME + ":" + port;
   var curl = "curl \"" + url + "\" -d \"" + content + "\" -H \"Accept: application/json\" 2>/dev/null";
   var rc = cmd.run( curl );
   var rcObj = JSON.parse( rc );
   if( rcObj[0]["errno"] != 0 )
   {
   }
   return rcObj;
}
