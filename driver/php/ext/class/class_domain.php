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

/** \file class_domain.php
    \brief domain class
 */

/**
 * SequoiaDomain Class. To get this Class object must be call SequoiaDB::getDomain.
 */
class SequoiaDomain
{
   /**
    * Alter the current domain.
    *
    * @param $options an array or the string argument. The options user wants to alter.
    *                                                  @code
    *                                                  Groups:    The list of replica group names which the domain is going to contain.
    *                                                             eg:
    *                                                                 array( 'Groups' => array( 'group1', 'group2', 'group3' ) )
    *
    *                                                             it means that domain changes to contain "group1", "group2" or "group3".
    *                                                             We can add or remove groups in current domain. However, if a group has data
    *                                                             in it, remove it out of domain will be failing.
    *
    *                                                  AutoSplit: Alter current domain to have the ability of automatically split or not. 
    *                                                             If this option is set to be true, while creating collection(ShardingType is "hash") in this domain,
    *                                                             the data of this collection will be split(hash split) into all the groups in this domain automatically.
    *                                                             However, it will not automatically split data into those groups which were add into this domain later.
    *                                                             eg:
    *                                                                 array( 'Groups' => array( 'group1', 'group2', 'group3' ), 'AutoSplit' => true )
    *                                                  @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $domainObj -> alter( array( 'Groups' => array( 'group1', 'group2', 'group3' ), 'AutoSplit' => true ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call alter, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function alter( array|string $options ){}

   /**
    * List the collection spaces in domain.
    *
    * @param $condition	an array or the string argument. This parameter is reserved and must be null.
    *
    * @param $selector	an array or the string argument. This parameter is reserved and must be null.
    *
    * @param $orderBy	an array or the string argument. This parameter is reserved and must be null.
    *
    * @param $hint	an array or the string argument. This parameter is reserved and must be null.
    *
    * @return Returns a new SequoiaCursor object.
    *
    * @retval SequoiaCursor Object
    *
    * Example:
    * @code
    * $cursor = $domainObj -> listCS() ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call listCS, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * } 
    * @endcode
   */
   public function listCS( array|string $condition = null, array|string $selector = null, array|string $orderBy = null, array|string $hint = null ){}

   /**
    * List the collections in domain.
    *
    * @param $condition	an array or the string argument. This parameter is reserved and must be null.
    *
    * @param $selector	an array or the string argument. This parameter is reserved and must be null.
    *
    * @param $orderBy	an array or the string argument. This parameter is reserved and must be null.
    *
    * @param $hint	an array or the string argument. This parameter is reserved and must be null.
    *
    * @return Returns a new SequoiaCursor object.
    *
    * @retval SequoiaCursor Object
    *
    * Example:
    * @code
    * $cursor = $domainObj -> listCL() ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call listCL, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * } 
    * @endcode
   */
   public function listCL( array|string $condition = null, array|string $selector = null, array|string $orderBy = null, array|string $hint = null ){}

   /**
    * List the groups in domain.
    *
    * @param $condition	an array or the string argument. This parameter is reserved and must be null.
    *
    * @param $selector	an array or the string argument. This parameter is reserved and must be null.
    *
    * @param $orderBy	an array or the string argument. This parameter is reserved and must be null.
    *
    * @param $hint	an array or the string argument. This parameter is reserved and must be null.
    *
    * @return Returns a new SequoiaCursor object.
    *
    * @retval SequoiaCursor Object
    *
    * Example:
    * @code
    * $cursor = $domainObj -> listGroup() ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call listGroup, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function listGroup( array|string $condition = null, array|string $selector = null, array|string $orderBy = null, array|string $hint = null ){}
}