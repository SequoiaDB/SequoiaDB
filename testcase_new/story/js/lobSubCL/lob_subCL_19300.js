/************************************
*@Description:  seqDB-19300 主表上listLobs指定cond匹配条件不正确
*@author:      wuyan
*@createDate:  2019.9.5
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var mainCLName = "mainCL19300";
   var subCLName = "subcl19300";
   var subCLNum = 2;
   var filePath = WORKDIR + "/subCLLob19300/";
   deleteTmpFile( filePath );
   var scope = 5;
   var beginBound = 20190801;
   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the beginning" );
   var mainCL = createMainCLAndAttachCL( db, COMMCSNAME, mainCLName, subCLName, "YYYYMMDD", subCLNum, beginBound, scope );

   //put lob
   var lobSizes = [1024, 10, 36, 1024 * 10, 1024 * 15, 1024 * 20, 3, 1, 2, 0];
   for( var i = 0; i < lobSizes.length; ++i )
   {
      var fileName = "lob_" + lobSizes[i];
      makeTmpFile( filePath, fileName, lobSizes[i] );
      var beginDate = beginBound + i;
      insertLob( mainCL, filePath + fileName, "YYYYMMDD", scope, 1, 1, beginDate );
   }

   //listlobs with error cond
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      mainCL.listLobs( SdbQueryOption().cond( {} ).cond( { "Size": { "$include": 0 } } ) );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      mainCL.listLobs( SdbQueryOption().cond( { "Size": { $kk: 0 } } ) );
   } );

   //listLobs with correct condition
   var expSize = 1024;
   listLobsWithCondAndCheckResult( mainCL, expSize );

   //listLobs with correct selCondition
   listLobsWithSelAndCheckResult( mainCL, lobSizes );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the ending" );
   deleteTmpFile( filePath );
}

function listLobsWithCondAndCheckResult ( mainCL, expSize )
{
   var listResult = mainCL.listLobs( SdbQueryOption().cond( { Size: expSize } ) );
   var actListResult = [];
   while( listResult.next() )
   {
      var listObj = listResult.current().toObj();
      var sizeValue = listObj["Size"];
      if( Number( sizeValue ) !== Number( expSize ) )
      {
         throw new Error( "expect Size: " + expSize + "actual Size: " + sizeValue );
      }
      actListResult.push( listObj );
   }

   var expNum = 1;
   if( Number( actListResult.length ) !== Number( expNum ) )
   {
      throw new Error( "expect listNum: " + expNum + "actual listNum: " + actListResult.length
         + "/nactListResult:" + JSON.stringify( actListResult ) );
   }
}

function listLobsWithSelAndCheckResult ( mainCL, lobSizes )
{
   var listResult = mainCL.listLobs( SdbQueryOption().sel( { Size: { "$include": 1 } } ).sort( { "Size": 1 } ) );
   var actListResult = [];
   while( listResult.next() )
   {
      var listObj = listResult.current().toObj();
      actListResult.push( listObj );
   }

   //lobSizes sort Ascending
   lobSizes.sort( function( a, b ) { return a - b; } );
   var expListResult = [];
   for( var i = 0; i < lobSizes.length; i++ )
   {
      var value = lobSizes[i];
      expListResult.push( { "Size": value } );
   }

   if( JSON.stringify( expListResult ) !== JSON.stringify( actListResult ) )
   {
      throw new Error( "/nexpectResult: " + JSON.stringify( expListResult ) + "/nactualResult: " + JSON.stringify( actListResult ) );
   }
}
