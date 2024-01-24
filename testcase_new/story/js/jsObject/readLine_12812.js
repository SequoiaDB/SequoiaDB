/************************************
*@Description: 
*@author:      liuxiaoxuan
*@createdate:  2017.10.11
*@testlinkCase:seqDB-12812
**************************************/
main( test );

function test ()
{

   var remotehost = toolGetRemotehost();
   var remotehostName = remotehost["hostname"];
   var remote = new Remote( remotehostName, CMSVCNAME );
   var remoteFile = remote.getFile();
   //WORKDIR: /tmp/jstest
   if( !remoteFile.exist( WORKDIR ) )
   {
      remoteFile.mkdir( WORKDIR, 0777 );
   }

   var emptyFileName = WORKDIR + "/emptyFile_12811";
   var NonEmptyFileName = WORKDIR + "/NonEmptyFile_12811";

   //read empty file
   readAndCheckEmptyFile( remote, emptyFileName );

   //write content to file
   var content1 = 'abcdabcdabcdabcdabcdabcdabcdabcd';
   writeContentToFile( remote, NonEmptyFileName, content1 );

   //read and check not empty file
   var expectContent1 = ['abcdabcdabcdabcdabcdabcdabcdabcd'];
   readAndCheckNotEmptyFile( remote, NonEmptyFileName, expectContent1 );

   //write content to file(include '\t','\n')
   var content2 = 'abcd\tabcd\nabcd\tabcd\tabcd\nabcdabcdabcd';
   writeContentToFile( remote, NonEmptyFileName, content2 );

   //read and check not empty file
   var expectContent2 = ['abcd	abcd\n', 'abcd	abcd	abcd\n', 'abcdabcdabcd'];
   var fileMode = SDB_FILE_READONLY;
   readAndCheckNotEmptyFile( remote, NonEmptyFileName, expectContent2, fileMode );
}


function readAndCheckEmptyFile ( remote, emptyFileName )
{
   var remoteFile = null;
   assert.tryThrow( SDB_EOF, function()
   {
      remoteFile = remote.getFile();
      if( remoteFile.exist( emptyFileName ) )
      {
         remoteFile.remove( emptyFileName );
      }
      var emptyFile = remote.getFile( emptyFileName );
      var content = emptyFile.readLine();
   } );
   remoteFile.remove( emptyFileName );
}

function writeContentToFile ( remote, writeFileName, content )
{
   var remoteFile = remote.getFile();
   if( remoteFile.exist( writeFileName ) )
   {
      remoteFile.remove( writeFileName );
   }
   var writeFile = remote.getFile( writeFileName );
   writeFile.write( content );
}

function readAndCheckNotEmptyFile ( remote, fileName, expectContent, fileMode )
{
   if( typeof ( fileMode ) == "undefined" ) { fileMode = SDB_FILE_READWRITE; }
   var remoteFile = remote.getFile();
   var readFile = remote.getFile( fileName, 0777, fileMode );
   var readLength = 0;
   var fileSize = remoteFile.getSize( fileName );
   var actContent = [];
   while( true )
   {
      var content = readFile.readLine();
      actContent.push( content );
      readLength = readLength + content.length;
      if( readLength >= fileSize )
      {
         break;
      }
   }

   //check the content length 
   assert.equal( actContent.length, expectContent.length );
   //check content each row 
   for( var i in actContent )
   {
      var actRow = actContent[i];
      var expRow = expectContent[i];
      assert.equal( actRow, expRow );
   }

   remoteFile.remove( fileName );
}