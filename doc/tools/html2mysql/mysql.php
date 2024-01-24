<?php

function addNewEdition( $id, $version )
{
   $sql = "insert into tp_edition(`id`,`cat_id`,`title`,`describe`,`add_time`,`is_open`) values('$id','','$version','$version','".time()."','1')" ;
   
   $db = mysqli_connect( "192.168.20.248:3306", "root", "sequoiadb", "cn_comm" ) ;
   if( mysqli_connect_errno() )
   {
      echo "Connect failed: ".mysqli_connect_error()."\n" ;
      return false ;
   }
   
   mysqli_query( $db, "set names utf8" ) ;
   
   if( $result = mysqli_query( $db, "select * from tp_edition where `id`='$id'" ) )
   {
      $editions = mysqli_fetch_all( $result, MYSQLI_NUM ) ;
      if( count( $editions ) > 0 )
      {
         mysqli_free_result( $result ) ;
         mysqli_close( $db ) ;
         return true ;
      }
      mysqli_free_result( $result ) ;
   }
   else
   {
      echo mysqli_error( $db )."\n" ;
      mysqli_close( $db ) ;
      return false ;
   }
   
   if( mysqli_query( $db, $sql ) == FALSE )
   {
      echo "Falied to insert new edition\n" ;
      echo mysqli_error( $db )."\n" ;
      mysqli_close( $db ) ;
      return false ;
   }

   mysqli_close( $db ) ;
   return true ;
}

function addNewDir( $version )
{
   $db = mysqli_connect( "192.168.20.248:3306", "root", "sequoiadb", "cn_comm" ) ;
   if( mysqli_connect_errno() )
   {
      echo "Connect failed: ".mysqli_connect_error()."\n" ;
      return false ;
   }

   mysqli_query( $db, "set names utf8" ) ;

   $sql = "CREATE TABLE IF NOT EXISTS `tp_dir$version` (`cat_id` int(11) NOT NULL AUTO_INCREMENT,`cat_name` varchar(255) NOT NULL DEFAULT '',`sort_order` tinyint(3) unsigned NOT NULL DEFAULT '50',`parent_id` int(11) unsigned NOT NULL DEFAULT '0',`is_open` int(11) NOT NULL DEFAULT '1',`has_doc` tinyint(1) unsigned NOT NULL DEFAULT '0',`is_newtoc` tinyint(1) NOT NULL DEFAULT '0'  COMMENT '是否展示新目录', `next_cat` int(11) NOT NULL,PRIMARY KEY (`cat_id`),KEY `sort_order` (`sort_order`),KEY `parent_id` (`parent_id`)) DEFAULT CHARSET=utf8 ;" ;
   if( mysqli_query( $db, $sql ) == FALSE )
   {
      echo "Falied to create dir table\n" ;
      echo mysqli_error( $db )."\n" ;
      mysqli_close( $db ) ;
      return false ;
   }
   if( mysqli_query( $db, "truncate table `tp_dir$version`" ) == FALSE )
   {
      echo "Falied to truncate table tp_dir$version\n" ;
      echo mysqli_error( $db )."\n" ;
      mysqli_close( $db ) ;
      return false ;
   }
   
   mysqli_close( $db ) ;
   return true ;
}

function _findNextDoc( $config, $path )
{
   foreach( $config['contents'] as $index => $value )
   {
      if( array_key_exists( 'contents', $value ) && array_key_exists( 'dir', $value ) )
      {
         $readmeFile = $path ;
         $nextPath   = _cat_path( $readmeFile, $value['dir'] ) ;
         $readmeFile = _cat_path( $nextPath, "Readme.html" ) ;

         echo "$readmeFile \r\n" ;

         if( file_exists( $readmeFile ) && filesize( $readmeFile ) > 0 )
         {
            return $value['id'] ;
         }
         else
         {
            $next = _findNextDoc( $value, $nextPath ) ;
            if( $next > 0 )
            {
               return $next ;
            }
         }
      }
      else if( !array_key_exists( 'contents', $value ) && array_key_exists( 'file', $value ) )
      {
         return $value['id'] ;
      }
   }

   return 0 ;
}

