<?php
/*******************************************************************************
   Copyright (C) 2012-2018 SequoiaDB Ltd.

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

/** \file class_cs.php
    \brief collection space class
 */

/**
 * SequoiaCS Class. To get this Class object must be call SequoiaDB::selectCS or SequoiaDB::getCS.
 */
class SequoiaCS
{
   /**
    * Drop collection space.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cs -> drop() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to drop collection space, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function drop(){}

   /**
    * Get the specified collection space name.
    *
    * @return Returns the collection space name.
    *
    * @retval string &lt;cs_name&gt;
    *
    * Example:
    * @code
    * $csName = $cs -> getName() ;
    * $err = $db -> getLastErrorMsg() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to get collection space name, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Collection space name is: ".$csName ;
    * @endcode
   */
   public function getName(){}
   
   /**
    * Get the specified collection, if is not exist, will auto create.
    *
    * @param $name	the string argument. The collection name.
    *
    * @param $options an array or the string argument. Please reference <a href="http://doc.sequoiadb.com/cn/index-cat_id-1432190821-edition_id-@SDB_SYMBOL_VERSION">here</a> to get the $options's info of create collection.
    *
    * @return Returns a new SequoiaCL object.
    *
    * @retval SequoiaCL Object
    *
    * Example:
    * @code
    * $cl = $cs -> selectCL( 'bar', array( 'Compressed' => true ) ) ;
    * if( empty( $cl ) ) {
    *    $err = $db -> getLastErrorMsg() ;
    *    echo "Failed to call selectCL, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: create auto increment collection
    * @code
    * $err = $cs -> selectCL( 'bar', array( 'AutoIncrement' => array( 'Field' => 'a', 'MaxValue' => 20000 ) ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call selectCL, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function selectCL( string $name, array|string $options = null ){}

   /**
    * Create the specified collection.
    *
    * @param $name   the string argument. The collection name.
    *
    * @param $options The options are as following:
    *        @code
    *        ShardingKey          : Assign the sharding key, foramt: { ShardingKey: { <key name>: <1/-1>} },
    *                               1 indicates positive order, -1 indicates reverse order.
    *                               e.g. array( "ShardingKey" => array( "a" => 1 ) )
    *        ShardingType         : Assign the sharding type, default is "hash"
    *        Partition            : The number of partition, it is valid when ShardingType is "hash",
    *                               the range is [2^3, 2^20], default is 4096
    *        ReplSize             : Assign how many replica nodes need to be synchronized when a write
    *                               request (insert, update, etc) is executed, default is 1
    *        Compressed           : Whether to enable data compression, default is true
    *        CompressionType      : The compression type of data, could be "snappy" or "lzw", default is "lzw"
    *        AutoSplit            : Whether to enable the automatic partitioning, it is valid when ShardingType
    *                               is "hash", defalut is false
    *        Group                : Assign the data group to which it belongs, default: The collection will
    *                               be created in any data group of the domain that the collection belongs to
    *        AutoIndexId          : Whether to build "$id" index, default is true
    *        EnsureShardingIndex  : Whether to build sharding index, default is true
    *        StrictDataMode       : Whether to enable strict date mode in numeric operations, default is false
    *        AutoIncrement        : Assign attributes of an autoincrement field or batch autoincrement fields
    *                               e.g. array( "AutoIncrement" => array( "Field" => "a", "MaxValue" => 2000 ) ),
    *                               array( "AutoIncrement" => array( array( "Field" => "a", "MaxValue" => 2000 ), array( "Field" => "a", "MaxValue" => 4000 ) ) )
    *        LobShardingKeyFormat : Assign the format of lob sharding key, could be "YYYYMMDD", "YYYYMM" or "YYYY".
    *                               It is valid when the collection is main collection
    *        IsMainCL             : Main collection or not, default is false, which means it is not main collection
    *        DataSource           : The name of the date soure used
    *        Mapping              : The name of the collection to be mapped
    *        @endcode
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cs -> createCL( 'bar', array( 'Compressed' => true ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to create collection, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: create auto increment collection
    * @code
    * $err = $cs -> createCL( 'bar', array( 'AutoIncrement' => array( 'Field' => 'a', 'MaxValue' => 20000 ) ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to create collection, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function createCL( string $name, array|string $options = null ){}

   /**
    * Get the specified collection.
    *
    * @param $name	the string argument. The collection name.
    *
    * @return Returns a new SequoiaCL object.
    *
    * @retval SequoiaCL Object
    *
    * Example:
    * @code
    * $cl = $cs -> getCL( 'bar' ) ;
    * if( empty( $cl ) ) {
    *    $err = $db -> getLastErrorMsg() ;
    *    echo "Failed to call getCL, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function getCL( string $name ){}
 
   /**
    * List all collections in current collection space.
    *
    * @return Returns SequoiaCursor object on success, or NULL on failure.
    *
    * @retval SequoiaCursor object or NULL.
    *
    * Example:
    * @code
    * $cursor = $cs -> listCL() ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getLastErrorMsg() ;
    *    echo "Failed to call listCL, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function listCL(){}

   /**
    * Drop the specified collection.
    *
    * @param $name	the string argument. The collection name.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cs -> dropCL( 'bar' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to drop collection, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function dropCL( string $name ){}

   /**
    * Rename collection name
    *
    * @param $oldName   The old collection name
    *
    * @param $newName   The new collection name
    *
    * @param $options   Reserved
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cs -> renameCL( 'bar', 'new_bar' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to rename collection, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function renameCL( string $oldName, string $newName, array|string $options = null ){}

   /**
    * Alter the specified collection space.
    *
    * @param $options   the array or string argument. The options to alter.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cs -> alter( array( 'PageSize' => 4096 ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to alter collection space, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function alter( array|string $options ){}

   /**
    * Alter the specified collection space to set domain.
    *
    * @param $options   the array or string argument. The options to alter.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cs -> setDomain( array( 'Domain' => 'domain' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to alter collection space, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function setDomain( array|string $options ){}

   /**
    * Get the Domain name of current collection space.
    *
    * @return Returns the domain name ( If the collection space does not belong to any domain, return "" ) on success, or NULL on failure.
    *
    * @retval string <domain_name> or NULL
    *
    * Example:
    * @code
    * $domainName = $cs -> getDomainName() ;
    * if( $domainName === NULL ) {    
    *    $err = $db -> getLastErrorMsg() ;
    *    if( $err['errno'] != 0 ) {
    *       echo "Failed to get collection space name, error code: ".$err['errno'] ;
    *       return ;
    * }
    * echo "Domain name is: ".$domainName ;
    * @endcode
   */
   public function getDomainName(  ){}

   /**
    * Alter the specified collection space to remove domain.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cs -> removeDomain() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to alter collection space, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function removeDomain(){}

   /**
    * Alter the specified collection space to enable capped.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cs -> enableCapped() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to alter collection space, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function enableCapped(){}

   /**
    * Alter the specified collection space to disable capped.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cs -> disableCapped() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to alter collection space, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function disableCapped(){}

   /**
    * Alter the specified collection space.
    *
    * @param $options   the array or string argument. The options to alter.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cs -> setAttributes( array( 'PageSize' => 4096 ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to alter collection space, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function setAttributes( array|string $options ){}
}
