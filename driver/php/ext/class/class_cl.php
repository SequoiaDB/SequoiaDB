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

/** \file class_cl.php
    \brief collection class
 */

/**
 * SequoiaCL Class. To get this Class object must be call SequoiaDB::getCL or SequoiaCS::selectCL or SequoiaCS::getCL.
 */
class SequoiaCL
{
   /** The flags represent whether bulk insert continue when hitting index key duplicate error. */
   define( "SDB_FLG_INSERT_CONTONDUP",                   0x00000001 ) ;

   /** Force to use specified hint to query, if database have no index assigned by the hint, fail to query. */
   define( "SDB_FLG_FIND_FORCE_HINT",                    0x00000080 ) ;
   /** Enable paralled sub query. */
   define( "SDB_FLG_FIND_PARALLED",                      0x00000100 ) ;
   /** In general, query will not return data until cursor get from database, when add this flag, return data in query response, it will be more high-performance. */
   define( "SDB_FLG_FIND_WITH_RETURNDATA",               0x00000200 ) ;


   /** Force to use specified hint to query, if database have no index assigned by the hint, fail to query */
   define( "SDB_FLG_QUERY_FORCE_HINT",                   0x00000080 ) ;
   /** Enable paralled sub query */
   define( "SDB_FLG_QUERY_PARALLED",                     0x00000100 ) ;
   /** In general, query will not return data until cursor get from database, when add this flag, return data in query response, it will be more high-performance */
   define( "SDB_FLG_QUERY_WITH_RETURNDATA",              0x00000200 ) ;
   /** Enable prepare more data when query */
   define( "SDB_FLG_QUERY_PREPARE_MORE",                 0x00004000 ) ;
   /** The sharding key in update rule is not filtered, when executing findAndUpdate */
   define( "SDB_FLG_QUERY_KEEP_SHARDINGKEY_IN_UPDATE",   0x00008000 ) ;

   /** The sharding key in update rule is not filtered, when executing update or upsert. */
   /** SDB_FLG_QUERY_KEEP_SHARDINGKEY_IN_UPDATE is equal to SDB_FLG_UPDATE_KEEP_SHARDINGKEY, to prevent confusion.*/
   define( "SDB_FLG_UPDATE_KEEP_SHARDINGKEY",            0x00008000 ) ;

   /** Open a new lob only. */
   define( "SDB_LOB_CREATEONLY",                         0x00000001 ) ;
   /** Open an existing lob to read. */
   define( "SDB_LOB_READ",                               0x00000004 ) ;
   /** Open an existing lob to write. */
   define( "SDB_LOB_WRITE",                              0x00000008 ) ;

   /**
    * Drop collection.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example: Drop collection
    * @code
    * $err = $cl -> drop() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to drop collection, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function drop(){}

   /**
    * Alter collection options.
    *
    * @param $options an array or the string argument. New collection options.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example: Alter collection option ReplSize
    * @code
    * $err = $cl -> alter( array( 'ReplSize' => -1 ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to alter collection options, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function alter( array|string $options ){}

   /**
    * Split the specified collection from source replica group to target by percent.
    *
    * @param $sourceGroup the string argument. The source replica group name.
    *
    * @param $targetGroup the string argument. The target replica group name.
    *
    * @param $percent an integer or a double argument. The split percent, Range:(0.0, 100.0].
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cl -> split( 'group1', 'group2', 50 ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to split collection, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function split( string $sourceGroup, string $targetGroup, integer|double $percent ){}

   /**
    * Split the specified collection from source replica group to target by range.
    *
    * @param $sourceGroup the string argument. The source replica group name.
    *
    * @param $targetGroup the string argument. The target replica group name.
    *
    * @param $condition an array or the string argument. The split condition.
    *
    * @param $endCondition an array or the string argument. The split end condition.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cl -> split( 'group1', 'group2', array( 'a' => 1 ), array( 'a' => 100 ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to split collection, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function split( string $sourceGroup, string $targetGroup, array|string $condition, array|string $endCondition = null ){}

   /**
    * Split the specified collection from source replica group to target by range.
    *
    * @param $sourceGroup the string argument. The source replica group name.
    *
    * @param $targetGroup the string argument. The target replica group name.
    *
    * @param $percent an integer or a double argument. The split percent, Range:(0.0, 100.0].
    *
    * @return Returns the result and task id, default return array.
    *
    * @retval array   array( 'errno' => 0, 'taskID' => 1 )
    * @retval string  { "errno": 0, "taskID": 1 }
    *
    * Example:
    * @code
    * $err = $cl -> splitAsync( 'group1', 'group2', 50 ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to split collection, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Task id is: ".$err['taskID'] ;
    * @endcode
   */
   public function splitAsync( string $sourceGroup, string $targetGroup, integer|double $percent ){}

