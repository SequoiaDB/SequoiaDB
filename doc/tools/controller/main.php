<?php

include_once "parseConf.php" ;
include_once "function.php" ;

//doxygen生成的文件和toc.json的映射关系
$apiList = array(
   "c"         => 1432190718,
   "bson"      => 1432190718,
   "cpp"       => 1432190724,
   "bsoncpp"   => 1432190724,
   "cs"        => 1432190730,
   "java"      => 1432190736,
   "php"       => 1432190742,
   "python"    => 1432190748
) ;

$root = dirname( __FILE__ ).( getOSInfo() == 'linux' ? "/../.." : "\\..\\.." ) ;
$toolPath = dirname( __FILE__ ).( getOSInfo() == 'linux' ? "/.." : "\\.." ) ;

if( FALSE == buildMdConverter( $toolPath, getcwd() ) )
{
   printLog( "Failed to build mdConverter" ) ;
   exit( 1 ) ;
}

printLog( "Build mdConverter success!" ) ;

$path = getOSInfo() == 'linux' ? "$root/config/toc.json" : "$root\\config\\toc.json" ;

$editionPath = getOSInfo() == 'linux' ? "$root/config/version.json" : "$root\\config\\version.json" ;

$edition = getVersion( $editionPath ) ;
if( $edition == FALSE )
{
   printLog( "Failed to parse config/version.json" ) ;
   exit( 1 ) ;
}

$major = $edition['major'] ;
$minor = $edition['minor'] ;

//输出的文件名
$outputFileName = "SequoiaDB_usermanuals_v$major.$minor" ;

$config = getConfig( $path ) ;
if( $config == FALSE )
{
   printLog( "Failed to parse config/toc.json" ) ;
   exit( 1 ) ;
}

try
{
   checkConfig( $config, null ) ;
}
catch( Exception $e )
{
   printLog( "Invalid toc.json config: ".$e->getMessage() ) ;
   exit( 1 ) ;
}

$param = getopt( 'h:m:' ) ;

if( array_key_exists( 'h', $param ) == false )
{
   $param['h'] = "0" ;
}
if( array_key_exists( 'm', $param ) == false )
{
   $param['m'] = "doc" ;
}

if( $param['h'] == "1" )
{
   echo "====== Help! ======\n" ;
   echo "-h  help.\n" ;
   echo "-m  mode: [doc] [pdf] [word] [website] [chm]\n" ;
   exit( 0 ) ;
}

//=== 初始化 ===
printLog( "Init...", "Event" ) ;

$os = getOSInfo() ;
$mdConvert = $os == 'windows' ? 'mdConverter.exe' : 'linux_mdConverter' ;
$html2mysql = $os == 'windows' ? 'exec.bat' : 'exec.sh' ;
$wkhtmltopdf = $os == 'windows' ? 'wkhtmltopdf.exe' : 'wkhtmltopdf' ;

//2.清理旧文件
printLog( "Clear file...", "Event" ) ;
if( file_exists( $os == 'windows' ? "$root\build\mid" : "$root/build/mid" ) && removeDir( $os == 'windows' ? "$root\build\mid" : "$root/build/mid" ) == false )
{
   printLog( "Failed to rm dir: $root/build/mid" ) ;
   exit( 1 ) ;
}

if( file_exists( $os == 'windows' ? "$root\build\output" : "$root/build/output" ) && removeDir( $os == 'windows' ? "$root\build\output" : "$root/build/output" ) == false )
{
   printLog( "Failed to rm dir: $root/build/output" ) ;
   exit( 1 ) ;
}

//2.创建目录
mkdir( "$root/build/mid", 0777, true ) ;
if( file_exists( "$root/build/output/api" ) == false )
{
   mkdir( "$root/build/output/api", 0777, true ) ;
}
chmod( "$root/tools/$mdConvert", 0777 ) ;

