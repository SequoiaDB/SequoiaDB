/******************************************************************************
*@Description : get local hostname
*@author      : Liang XueWang
******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

function toolGetLocalhost ()
{
   var cmd = new Cmd();
   var localhost = cmd.run( "hostname" ).split( "\n" )[0];
   return localhost;
}

/******************************************************************************
*@Description : get hosts in cluster
*@author      : Liang XueWang
******************************************************************************/
function toolGetHosts ()
{
   var hosts = [];
   var k = 0;

   var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   if( commIsStandalone( db ) )
   {
      return hosts;
   }

   var tmpInfo = db.listReplicaGroups().toArray();
   for( var i = 0; i < tmpInfo.length; i++ )
   {
      var tmpObj = JSON.parse( tmpInfo[i] );
      var tmpArr = tmpObj.Group;
      for( var j = 0; j < tmpArr.length; j++ )
      {
         if( hosts.indexOf( tmpArr[j].HostName ) == -1 )
            hosts[k++] = tmpArr[j].HostName;
      }
   }
   return hosts;
}

/******************************************************************************
*@Description : get a remote hostname in cluster
*               if cluster has no remote host, return localhost
*@author      : Liang XueWang
******************************************************************************/
function toolGetRemotehost ()
{
   var hosts = toolGetHosts();
   var localhost = toolGetLocalhost();
   var remotehost = localhost;
   for( var i = 0; i < hosts.length; i++ )
   {
      if( hosts[i] !== localhost )
      {
         remotehost = hosts[i];
         break;
      }
   }
   return remotehost;
}

//read and write content, check length
function readWriteContentAndCheck ( readFile, writeFile, length, fileSize )
{
   if( typeof ( length ) == "undefined" ) { length = 1024; }
   if( typeof ( fileSize ) == "undefined" ) { fileSize = length; }

   var content = readFile.readContent( length );
   var readLength = content.getLength();
   if( length > fileSize )
   {
      length = fileSize;
   }
   assert.equal( readLength, parseInt( length ) );

   writeFile.writeContent( content );
   writeFileName = writeFile.getInfo().toObj().filename;
   var writeLength = parseInt( writeFile.stat( writeFileName ).toObj().size );
   writeFile.remove( writeFileName );
   assert.equal( writeLength, parseInt( length ) );
}

//read and write many times
function readWriteContentManyTimes ( readFile, writeFile, length )
{
   readFileName = readFile.getInfo().toObj().filename;
   writeFileName = writeFile.getInfo().toObj().filename;
   var fileSize = parseInt( readFile.stat( readFileName ).toObj().size );
   var times = Math.ceil( fileSize, length );

   for( var i = 0; i < times; i++ )
   {
      try
      {
         var content = readFile.readContent( length );
         writeFile.writeContent( content );
      }
      catch( e )
      {
         if( SDB_EOF == e.message )
         {
            break;
         }
         else
         {
            throw e;
         }
      }
   }

   //check
   var readMd5 = readFile.md5( readFileName );
   var writeMd5 = writeFile.md5( writeFileName );
   writeFile.remove( writeFileName );
   assert.equal( readMd5, writeMd5 );
}

function checkArgumentRead ( readFile, length, errCode )
{
   if( typeof ( errCode ) == "undefined" ) { errCode = -6; }
   assert.tryThrow( errCode, function()
   {
      readFile.readContent( length );
   } )
}

function checkArgumentWrite ( readFile, writeFile, content, length )
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      writeFile.writeContent( content );
   } )
   writeFile.remove( writeFileName );
}

function toBase64CodeTest ( readFile, actualFile, expectFile, length, cmd )
{
   if( typeof ( length ) == "undefined" ) { length = 1024; }

   var readFileName = readFile.getInfo().toObj().filename;
   var actualFileName = actualFile.getInfo().toObj().filename;
   var expectFileName = expectFile.getInfo().toObj().filename;
   var fileSize = parseInt( readFile.stat( readFileName ).toObj().size );

   var times = Math.ceil( fileSize, length );
   for( var i = 0; i < times; i++ )
   {
      try
      {
         var content = readFile.readContent( length );
         var base64 = content.toBase64Code();
         actualFile.write( base64 );
      }
      catch( e )
      {
         if( SDB_EOF == e.message ) { break; }
      }
   }

   cmd.run( "base64 " + readFileName + "> " + expectFileName + "_tmp" );
   //cmd.run( "sed -i ':a; N; $!ba; s/\\n//g' " + expectFileName ); 
   cmd.run( "cat " + expectFileName + "_tmp |tr -d '\\n' >" + expectFileName );

   //check
   var expectMd5 = expectFile.md5( expectFileName );
   var actualMd5 = actualFile.md5( actualFileName );
   expectFile.remove( expectFileName + "_tmp" );
   expectFile.remove( expectFileName );
   actualFile.remove( actualFileName );
   assert.equal( expectMd5, actualMd5 );
}

function getPermission ( file )
{
   var mode = file._getPermission( "/tmp" );
   assert.equal( mode, 511 );
}