function _insertDirRecord( $db, $id, $dir, $order, $parentId, $path )
{
   if( array_key_exists( 'disable', $dir ) && $dir['disable'] )
   {
      return true ;
   }

   $isNewToc = 0 ;
   $next = 0 ;

   if( array_key_exists( 'top', $dir ) && $dir['top'] === true )
   {
      $isNewToc = 1 ;
   }

   $hasDoc = 0 ;
   if( array_key_exists( 'id', $dir ) &&  array_key_exists( 'dir', $dir ) && $dir['id'] > 10 )
   {
      $readmeFile = _cat_path( $path, "Readme.html" ) ;

      if( file_exists( $readmeFile ) && filesize( $readmeFile ) > 0 )
      {
         $hasDoc = 1 ;
      }
      else if ( $isNewToc )
      {
         //没有readme 并且 指定了是 top
         $next = _findNextDoc( $dir, _cat_path( $path, $dir['dir'] ) ) ;
      }
   }

   $sql = "INSERT INTO `tp_dir$id` (`cat_id`, `cat_name`, `sort_order`, `parent_id`, `is_open`, `has_doc`, `is_newtoc`, `next_cat` ) VALUES(".$dir['id'].", '".$dir['cn']."', $order, $parentId, 1, $hasDoc, $isNewToc, $next)" ;
   if( mysqli_query( $db, $sql ) == FALSE )
   {
      echo "Falied to insert tp_dir$id record\n" ;
      echo mysqli_error( $db )."\n" ;
      return false ;
   }

   return true ;
}

function _insertDirAll( $db, $id, $config, $order, $parentId, $path )
{
   if( false == _insertDirRecord( $db, $id, $config, $order, $parentId, $path ) )
   {
      echo "Failed to insert dir record, id = ".$config['id']."\n" ;
      return false ;
   }

   if( array_key_exists( 'contents', $config ) )
   {
      foreach( $config['contents'] as $index => $value )
      {
         $newPath = $path ;

         if( array_key_exists( 'id', $value ) &&  array_key_exists( 'dir', $value ) )
         {
            $newPath = _cat_path( $newPath, $value['dir'] ) ;
         }

         if( _insertDirAll( $db, $id, $value, $index + 1, $config['id'], $newPath ) == FALSE )
         {
            echo "Failed to insert dir record, id = ".$value['id']."\n" ;
            return false ;
         }
      }
   }

   return true ;
}

function insertDir( $id, $config, $rootPath )
{
   $config = array(
      "id" => 1, 
      "cn" => "文档", 
      "en" => "document", 
      "contents" => array(
         array(
            "id" => 3, 
            "cn" => "文档", 
            "en" => "document", 
            "contents" => $config['contents']
         ),
      )
   ) ;
   $db = mysqli_connect( "192.168.20.248:3306", "root", "sequoiadb", "cn_comm" ) ;
   if( mysqli_connect_errno() )
   {
      echo "Connect failed: ".mysqli_connect_error()."\n" ;
      return false ;
   }
   
   mysqli_query( $db, "set names utf8" ) ;

   $rootPath = _cat_path( $rootPath, "build" ) ;
   $rootPath = _cat_path( $rootPath, "mid" ) ;
   
   if( _insertDirAll( $db, $id, $config, 1, 0, $rootPath ) == FALSE )
   {
      echo "Failed to insert dir all\n" ;
      mysqli_close( $db ) ;
      return false ;
   }
   
   mysqli_close( $db ) ;
   return true ;
}