//=== 预处理 ===
if( $param['m'] == "doc" || $param['m'] == "chm" || $param['m'] == "offline" || $param['m'] == "website" || $param['m'] == "doxygen" )
{
   printLog( "Generate doxygen file", "Event" ) ;
   $doxygen_files = iterDoxygenConfig( "$root/config/doxygen" ) ;

   //1. 生成doxygen
   foreach( $doxygen_files as $index => $file )
   {
      if ( $file == "config/doxygen/doxygen-java.conf" )
      {
          continue ;
      }
      printLog( "Generate doxygen file: $root/$file\n", "Event" ) ;
      if( execCmd( "doxygen $root/$file" ) != 0 )
      {
         printLog( "Failed to exec doxygen file: $file" ) ;
         exit( 1 ) ;
      }
   }

   printLog( "Generate java driver apidocs\n", "Event" ) ;

   if( execCmd( "mvn -f $root/../driver/java/pom.xml versions:set -DnewVersion=$major.$minor" ) != 0 )
   {
      printLog( "Failed to exec \"mvn -f $root/../driver/java/pom.xml versions:set -DnewVersion=$major.$minor\"" ) ;
      exit( 1 ) ;
   }

   if( execCmd( "mvn -f $root/../driver/java/pom.xml clean javadoc:javadoc" ) != 0 )
   {
      printLog( "Failed to exec \"mvn -f $root/../driver/java/pom.xml clean javadoc:javadoc\"" ) ;
      exit( 1 ) ;
   }

   if( execCmd( "mvn -f $root/../driver/java/pom.xml versions:revert" ) != 0 )
   {
      printLog( "Failed to exec \"mvn -f $root/../driver/java/pom.xml versions:revert\"" ) ;
      exit( 1 ) ;
   }

   copyDir( "$root/../driver/java/target/site/apidocs", "$root/build/output/api/java/html") ;

   if( $param['m'] == "offline" )
   {
      if( formatApiDoc( "$root/build/output/api" ) === false )
      {
         printLog( 'Failed to format api document: build/mid' ) ;
         exit( 1 ) ;
      }
   }
   
   printLog( "Finish build doxygen document, path: doc/build/output/api", "Event" ) ;
   
   printLog( "replace symbol..., path: doc/build/output/api", "Event" ) ;
   if( execCmd( "python $root/replaceUnit.py", true ) != 0 )
   {
      printLog( "Failed to exec \"python $root/replaceUnit.py\"" ) ;
      exit( 1 ) ;
   }

}

//=== 转换 + 生成 ===
//1. pdf
if( $param['m'] == "doc" || $param['m'] == "pdf" )
{
   printLog( "Generate pdf", "Event" ) ;
   if( clearDir( "$root/build/mid" )  == false )
   {
      printLog( 'Failed to clear dir files: build/mid' ) ;
      exit( 1 ) ;
   }

   $pdf = "$root/tools/$mdConvert -c ./config/toc.json -v $major -e $minor -d single -t false -s $root/tools/pdfConvertor/src/pdf.css" ;
   if( execCmd( $pdf ) != 0 )
   {
      printLog( 'Failed to convert pdf middle file' ) ;
      exit( 1 ) ;
   }
   $platform = $os == 'windows' ? 'win32' : 'linux64' ;
   chmod( "$root/tools/pdfConvertor/tools/$platform/wkhtmltox/bin/$wkhtmltopdf", 0777 ) ;

   //修改配置
   $headerContents = file_get_contents( "$root/tools/pdfConvertor/src/header.html" ) ;
   $headerContents = str_replace( '{{version}}', "$major.$minor", $headerContents ) ;
   file_put_contents( "$root/tools/pdfConvertor/src/header_tmp.html", $headerContents ) ;

   $coverContents = file_get_contents( "$root/tools/pdfConvertor/src/cover.html" ) ;
   $coverContents = str_replace( '{{version}}', "$major.$minor", $coverContents ) ;
   file_put_contents( "$root/tools/pdfConvertor/src/cover_tmp.html", $coverContents ) ;

   $pdf = "$root/tools/pdfConvertor/tools/$platform/wkhtmltox/bin/$wkhtmltopdf --page-size A4 --dpi 300 --enable-smart-shrinking --load-error-handling ignore --encoding utf-8 --user-style-sheet $root/tools/pdfConvertor/src/pdf_global.css --footer-html $root/tools/pdfConvertor/src/footer.html --header-html $root/tools/pdfConvertor/src/header_tmp.html --page-offset -1 cover $root/tools/pdfConvertor/src/cover_tmp.html toc --xsl-style-sheet $root/tools/pdfConvertor/src/toc.xsl $root/build/mid/build.html $root/build/output/$outputFileName.pdf" ;
   if( execCmd( $pdf ) != 0 )
   {
      //忽略pdf在linux的错误
      if( $os == 'windows' )
      {
         printLog( 'Failed to convert pdf file' ) ;
         exit( 1 ) ;
      }
   }
   printLog( "Finish build pdf document, path: doc/build/output/$outputFileName.pdf", "Event" ) ;
}

