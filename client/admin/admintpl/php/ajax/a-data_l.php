<?php
$isfirst = true ;
//$cscl_list = '{"name":"数据库","child":{"name":"集合空间","child":[' ;
//$db -> install ( "{ install : false }" ) ;

$array_1 = array() ;
$array_2 = array() ;
$array_3 = array() ;

$cursor = $db -> getList ( SDB_LIST_COLLECTIONSPACES ) ;
if ( !empty ( $cursor ) )
{
	while ( $arr = $cursor -> getNext() )
	{
		array_push( $array_1, $arr ) ;
	}
}

$cursor = $db -> getSnapshot ( SDB_LIST_COLLECTIONSPACES ) ;
if ( !empty ( $cursor ) )
{
	while ( $arr = $cursor -> getNext() )
	{
        if( array_key_exists( 'Collection', $arr ) == true )
        {
           foreach( $arr['Collection'] as $index => $clInfo )
           {
              $clName = explode( '.', $clInfo['Name'] ) ;
              if( count( $clName ) > 1 )
              {
                 $arr['Collection'][$index]['Name'] = $clName[1] ;
              }
           }
        }
		array_push( $array_2, $arr ) ;
	}
}

$cursor = $db -> getSnapshot ( 8 ) ;
if ( !empty ( $cursor ) )
{
	while ( $arr = $cursor -> getNext() )
	{
        if( array_key_exists( 'IsMainCL', $arr ) && $arr['IsMainCL'] == true )
        {
            $tmpCSCL = explode( '.', $arr['Name'] ) ;
            array_push( $array_3, $tmpCSCL  ) ;
        }
	}
}

$cscl_list = arrayMerges( $array_1, $array_2 ) ;

foreach( $cscl_list as $key => $csInfo )
{
    if( array_key_exists( 'Collection', $cscl_list[ $key ] ) == false )
    {
        $cscl_list[ $key ]['Collection'] = [] ;
    }
    foreach( $array_3 as $masterInfo )
    {
        if( $csInfo['Name'] == $masterInfo[0] )
        {
            array_push( $cscl_list[ $key ]['Collection'], array( 'Name' => $masterInfo[1] ) ) ;
        }
    }
}

$cscl_list = json_encode( array( 'name' => '数据库', 'child' => array( 'name' => '集合空间', 'child' => $cscl_list ) ), true ) ;

$smarty -> assign( "cscl_list", $cscl_list ) ;

function arrayMerges( $a, $b )
{
   foreach( $a as $key => $value )
   {
      foreach( $b as $key2 => $value2 )
      {
         if( $value2['Name'] == $value['Name'] )
         {
            $a[$key] = array_merge( $a[$key], $b[$key2] ) ;
         }
      }
   }
   return $a ;
}

?>