function addNewDoc( $version )
{
   $db = mysqli_connect( "192.168.20.248:3306", "root", "sequoiadb", "cn_comm" ) ;
   if( mysqli_connect_errno() )
   {
      echo "Connect failed: ".mysqli_connect_error()."\n" ;
      return false ;
   }

   mysqli_query( $db, "set names utf8" ) ;

   $sql = "CREATE TABLE IF NOT EXISTS `tp_doc$version` ( `article_id` int(11) unsigned NOT NULL AUTO_INCREMENT, `cat_id` int(11) NOT NULL DEFAULT '0', `title` varchar(150) DEFAULT NULL, `content` longtext NOT NULL, `is_open` tinyint(1) unsigned NOT NULL DEFAULT '1', `add_time` int(10) unsigned NOT NULL DEFAULT '0', `sort_order` int(8) NOT NULL DEFAULT '50', `edition` int(11) NOT NULL DEFAULT '0', PRIMARY KEY (`article_id`), KEY `cat_id` (`cat_id`)) DEFAULT CHARSET=utf8 ;" ;
   if( mysqli_query( $db, $sql ) == FALSE )
   {
      echo "Falied to create doc table\n" ;
      echo mysqli_error( $db )."\n" ;
      mysqli_close( $db ) ;
      return false ;
   }
   if( mysqli_query( $db, "truncate table `tp_doc$version`" ) == FALSE )
   {
      echo "Falied to truncate table tp_doc$version\n" ;
      echo mysqli_error( $db )."\n" ;
      mysqli_close( $db ) ;
      return false ;
   }
   
   mysqli_close( $db ) ;
   return true ;
}

function _insertDocAll( $db, $version, $config, $path )
{
   if( array_key_exists( 'contents', $config ) )
   {
      $newPath = $path ;
      if( array_key_exists( 'dir', $config ) )
      {
         $newPath = _cat_path( $path, $config['dir'] ) ;
      }

      //被禁用
      if( array_key_exists( 'disable', $config ) && $config['disable'] === true )
      {
         return true ;
      }

      {
         $readmeFile = _cat_path( $newPath, "Readme.html" ) ;

         if( file_exists( $readmeFile ) && filesize( $readmeFile ) > 0 )
         {
            $readmeConfig = array(
               "id"     =>  $config['id'],
               "file"   =>  "Readme",
               "cn"     =>  $config['cn'],
               "en"     =>  $config['en'],
            ) ;

            if( _insertDocAll( $db, $version, $readmeConfig, $newPath ) == FALSE )
            {
               echo "Failed to insert dir record, id = ".$value['id']."\n" ;
               return false ;
            }
         }
      }

      foreach( $config['contents'] as $index => $value )
      {
         if( _insertDocAll( $db, $version, $value, $newPath ) == FALSE )
         {
            echo "Failed to insert dir record, id = ".$value['id']."\n" ;
            return false ;
         }
      }
   }
   else
   {
      $path = _cat_path( $path, $config['file'].".html" ) ;

      if( array_key_exists( 'disable', $config ) && $config['disable'] == true )
      {
         return true ;
      }
      if( file_exists( $path ) == FALSE )
      {
         echo "No such file $path\n" ;
         return false ;
      }
      $catid = $config['id'] ;
      $title = $config['cn'] ;
      $contents = file_get_contents( $path ) ;
      $title = mysqli_real_escape_string( $db, $title ) ;
      $contents = mysqli_real_escape_string( $db, $contents ) ;

      $sql = "INSERT INTO `tp_doc$version` (`cat_id`, `title`, `content`, `add_time`, `edition`) VALUES($catid, '$title', '$contents', '".time()."', $version)" ;
      if( mysqli_query( $db, $sql ) == FALSE )
      {
         echo "Falied to insert tp_doc$version record\n" ;
         echo mysqli_error( $db )."\n" ;
         return false ;
      }
   }
   return true ;
}

function insertDoc( $version, $config, $root )
{
   $db = mysqli_connect( "192.168.20.248:3306", "root", "sequoiadb", "cn_comm" ) ;
   if( mysqli_connect_errno() )
   {
      echo "Connect failed: ".mysqli_connect_error()."\n" ;
      return false ;
   }

   mysqli_query( $db, "set names utf8" ) ;

   $rootPath = $root ;
   $rootPath = _cat_path( $rootPath, "build" ) ;
   $rootPath = _cat_path( $rootPath, "mid" ) ;

   if( _insertDocAll( $db, $version, $config, $rootPath ) == FALSE )
   {
      echo "Failed to insert dir all\n" ;
      mysqli_close( $db ) ;
      return false ;
   }
   
   mysqli_close( $db ) ;
   return true ;
}

function _cat_path( $path, $name )
{
   if( getOSInfo() == 'windows' )
   {
      return "$path\\$name" ;
   }
   else
   {
      return "$path/$name" ;
   }
}