//2. word
if(  $param['m'] == "word" && $os == 'windows' )
{
   printLog( "Generate word", "Event" ) ;
   if( clearDir( "$root/build/mid" )  == false )
   {
      printLog( 'Failed to clear dir files: build/mid' ) ;
      exit( 1 ) ;
   }

   $word = "$root/tools/$mdConvert -c ./config/toc.json -v $major -e $minor -d word -t false" ;
   if( execCmd( $word ) != 0 )
   {
      printLog( 'Failed to convert word middle file: '.$word ) ;
      exit( 1 ) ;
   }

   $word = "$root/../java/jdk_win32/bin/java.exe -Dlog4j.configuration=file:$root/tools/wordConvertor/log4j.properties -jar $root/tools/wordConvertor/wordConvertor.jar -i $root/build/mid/build.html -o $root/build/output/$outputFileName.doc -t" ;
   if( execCmd( $word, true ) != 0 )
   {
      printLog( 'Failed to convert word file' ) ;
      exit( 1 ) ;
   }
   printLog( "Finish build word document, path: doc/build/output/$outputFileName.doc", "Event" ) ;
}

//3. chm
if( $param['m'] == "chm" && $os == 'windows' )
{
   printLog( "Generate chm", "Event" ) ;
   if( clearDir( "$root/build/mid" )  == false )
   {
      printLog( 'Failed to clear dir files: build/mid' ) ;
      exit( 1 ) ;
   }

   $chm = "$root/tools/$mdConvert -c ./config/toc.json -v $major -e $minor -t false -d chm -l false -s $root/tools/pdfConvertor/src/pdf.css" ;
   if( execCmd( $chm ) != 0 )
   {
      printLog( "Failed to convert chm middle file: $chm" ) ;
      exit( 1 ) ;
   }

   foreach( $apiList as $doxyDoc => $id )
   {
      $source = "$root/build/output/api/$doxyDoc" ;
      //$dest   = getCnPath( $id, $config, "$root/build/mid" ) ;
      $dest   = getDirPath( $id, $config, "$root/build/mid" ) ;
      if( $dest == false )
      {
         printLog( "Failed to find id: $id in toc.json" ) ;
         exit( 1 ) ;
      }
      $dest = "$dest/api/$doxyDoc" ;
      if( $os == 'windows' )
      {
         $dest = iconv( 'utf-8', 'gb2312//IGNORE', $dest ) ;
      }
      copyDir( $source, $dest ) ;
      //echo $source."\n" ;
      //echo iconv( 'utf-8', 'gb2312//IGNORE', $dest )."\n" ;
   }

   $chm = "$root/../java/jdk_win32/bin/java.exe -jar $root/tools/chmProjectCreator/chmProjectCreator.jar -i $root/build/mid -o $root/build/output -t \"$outputFileName\" -c $root/config/toc.json" ;
   if( execCmd( $chm ) != 0 )
   {
      printLog( 'Failed to convert chm config file' ) ;
      exit( 1 ) ;
   }

   printLog( "Finish build chm config, path: doc/build/output/$outputFileName.wcp", "Event" ) ;

   file_put_contents( "$root/tools/anjian/config.ini", "[config]\r\npath=$root\\build\\output\\$outputFileName.wcp\r\nfile=$outputFileName.wcp" ) ;

   $chm = "$root/tools/anjian/autoBuildCHM.exe" ;
   execCmd( $chm ) ;

   $log = file_get_contents( "$root/tools/anjian/anjian.log" ) ;
   echo "\n".$log."\n" ;
   if( strpos( $log, "Error" ) !== false )
   {
      printLog( 'Failed to build chm.' ) ;
      exit( 1 ) ;
   }

   printLog( "Finish build chm document, path: doc/build/output/$outputFileName.chm", "Event" ) ;
}

