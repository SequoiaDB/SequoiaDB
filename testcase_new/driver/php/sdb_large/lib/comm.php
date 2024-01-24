/****************************************************
@description:      common function warp class
@testlink cases:   
@modify list:
        2016-6-20 wenjing wang init
****************************************************/
<?php
class common
{
    static function IsStandlone($db)
    {
       $db -> list( SDB_LIST_GROUPS );
       $err = $db -> getError();
       if( $err['errno'] === -159 ) //-159: The operation is for coord node only
       {
         echo "   Is standlone mode!! \n";
         return true;
       }
       else
       {
          return false;
       }
    }
}
?>