   /**
    * Split the specified collection from source replica group to target by range.
    *
    * @param $sourceGroup the string argument. The source replica group name.
    *
    * @param $targetGroup the string argument. The target replica group name.
    *
    * @param $condition an array or the string argument. The split condition.
    *
    * @param $endCondition an array or the string argument. The split end condition.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0, 'taskID' => 1 )
    * @retval string  { "errno": 0, "taskID": 1
    *
    * Example:
    * @code
    * $err = $cl -> splitAsync( 'group1', 'group2', array( 'a' => 1 ), array( 'a' => 100 ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to split collection, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Task id is: ".$err['taskID'] ;
    * @endcode
   */
   public function splitAsync( string $sourceGroup, string $targetGroup, array|string $condition, array|string $endCondition = null ){}

   /**
    * Get the specified collection full name.
    *
    * @return Returns the collection full name.
    *
    * @retval string &lt;cs_name.cl_name&gt;
    *
    * Example:
    * @code
    * $fullName = $cl -> getFullName() ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to get collection full name, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Collection full name is: ".$fullName ;
    * @endcode
   */
   public function getFullName(){}

   /**
    * Get the specified collection space name.
    *
    * @return Returns the collection space name.
    *
    * @retval string &lt;cs_name&gt;
    *
    * Example:
    * @code
    * $csName = $cl -> getCSName() ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to get collection space name, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Collection space name is: ".$csName ;
    * @endcode
   */
   public function getCSName(){}

   /**
    * Get the specified collection name.
    *
    * @return Returns the collection name.
    *
    * @retval string &lt;cl_name&gt;
    *
    * Example:
    * @code
    * $clName = $cl -> getName() ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to get collection name, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Collection name is: ".$clName ;
    * @endcode
   */
   public function getName(){}

   /**
    * Attach the specified collection.
    *
    * @param $subClFullName	the string argument. The name of the subcollection.
    *
    * @param $options an array or the string argument. The low boudary and up boudary eg: @code
    *                                                                                     array( 'LowBound' => array( '<key>' => <value> ), 'UpBound' => array( '<key>' => <value> ) )
    *                                                                                     @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example: 2 collections have been created, one of which is a vertival partition.
    * @code
    * $err = $veticalCL -> attachCL( 'cs.normalCL', array( 'LowBound' => array( 'id' => 0 ), 'UpBound' => array( 'id' => 100 ) ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to attach collection, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function attachCL( string $subClFullName, array|string $options ){}

   /**
    * Detach the specified collection.
    *
    * @param $subClFullName	the string argument. The name of the subcollection.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example: 2 collections have been created, one of which is a vertival partition and attach collection.
    * @code
    * $err = $veticalCL -> detachCL( 'cs.normalCL' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to detach collection, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function detachCL( string $subClFullName ){}

   /**
    * Insert a record into current collection.
    *
    * @param $record	an array or the string argument. The inserted record, cannot be empty.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0, '_id' => &lt;24 hexadecimal characters&gt; )
    * @retval string  { "errno": 0, "_id": &lt;24 hexadecimal characters&gt; }
    *
    * Example: Record type is php array
    * @code
    * $err = $cl -> insert( array( 'time' => '2012-12-12' ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to insert record, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Insert record id is: ".$err['_id'] ;
    * @endcode
    *
    * Example: Record type is json string
    * @code
    * $err = $cl -> insert( '{ "time": "2012-12-12" }' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to insert record, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Insert record id is: ".$err['_id'] ;
    * @endcode
   */
   public function insert( array|string $record ){}