//4. offline html document
if( $param['m'] == "offline" && $os == 'windows' )
{
   printLog( "Generate offline html document", "Event" ) ;
   if( clearDir( "$root/build/mid" )  == false )
   {
      printLog( 'Failed to clear dir files: build/mid' ) ;
      exit( 1 ) ;
   }

   $chm = "$root/tools/$mdConvert -c ./config/toc.json -v $major -e $minor -t false -d offline -l false -s $root/tools/pdfConvertor/src/pdf.css" ;
   if( execCmd( $chm ) != 0 )
   {
      printLog( "Failed to convert offline middle file: $chm" ) ;
      exit( 1 ) ;
   }

   foreach( $apiList as $doxyDoc => $id )
   {
      $source = "$root/build/output/api/$doxyDoc" ;
      //$dest   = getCnPath( $id, $config, "$root/build/mid" ) ;
      $dest   = getDirPath( $id, $config, "$root/build/mid" ) ;
      if( $dest == false )
      {
         printLog( "Failed to find id: $id in toc.json" ) ;
         exit( 1 ) ;
      }
      $dest = "$dest/api/$doxyDoc" ;
      if( $os == 'windows' )
      {
         $dest = iconv( 'utf-8', 'gb2312//IGNORE', $dest ) ;
      }
      copyDir( $source, $dest ) ;
      //echo $source."\n" ;
      //echo iconv( 'utf-8', 'gb2312//IGNORE', $dest )."\n" ;
   }

   $chm = "$root/../java/jdk_win32/bin/java.exe -jar $root/tools/chmProjectCreator/chmProjectCreator.jar -i $root/build/mid -o $root/build/output -t \"$outputFileName\" -c $root/config/toc.json" ;
   if( execCmd( $chm ) != 0 )
   {
      printLog( 'Failed to convert offline config file' ) ;
      exit( 1 ) ;
   }

   printLog( "Finish build offline config, path: doc/build/output/$outputFileName.wcp", "Event" ) ;

   file_put_contents( "$root/tools/anjian/config.ini", "[config]\r\npath=$root\\build\\output\\$outputFileName.wcp\r\nfile=$outputFileName.wcp" ) ;

   $chm = "$root/tools/anjian/autoBuildHtml.exe" ;
   execCmd( $chm ) ;

   $log = file_get_contents( "$root/tools/anjian/anjian.log" ) ;
   echo "\n".$log."\n" ;
   if( strpos( $log, "Error" ) !== false )
   {
      printLog( 'Failed to build offline.' ) ;
      exit( 1 ) ;
   }

   printLog( "Finish build chm document, path: doc/build/output/$outputFileName.chm", "Event" ) ;
}

//5. 官网
if( $param['m'] == "website" )
{
   printLog( "Generate website", "Event" ) ;
   if( clearDir( "$root/build/mid" ) == false )
   {
      printLog( 'Failed to clear dir files: build/mid' ) ;
      exit( 1 ) ;
   }

   $website = "$root/tools/$mdConvert -c ./config/toc.json -v $major -e $minor -d website -l false -u false" ;
   if( execCmd( $website ) != 0 )
   {
      printLog( 'Failed to convert website middle file: '.$website ) ;
      exit( 1 ) ;
   }

   $website = "$root/tools/html2mysql/$html2mysql" ;
   chmod( $website, 0777 ) ;
   if( execCmd( $website ) != 0 )
   {
      printLog( 'Failed to send website middle file: '.$website ) ;
      exit( 1 ) ;
   }
   printLog( "Finish build web site", "Event" ) ;
}

echo "Success!\n" ;
exit( 0 ) ;
