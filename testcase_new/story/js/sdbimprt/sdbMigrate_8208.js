/*******************************************************************************
*@Description : import one record that across multipile lines.
*               [SEQUOIADBMAINSTREAM-474]
*@Modify list :
*               2014-11-10  xiaojun Hu  Change
*               2019-12-02  Siqin Chen  Change
*******************************************************************************/

main( test );

function test ()
{
   var clName = COMMCLNAME + "_import8208";
   var imprtFile = tmpFileDir + "8208.json";
   var nestNum = 10;
   var recordNum = 100;

   commDropCL( db, COMMCSNAME, clName, true, true );
   cmd.run( "rm -rf " + imprtFile );

   var cl = commCreateCL( db, COMMCSNAME, clName );

   var nest = migNestObject( nestNum );
   var file = new fileInit( imprtFile );
   var nestRecords = "";
   var expRecs = "["
   for( var i = 0; i < recordNum; i++ )
   {
      nestRecords = nestRecords + nest;
      expRecs = expRecs + nest;
      if( i != recordNum - 1 )
      {
         expRecs = expRecs + ",";
      }
   }
   expRecs = expRecs + "]";
   file.write( nestRecords );

   testImportData8208_1( COMMCSNAME, clName, imprtFile, cl, recordNum );
   testImportData8208_2( COMMCSNAME, clName, imprtFile, cl, recordNum, expRecs );

   commDropCL( db, COMMCSNAME, clName );
   cmd.run( "rm -rf " + imprtFile );

}

//JSON导入时，一条嵌套对象记录跨多行时，导入的数据--linepriority true
function testImportData8208_1 ( csName, clName, imprtFile, cl, recordNum )
{
   var imprtOption = installDir + "bin/sdbimprt " + "--hostname " + COORDHOSTNAME + " --svcname " + COORDSVCNAME +
      " -c " + csName + " -l " + clName + " --file " + imprtFile +
      " --type json ";
   cmd.run( imprtOption );
   var actNum = cl.find().count();
   assert.equal( recordNum, actNum );
}

//JSON导入时，一条嵌套对象记录跨多行时，导入的数据--linepriority false
function testImportData8208_2 ( csName, clName, imprtFile, cl, recordNum, expRecs )
{
   cl.truncate();
   var imprtOption = installDir + "bin/sdbimprt " + "--hostname " + COORDHOSTNAME + " --svcname " + COORDSVCNAME +
      " -c " + csName + " -l " + clName + " --file " + imprtFile +
      " --type json --linepriority false";
   cmd.run( imprtOption );
   var actNum = cl.count();
   assert.equal( recordNum, actNum );
   var cursor = cl.find( {}, { "nestField_0": 1 } );
   commCompareResults( cursor, JSON.parse( expRecs ) );
}

/*******************************************************************************
* @Description : construct nest object and will line feed[newline]. such as :
*                {
*                  a:
*                    {
*                      b:"nestObject"
*                    }
*                }
*******************************************************************************/
function migNestObject ( nestLayer, totalLayer )
{
   if( undefined == nestLayer )
   {
      nestLayer = 31;
      totalLayer = 31;
   }
   if( undefined == totalLayer )
      totalLayer = nestLayer;
   var nestRecord = "";
   var nestFieldVal = "Oh, My God !";
   var space = "";
   var spaceVar = "  ";
   if( 0 == nestLayer )
   {
      nestRecord = '"' + nestFieldVal + '"';
      return nestRecord;
   }
   else
   {
      var nestFunc = migNestObject( nestLayer - 1, totalLayer );
      var cnt = totalLayer - nestLayer;
      var nestFieldName = "nestField_" + cnt;
      spaceBrace = nestSpace( cnt );
      spaceField = nestSpace( cnt + 1 );
      nestRecord = '{\n' + spaceField + '"' +
         nestFieldName + '": ' + nestFunc + '\n' + spaceBrace + '}';
      return nestRecord;
   }
}

/*******************************************************************************
* @Description : construct nest two space, such as "  "
*******************************************************************************/
function nestSpace ( number )
{
   var space = "  ";
   var nospace = "";
   if( 0 == number )
      return nospace;
   else if( 1 == number )
      return space;
   else
   {
      var totalSpace = space + nestSpace( number - 1 );
      return totalSpace;
   }
}
