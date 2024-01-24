/******************************************************************************
 * @Description   : seqDB-12136:插入记录字段名包含特殊字符
 * @Author        : Zhang Yanan
 * @CreateTime    : 2017.07.11
 * @LastEditTime  : 2021.07.19
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_12136";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   var specialStr = "~ ` ! @ # $ % ^ * ( ) - _ = + { } [ ] | \\ : ; \" < > ? / . $";
   var specialChar = [];
   specialChar = specialStr.split( " " );
   var docs = [];
   for( var i = 0; i < specialChar.length; ++i )
   {
      var insertStr = "{'" + specialChar[i] + "key" + specialChar[i] + "field':'SpecialCharacterValue'}";
      var evalInsertStr = eval( "(" + insertStr + ")" );
      varCL.insert( evalInsertStr );
      docs.push( evalInsertStr );
   }

   var cursor = varCL.find();
   commCompareResults( cursor, docs );
}