   /**
    * Insert records into current collection.
    *
    * @param $records	an array or the string argument. The inserted record, cannot be empty.
    *
    * @param $flags an integer argument. While SDB_FLG_INSERT_CONTONDUP is set, if some records hit index key duplicate error,
    *                                    database will skip them and go on inserting. However, while 0 is set, database will stop inserting in that case, and return errno code.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example: Insert an array record
    * @code
    * $err = $cl -> bulkInsert( array( 'a' => 1 ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to insert records, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: Insert the json string
    * @code
    * $err = $cl -> bulkInsert( '{ "a": 2 }' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to insert records, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: Insert multiple records
    * @code
    * $records = array(
    *    array( 'a' => 3 ),
    *    array( 'a' => 4 ),
    *    array( 'a' => 5 ),
    *    array( 'a' => 6 ),
    *    array( 'a' => 7 ),
    *    '{ "a": 8 }',
    *    '{ "a": 9 }',
    *    '{ "a": 10 }',
    *    '{ "a": 11 }',
    *    '{ "a": 12 }'
    * ) ;
    * $err = $cl -> bulkInsert( $records ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to insert records, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: Set $flags SDB_FLG_INSERT_CONTONDUP
    * @code
    * $records = array(
    *    array( 'a' => 3 ),
    *    array( 'a' => 4 ),
    *    array( 'a' => 5 ),
    *    array( 'a' => 6 ),
    *    array( 'a' => 7 ),
    *    '{ "a": 8 }',
    *    '{ "a": 9 }',
    *    '{ "a": 10 }',
    *    '{ "a": 11 }',
    *    '{ "a": 12 }'
    * ) ;
    * $err = $cl -> bulkInsert( $records, SDB_FLG_INSERT_CONTONDUP ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to insert records, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function bulkInsert( array|string $records, integer $flags = 0 ){}

   /**
    * Delete the matching documents in current collection, never rollback if failed.
    *
    * @param $condition	an array or the string argument. The matching rule, delete all the documents if null.
    *
    * @param $hint	an array or the string argument. The hint, automatically match the optimal hint if null.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example: Remove all records
    * @code
    * $err = $cl -> remove() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to remove, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: Remove match condition records
    * @code
    * $err = $cl -> remove( array( 'age' => array( '$lte' => 50 ) ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to remove, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function remove( array|string $condition = null, array|string $hint = null ){}

   /**
    * Update the matching documents in current collection.
    *
    * @param $rule	an array or the string argument. The updating rule, cannot be null.
    *
    * @param $condition	an array or the string argument. The matching rule, update all the documents if null.
    *
    * @param $hint	an array or the string argument. The hint, automatically match the optimal hint if null.
    *
    * @param $flag	an integer argument. The query flag, default to be 0.
    *                                   @code
    *                                   SDB_FLG_UPDATE_KEEP_SHARDINGKEY(0x00008000) : The sharding key in update rule is not filtered.
    *                                   @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cl -> update( '{ "$set": { "phone" : "" } }', array( 'age' => array( '$lt' => 10 ) ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to update, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function update( array|string $rule, array|string $condition = null, array|string $hint = null, integer $flag = 0 ){}

   /**
    * Update the matching documents in current collection, insert if no matching.
    *
    * @param $rule	an array or the string argument. The updating rule, cannot be null.
    *
    * @param $condition	an array or the string argument. The matching rule, update all the documents if null.
    *
    * @param $hint	an array or the string argument. The hint, automatically match the optimal hint if null.
    *
    * @param $setOnInsert	an array or the string argument. The setOnInsert, assigns the specified values to the fileds when insert
    *
    * @param $flag	an integer argument. The query flag, default to be 0.
    *                                   @code
    *                                   SDB_FLG_UPDATE_KEEP_SHARDINGKEY(0x00008000) : The sharding key in update rule is not filtered.
    *                                   @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cl -> upsert( '{ "$set": { "b" : 1 } }', array( 'a' => array( '$gt' => 100 ) ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to upsert, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function upsert( array|string $rule, array|string $condition = null, array|string $hint = null, array|string $setOnInsert = null, integer $flag = 0 ){}

   /**
    * Get the matching records in current collection.
    *
    * @param $condition	an array or the string argument. The matching rule, return all the record if null.
    *
    * @param $selector	an array or the string argument. The selective rule, return the whole record if null.
    *
    * @param $orderBy	an array or the string argument. The The ordered rule, never sort if null.
    *
    * @param $hint	an array or the string argument. The hint, automatically match the optimal hint if null.
    *
    * @param $numToSkip an integer argument.	Skip the first numToSkip records, never skip if this parameter is 0.
    *
    * @param $numToReturn	an integer argument. Only return numToReturn records, return all if this parameter is -1.
    *
    * @param $flag	an integer argument. The query flag, default to be 0.
    *                                   @code
    *                                   SDB_FLG_QUERY_FORCE_HINT(0x00000080)        : Force to use specified hint to query, if database have no index assigned by the hint, fail to query
    *                                   SDB_FLG_QUERY_PARALLED(0x00000100)          : Enable paralled sub query
    *                                   SDB_FLG_QUERY_WITH_RETURNDATA(0x00000200)   : In general, query will not return data until cursor get from database,
    *                                                                                 when add this flag, return data in query response, it will be more high-performance
    *                                   SDB_FLG_QUERY_PREPARE_MORE(0x00004000)      : Enable prepare more data when query
    *                                   SDB_FLG_UPDATE_KEEP_SHARDINGKEY(0x00008000) : The sharding key in update rule is not filtered, when updating records.
    *                                   @endcode
    *
    * @return Returns a new SequoiaCursor object.
    *
    * @retval SequoiaCursor Object
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
   public function find( array|string $condition = null, array|string $selector = null, array|string $orderBy = null, array|string $hint = null, integer $numToSkip = 0, integer $numToReturn = -1, integer $flag = 0 ){}

   /**
    * Get the matching records in current collection and update.
    *
    * @param $rule	an array or the string argument. The update rule, can't be null.
    *
    * @param $condition	an array or the string argument. The matching rule, return all the record if null.
    *
    * @param $selector	an array or the string argument. The selective rule, return the whole record if null.
    *
    * @param $orderBy	an array or the string argument. The The ordered rule, never sort if null.
    *
    * @param $hint	an array or the string argument. The hint, automatically match the optimal hint if null.
    *
    * @param $numToSkip	an integer argument.	Skip the first numToSkip records, never skip if this parameter is 0.
    *
    * @param $numToReturn	an integer argument.	Only return numToReturn records, return all if this parameter is -1.
    *
    * @param $flag	an integer argument.	The query flag, default to be 0.
    *                                   @code
    *                                   SDB_FLG_QUERY_FORCE_HINT(0x00000080)                 : Force to use specified hint to query, if database have no index assigned by the hint, fail to query
    *                                   SDB_FLG_QUERY_PARALLED(0x00000100)                   : Enable paralled sub query
    *                                   SDB_FLG_QUERY_WITH_RETURNDATA(0x00000200)            : In general, query will not return data until cursor get from database,
    *                                                                                          when add this flag, return data in query response, it will be more high-performance
    *                                   SDB_FLG_QUERY_KEEP_SHARDINGKEY_IN_UPDATE(0x00008000) : The sharding key in update rule is not filtered.
    *
    *                                   @endcode
    *
    * @param $returnNew	a boolean argument. When TRUE, returns the updated record rather than the original.
    *
    * @return Returns a new SequoiaCursor object.
    *
    * @retval SequoiaCursor Object
    *
    * Example:
    * @code
    * $cursor = $cl -> findAndUpdate( array( '$set' => array( 'a' => 0 ) ), false, array( 'a' => array( '$gt' => 0 ) ) ) ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call findAndUpdate, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function findAndUpdate( array|string $rule, array|string $condition = null, array|string $selector = null, array|string $orderBy = null, array|string $hint = null, integer $numToSkip = 0, integer $numToReturn = -1, integer $flag = 0, boolean $returnNew = false ){}

   /**
    * Get the matching documents in current collection and remove.
    *
    * @param $condition	an array or the string argument. The matching rule, return all the record if null.
    *
    * @param $selector	an array or the string argument. The selective rule, return the whole record if null.
    *
    * @param $orderBy	an array or the string argument. The The ordered rule, never sort if null.
    *
    * @param $hint	an array or the string argument. The hint, automatically match the optimal hint if null.
    *
    * @param $numToSkip	an integer argument.	Skip the first numToSkip records, never skip if this parameter is 0.
    *
    * @param $numToReturn	an integer argument.	Only return numToReturn records, return all if this parameter is -1.
    *
    * @param $flag	an integer argument.	The query flag, default to be 0.
    *                                   @code
    *                                   SDB_FLG_QUERY_FORCE_HINT(0x00000080)      : Force to use specified hint to query, if database have no index assigned by the hint, fail to query
    *                                   SDB_FLG_QUERY_PARALLED(0x00000100)        : Enable paralled sub query
    *                                   SDB_FLG_QUERY_WITH_RETURNDATA(0x00000200) : In general, query will not return data until cursor get from database,
    *                                                                           when add this flag, return data in query response, it will be more high-performance
    *                                   @endcode
    *
    * @return Returns a new SequoiaCursor object.
    *
    * @retval SequoiaCursor Object
    *
    * Example:
    * @code
    * $cursor = $cl -> findAndRemove( array( 'a' => array( '$gt' => 0 ) ) ) ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call findAndRemove, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function findAndRemove( array|string $condition = null, array|string $selector = null, array|string $orderBy = null, array|string $hint = null, integer $numToSkip = 0, integer $numToReturn = -1, integer $flag = 0 ){}

   /**
    * Get access plan of query
    *
    * @param $condition	an array or the string argument. The matching rule, return all the record if null.
    *
    * @param $selector	an array or the string argument. The selective rule, return the whole record if null.
    *
    * @param $orderBy	an array or the string argument. The ordered rule, never sort if null.
    *
    * @param $hint	an array or the string argument. The hint, automatically match the optimal hint if null.
    *
    * @param $numToSkip	an integer argument.	Skip the first numToSkip records, never skip if this parameter is 0.
    *
    * @param $numToReturn	an integer argument.	Only return numToReturn records, return all if this parameter is -1.
    *
    * @param $flag	an integer argument.	The query flag, default to be 0.
    *                                   @code
    *                                   SDB_FLG_QUERY_FORCE_HINT(0x00000080)      : Force to use specified hint to query, if database have no index assigned by the hint, fail to query
    *                                   SDB_FLG_QUERY_PARALLED(0x00000100)        : Enable paralled sub query
    *                                   SDB_FLG_QUERY_WITH_RETURNDATA(0x00000200) : In general, query will not return data until cursor get from database,
    *                                                                               when add this flag, return data in query response, it will be more high-performance
    *                                   @endcode
    *
    * @param $options	an array or the string argument. The rules of explain, the options are as below:
    *                                   @code
    *                                   Run: Whether execute query explain or not, true for excuting query explain then get
    *                                        the data and time information; false for not excuting query explain but get the
    *                                        query explain information only.
    *                                   e.g. array( 'run' => true )
    *                                   @endcode
    *
    * @return Returns a new SequoiaCursor object.
    *
    * @retval SequoiaCursor Object
    *
    * Example:
    * @code
    * $cursor = $cl -> explain( array( 'Run' => true ) ) ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call explain, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function explain( array|string $condition = null, array|string $selector = null, array|string $orderBy = null, array|string $hint = null, integer $numToSkip = 0, integer $numToReturn = -1, integer $flag = 0, array|string $options = null ){}

   /**
    * Get the count of records in specified collection.
    *
    * @param $condition	an array or the string argument. The matching rule, return the count of all records if this parameter is null.
    *
    * @param $hint	an array or the string argument. The hint, automatically match the optimal hint if null.
    *
    * @return Returns the number of records matching the query.
    *
    * @retval integer|SequoiaINT64 Records number
    *
    * Example:
    * @code
    * $recordNum = $cl -> count() ;
    * if( $recordNum < 0 ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call count, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Collection records number: ".$recordNum ;
    * @endcode
   */
   public function count( array|string $condition = null, array|string $hint = null ){}

