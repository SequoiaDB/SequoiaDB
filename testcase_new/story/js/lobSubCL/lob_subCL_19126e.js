/************************************
*@Description:  seqDB-19126 主表上执行hint查询，查询数据跨多个子表
*@author:      wuyan
*@createDate:  2019.8.26
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = "cs19126";
   var mainCLName = "mainCL19126";
   var subCLName = "subcl19126";
   var subCLNum = 1;
   var lobPageSize = 4096;
   var filePath = WORKDIR + "/subCLLob19126/";
   deleteTmpFile( filePath );
   var beginBound = 20190101;
   commDropCS( db, csName, true, "drop CS in the beginning" )
   commCreateCS( db, csName, false, "", { LobPageSize: lobPageSize } );
   var mainCL = createMainCLAndAttachCL( db, csName, mainCLName, subCLName, "YYYYMMDD", subCLNum, beginBound );

   //put lob
   var lobSize = putLobs( mainCL, filePath );

   //listLobs with hint; 
   listLobsWithHintAndCheckResult( mainCL, lobSize, lobPageSize );

   commDropCS( db, csName, true, "drop CS in the ending" )
   deleteTmpFile( filePath );
}

function putLobs ( mainCL, filePath )
{
   var maxLobSize = 1024 * 1024;
   var lobSize = Math.round( Math.random() * maxLobSize );
   var fileName = "lob_" + lobSize;
   var expListResult = [];
   makeTmpFile( filePath, fileName, lobSize );

   var beginDate = "2019-01-01-00.00.00.000000";
   var lobOid = mainCL.createLobID( beginDate );
   mainCL.putLob( filePath + fileName, lobOid );

   return lobSize;
}

function listLobsWithHintAndCheckResult ( mainCL, lobSize, lobPageSize )
{

   var listResult = mainCL.listLobPieces();
   var expListResult = [];
   while( listResult.next() )
   {
      var listObj = listResult.current().toObj();
      expListResult.push( listObj );
   }
   expListResult.sort(
      function( a, b )
      {
         return a.Sequence - b.Sequence;
      }
   )

   var actRecs = [];
   var listResult = mainCL.listLobs( SdbQueryOption().sort( { "Sequence": 1 } ).hint( { "ListPieces": 1 } ) );
   while( listResult.next() )
   {
      var listObj = listResult.current().toObj();
      actRecs.push( listObj );
   }

   var piecesNum = 0;
   if( lobSize % lobPageSize == 0 )
   {
      piecesNum = lobSize / lobPageSize + 1;
   }
   //lobmeta size is 1k = 1024
   else if( lobSize % lobPageSize <= lobPageSize - 1024 )
   {
      piecesNum = Math.ceil( lobSize / lobPageSize );
   }
   else
   {
      piecesNum = Math.ceil( lobSize / lobPageSize ) + 1;
   }

   if( piecesNum !== actRecs.length )
   {
      throw new Error( "CheckLobPiecesNums error", "\n expPieces =" + piecesNum + "\n actPieces=" + actRecs.length
         + "\nactual value= " + JSON.stringify( actRecs ) + "\nexpect value= " + JSON.stringify( expListResult ) + "\n lobSize=" + lobSize );
   }

   if( JSON.stringify( actRecs ) !== JSON.stringify( expListResult ) )
   {
      throw new Error( "listLobsWithQueryAndCheckResult()", "\nactual value= " + JSON.stringify( actRecs ) + "\nexpect value= "
         + JSON.stringify( expListResult ) );
   }
}
