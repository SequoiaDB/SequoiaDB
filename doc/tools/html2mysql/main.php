<?php

include_once "parseConf.php" ;
include_once "mysql.php" ;
include_once "function.php" ;

$root = dirname( __FILE__ ).( getOSInfo() == 'linux' ? "/../.." : "\\..\\.." ) ;

$path = getOSInfo() == 'linux' ? "$root/config/toc.json" : "$root\\config\\toc.json" ;

$editionPath = getOSInfo() == 'linux' ? "$root/config/version.json" : "$root\\config\\version.json" ;

$edition = getVersion( $editionPath ) ;
if( $edition == FALSE )
{
   exit( 1 ) ;
}

$major = $edition['major'] ;
$minor = $edition['minor'] ;

$edition = $major * 100 + $minor ;
$version = "v$major.$minor" ;

$config = getConfig( $path ) ;
if( $config == FALSE )
{
   exit( 1 ) ;
}

if( addNewEdition( $edition, $version ) == FALSE )
{
   exit( 1 ) ;
}

if( addNewDir( $edition ) == FALSE )
{
   exit( 1 ) ;
}

if( insertDir( $edition, $config, $root ) == FALSE )
{
   exit( 1 ) ;
}

if( addNewDoc( $edition ) == FALSE )
{
   exit( 1 ) ;
}

if( insertDoc( $edition, $config, $root ) == FALSE )
{
   exit( 1 ) ;
}

if( iterDir( getOSInfo() == 'linux' ? "$root/src/images" : "$root\\src\\images", '', $edition, 'images' ) == FALSE )
{
   exit( 1 ) ;
}

if( iterDir( getOSInfo() == 'linux' ? "$root/build/output/api" : "$root\\build\\output\api", '', $edition, 'document' ) == FALSE )
{
   exit( 1 ) ;
}

echo "success\n" ;
exit( 0 ) ;