   /**
    * Execute aggregate operation in specified collection.
    *
    * @param $aggrObj an array or the string argument. Aggregation parameter,
    *                                if the input string or an array, you can enter only one parameter,
    *                                such as the string: @code
    *                                                    '{ "$project": { "field": 1 } }'
    *                                                    @endcode
    *                                an associative array: @code
    *                                                      array( '$project' => array ( 'field' => 1 ) )
    *                                                      @endcode
    *                                if the input array, you can enter multiple parameters,
    *                                such as: @code
    *                                         array ( '{ "$project": { "field1": 1, "field2": 2 } }', '{ "$project": { "field1": 1 } }' )
    *                                         @endcode
    *                                or @code
    *                                   array ( array ( '$project' => array ( 'field1' => 1, 'field2' => 2 ) ), array ( '$project' => array ( 'field1' => 1 ) ) )
    *                                   @endcode
    *
    * @return Returns a new SequoiaCursor object.
    *
    * @retval SequoiaCursor Object
    *
    * Example:
    * @code
    * $cursor = $cl -> aggregate( array ( array ( '$project' => array ( 'field1' => 1, 'field2' => 2 ) ), array ( '$project' => array ( 'field1' => 1 ) ) ) ) ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call aggregate, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function aggregate( array|string $aggrObj ){}

   /**
    * Create the index in current collection.
    *
    * @param $indexDef an array or the string argument. The index element. e.g. array( 'name' => 1, 'age' => -1 )
    *
    * @param $indexName the string argument. The index name.
    *
    * @param $isUnique a boolean argument. Whether the index elements are unique or not,default is false.
    *
    * @param $isEnforced a boolean argument. Whether the index is enforced unique This element is meaningful when isUnique is set to true.
    *
    * @param $sortBufferSize an integer argument. The size of sort buffer used when creating index, the unit is MB, zero means don't use sort buffer.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    *
    * Example:
    * @code
    * $err = $cl -> createIndex( array( 'name' => 1, 'age' => -1 ), "myIndex" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to create index, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function createIndex( array|string $indexDef, string $indexName, boolean $isUnique = false, boolean $isEnforced = false, integer $sortBufferSize = 64 ){}

   /**
    * Drop the index in current collection.
    *
    * @param $indexName the string argument. The index name.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cl -> dropIndex( "myIndex" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to drop index, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function dropIndex( string $indexName ){}

   /**
    * Get all of or one of the indexes in current collection.
    *
    * @param $indexName the string argument. The index name, returns all of the indexes if this parameter is empty string.
    *
    * @return Returns a new SequoiaCursor object.
    *
    * @retval SequoiaCursor Object
    *
    * Example:
    * @code
    * $cursor = $cl -> getIndex() ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to get indexes, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function getIndex( string $indexName = "" ){}

   /**
    * Create $id index in collection.
    *
    * @param $args an array or the string argument. The arguments of creating id index. set it as null if no args. e.g. array( 'SortBufferSize' => 64 )
    *                           @code
    *                           SortBufferSize : The size of sort buffer used when creating index, the unit is MB,
    *                                            zero means don't use sort buffer
    *                           @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cl -> createIdIndex( array( 'SortBufferSize' => 64 ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to create id index, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function createIdIndex( array|string $args = null ){}

   /**
    * Drop $id index in collection.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cl -> dropIdIndex() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to drop id index, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function dropIdIndex(){}

   /**
    * Create a large object or open a large object to read or write.
    *
    * @param $oid the string argument. The object id.
    *
    * @param $mode an integer argument. The open mode: @code
    *                                                  SDB_LOB_CREATEONLY
    *                                                  SDB_LOB_READ
    *                                                  SDB_LOB_WRITE
    *                                                  @endcode
    *
    * @return Returns a new SequoiaLob object.
    *
    * @retval SequoiaLob Object
    *
    * Example:
    * @code
    * $lobObj = $cl -> openLob( "123456789012345678901234", SDB_LOB_CREATEONLY ) ;
    * if( empty( $lobObj ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to open lob, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function openLob( string $oid, integer $mode ){}

   /**
    * Remove lob
    *
    * @param $oid the string argument. The object id.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cl -> removeLob( "123456789012345678901234" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to remove lob, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function removeLob( string $oid ){}

   /**
    * Truncate lob
    *
    * @param $oid the string argument. The object id.
    *
    * @param $length the integer argument. The truncate length.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $cl -> truncateLob( "123456789012345678901234", 10 ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to truncate lob, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function truncateLob( string $oid, integer|SequoiaINT64 $length ){}

   /**
    * List all the lobs' meta data in current collection.
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
    * $cursor = $cl -> listLob() ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call listLob, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function listLob( array|string $condition = null, array|string $selector = null, array|string $orderBy = null, array|string $hint = null ){}

   /**
    * List all the pieces in the lob.
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
    * $cursor = $cl -> listLobPieces() ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call listLobPieces, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function listLobPieces( array|string $condition = null, array|string $selector = null, array|string $orderBy = null, array|string $hint = null ){}
}