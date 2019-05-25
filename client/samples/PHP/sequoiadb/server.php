<?php
/* 接收POST信息 */
$addr = empty($_POST['addr']) ? "" : $_POST['addr'];
$work = empty($_POST['work']) ? "" : $_POST['work'];
$space = empty($_POST['space']) ? "" : $_POST['space'];
$collectname = empty($_POST['collectname']) ? "" : $_POST['collectname'];
$condition = empty($_POST['condition']) ? NULL : $_POST['condition'];
$selected = empty($_POST['selected']) ? NULL : $_POST['selected'];
$orderBy = empty($_POST['orderBy']) ? NULL : $_POST['orderBy'];
$hint = empty($_POST['hint']) ? NULL : $_POST['hint'];
$numToSkip = empty($_POST['numToSkip']) ? 0 : $_POST['numToSkip'];
$numToReturn = empty($_POST['numToReturn']) ? -1 : $_POST['numToReturn'];
$rule = empty($_POST['rule']) ? NULL : $_POST['rule'];
$indexDef = empty($_POST['indexDef']) ? NULL : $_POST['indexDef'];
$indexpName = empty($_POST['indexpName']) ? NULL : $_POST['indexpName'];
$isUnique = empty($_POST['isUnique']) ? NULL : $_POST['isUnique'];
$obj = empty($_POST['obj']) ? "" : $_POST['obj'];
$listType = empty($_POST['listType']) ? 0 : $_POST['listType'];
$pName = empty($_POST['pName']) ? NULL : $_POST['pName'];

$sequoia_db = new SequoiaDB ($addr) ;
if ( $sequoia_db )
{
   if($work == "create_coll_space")
   {
      $sequoia_db->selectCS($space);
      if ( $sequoia_db )
      {
         echo "true";
      }
   }
   else if($work == "del_coll_space")
   {
      $cs = $sequoia_db->selectCS($space);
      if ( $cs )
      {
         var_dump ( $cs->drop() ) ;
      }
      
   }
   else if($work == "list_coll_space")
   {
      $cursor = $sequoia_db->listCSs () ;
      if ( $cursor )
      {
         while( $str = $cursor->getNext () )
         {
            var_dump ( $str ) ;
            echo "<br />" ;
         }
      }
   }
   else if($work == "create_coll")
   {
      $cs = $sequoia_db->selectCS($space) ;
      $cl = $cs->selectCollection($collectname);
      if ( $cl )
      {
         echo "true";
      }
   }
   else if($work == "dele_coll")
   {
      $cs = $sequoia_db->selectCS($space) ;
      $cl = $cs->selectCollection($collectname);
      var_dump ( $cl->drop() );
   }
   else if($work == "list_coll")
   {
      $cursor = $sequoia_db->listCollections () ;
      if ( $cursor )
      {
         while( $str = $cursor->getNext () )
         {
            var_dump (  $str ) ;
            echo "<br />" ;
         }
      }
   }
   else if($work == "query")
   {
      $cs = $sequoia_db->selectCS($space) ;
      $cl = $cs->selectCollection($collectname);
      $cursor = $cl->find ( $condition,$selected,$orderBy,$hint,$numToSkip,$numToReturn ) ;
      if ( $cursor )
      {
         //$cursor->install( array("install"=> false ) ) ;
         while( $srt = $cursor->getNext () )
         {
            var_dump ( $srt );
            echo "<br />" ;
         }
      }
   }
   else if($work == "insert")
   {
      $cs = $sequoia_db->selectCS($space) ;
      $cl = $cs->selectCollection($collectname);
      var_dump ( $cl->insert( $obj ));
   }
   else if($work == "update")
   {
      $cs = $sequoia_db->selectCS($space) ;
      $cl = $cs->selectCollection($collectname);
      var_dump ( $cl->update( $rule, $condition, $hint ));
   }
   else if($work == "delete")
   {
      $cs = $sequoia_db->selectCS($space) ;
      $cl = $cs->selectCollection($collectname);
      var_dump ( $cl->remove($condition,$hint));
   }
   else if($work == "getcount")
   {
      $cs = $sequoia_db->selectCS($space) ;
      $cl = $cs->selectCollection($collectname);
      echo $cl->count($condition);
   }
   else if($work == "create_index")
   {
      $cs = $sequoia_db->selectCS($space) ;
      $cl = $cs->selectCollection($collectname);
      var_dump ( $cl->createIndex($indexDef, $indexpName, $isUnique ));
   }
   else if($work == "del_index")
   {
      $cs = $sequoia_db->selectCS($space) ;
      $cl = $cs->selectCollection($collectname);
      var_dump ( $cl->deleteIndex($indexpName ));
   }
   else if($work == "query_current")
   {
      $cs = $sequoia_db->selectCS($space) ;
      $cl = $cs->selectCollection($collectname);
      $cursor = $cl->find($condition,$selected,$orderBy,$hint,$numToSkip,$numToReturn );
      if ( $cursor )
      {
         $cursor->getNext ();
         var_dump ( $cursor->current());
      }
   }
   else if($work == "update_current")
   {
      $cs = $sequoia_db->selectCS($space) ;
      $cl = $cs->selectCollection($collectname);
      $cursor = $cl->find($condition,$selected,$orderBy,$hint,$numToSkip,$numToReturn );
      if ( $cursor )
      {
         $cursor->getNext ();
         var_dump ( $cursor->updateCurrent($rule ));
      }
   }
   else if($work == "del_current")
   {
      $cs = $sequoia_db->selectCS($space) ;
      $cl = $cs->selectCollection($collectname);
      $cursor = $cl->find($condition,$selected,$orderBy,$hint,$numToSkip,$numToReturn );
      if ( $cursor )
      {
         $cursor->getNext ();
         var_dump ( $cursor->deleteCurrent () );
      }
    
   }
   else if ( $work == "getSnapshot" )
   {
      $cursor = $sequoia_db->getSnapshot ( $listType,
                                           $condition,
                                           $selected,
                                           $orderBy ) ;
      if ( $cursor )
      {
         //$cursor->install ( array ( "install"=>false ) ) ;
         while($srt = $cursor->getNext ())
         {
            var_dump (  $srt );
            echo "<br />" ;
         }
      }
   }
   else if ( $work == "resetSnapshot" )
   {
      var_dump ( $sequoia_db->resetSnapshot ( ) );
   }
   else if ( $work == "getList" )
   {
      $cursor = $sequoia_db->getList ( $listType,
                                       $condition,
                                       $selected,
                                       $orderBy ) ;
      if ( $cursor )
      {
         while($srt = $cursor->getNext ())
         {
            var_dump ( $srt );
            echo "<br />" ;
         }
      }
   }
   else if ( $work == "collectionRename" )
   {
      $cs = $sequoia_db->selectCS($space) ;
      $cl = $cs->selectCollection($collectname);
      var_dump ( $cl->rename ( $pName ) );
   }
   else if ( $work == "getIndex" )
   {
      $cs = $sequoia_db->selectCS($space) ;
      $cl = $cs->selectCollection($collectname);
      $cursor = $cl->getIndex (  $pName ) ;
      if ( $cursor )
      {
         while( $srt = $cursor->getNext () )
         {
            var_dump ( $srt );
            echo "<br />" ;
         }
      }
   }
   var_dump ( $sequoia_db->getError() ) ;
}
$sequoia_db->close();
?>

</head>
</html>

