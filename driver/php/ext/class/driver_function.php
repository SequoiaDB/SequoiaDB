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

/** \file driver_function.php
    \brief driver function
 */
 
/**
 * Open cache strategy to improve performance
 *
 * @param $config an array argument. The configuration, the options as below:
 *                                     @code
 *                                     enableCacheStrategy : a boolean argument. The flag to open the cache strategy.
 *                                     cacheTimeInterval   : an integer argument. The life cycle of cached object.
 *                                     @endcode
 *
 * @return Returns the number of errno.
 *
 * @retval integer errno
 *
 * Example: open cache
 * @code
 * $rc = sdbInitClient( array( 'enableCacheStrategy' => true, 'cacheTimeInterval' => 200 ) ) ;
 * if( $rc != 0 )
 * {
 *    echo "Failed to open cache, error code: ".$rc ;
 *    return ;
 * }
 * $db = new SequoiaDB() ;
 * $err = $db -> connect( "192.168.1.10:11810" ) ;
 * if( $err['errno'] != 0 ) {
 *    echo "Failed to connect database, error code: ".$err['errno'] ;
 *    return ;
 * }
 * $cs = $db -> selectCS( 'foo' ) ;
 * if( empty( $cs ) ) {
 *    $err = $db -> getError() ;
 *    echo "Failed to call selectCS, error code: ".$err['errno'] ;
 *    return ;
 * }
 * @endcode
 *
*/
function sdbInitClient( $config ){}