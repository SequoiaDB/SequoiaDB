<?php
/*******************************************************************************
   Copyright (C) 2012-2014 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*******************************************************************************/

/** \file class_cursor.php
    \brief cursor class
 */

/**
 * SequoiaCursor Class. To get this Class object must be call SequoiaDB::list or SequoiaDB::snapshot or SequoiaCL::find, and so on.
 */
class SequoiaCursor
{
   /**
    * Return the next record to which this cursor points, and advance the cursor.
    *
    * @return Returns the record, default return array.
    *
    * @retval array   record
    * @retval string  record
    *
    * Example:
    * @code
    * $cursor = $cl -> find( array( 'a' => array( '$lte' => 50 ) ) ) ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to find, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function next(){}

   /**
    * Return the current element.
    *
    * @return Returns the record, default return array.
    *
    * @retval array   record
    * @retval string  record
    *
    * Example:
    * @code
    * $cursor = $cl -> find( array( 'a' => array( '$lte' => 50 ) ) ) ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to find, error code: ".$err['errno'] ;
    *    return ;
    * }
    * if( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    *    $record = $cursor -> current() ;
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function current(){}
}