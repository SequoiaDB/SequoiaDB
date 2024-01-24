/************************************
*@Description: 
*@author:      liuxiaoxuan
*@createdate:  2017.10.11
*@testlinkCase:seqDB-12811
**************************************/
main( test );

function test ()
{

   //WORKDIR: /tmp/jstest
   if( !File.exist( WORKDIR ) )
   {
      File.mkdir( WORKDIR, 0777 );
   }

   //read and check empty file
   var emptyFileName = WORKDIR + "/emptyFile_12811";
   readAndCheckEmptyFile( emptyFileName );

   //check text file
   var NonEmptyFileName = WORKDIR + "/NonEmptyFile_12811";

   //write content to text file
   var content1 = 'abcdabcdabcdabcdabcdabcdabcdabcd';
   writeContentToFile( NonEmptyFileName, content1 );

   //read and check not empty file
   var expectContent1 = ['abcdabcdabcdabcdabcdabcdabcdabcd'];
   readAndCheckNotEmptyFile( NonEmptyFileName, expectContent1 );

   //write content to text file(include '\t','\n'')
   var content2 = 'abcd\tabcd\nabcd\tabcd\tabcd\nabcd';
   writeContentToFile( NonEmptyFileName, content2 );

   //read and check not empty file
   var expectContent2 = ['abcd	abcd\n', 'abcd	abcd	abcd\n', 'abcd'];
   var fileMode = SDB_FILE_READONLY;
   readAndCheckNotEmptyFile( NonEmptyFileName, expectContent2, fileMode );

}

function readAndCheckEmptyFile ( emptyFileName )
{
   assert.tryThrow( SDB_EOF, function()
   {
      if( File.exist( emptyFileName ) )
      {
         File.remove( emptyFileName );
      }
      var emptyFile = new File( emptyFileName );
      var content = emptyFile.readLine();
   } );
   File.remove( emptyFileName );
}

function writeContentToFile ( writeFileName, content )
{
   if( File.exist( writeFileName ) )
   {
      File.remove( writeFileName );
   }
   var writeFile = new File( writeFileName );
   writeFile.write( content );
}

function readAndCheckNotEmptyFile ( fileName, expectContent, fileMode )
{
   if( typeof ( fileMode ) == "undefined" ) { fileMode = SDB_FILE_READWRITE; }

   var readFile = new File( fileName, 0777, fileMode );
   var readLength = 0;
   var fileSize = File.getSize( fileName );
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

   File.remove( fileName );
}