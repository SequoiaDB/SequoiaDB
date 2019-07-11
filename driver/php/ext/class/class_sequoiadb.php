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

/** \file class_sequoiadb.php
    \brief db class
 */

/**
 * Class for create an object of the db
 */
class SequoiaDB
{
   /** Get the snapshot of all the contexts. */
   define( "SDB_SNAP_CONTEXTS",            0 ) ;
   /** Get the snapshot of current context */
   define( "SDB_SNAP_CONTEXTS_CURRENT",    1 ) ;
   /** Get the snapshot of all the sessions. */
   define( "SDB_SNAP_SESSIONS",            2 ) ;
   /** Get the snapshot of current session. */
   define( "SDB_SNAP_SESSIONS_CURRENT",    3 ) ;
   /** Get the snapshot of all the collections. */
   define( "SDB_SNAP_COLLECTIONS",         4 ) ;
   /** Get the snapshot of all the collection spaces. */
   define( "SDB_SNAP_COLLECTIONSPACES",    5 ) ;
   /** Get the snapshot of the database. */
   define( "SDB_SNAP_DATABASE",            6 ) ;
   /** Get the snapshot of the system. */
   define( "SDB_SNAP_SYSTEM",              7 ) ;
   /** Get the snapshot of the catalog. */
   define( "SDB_SNAP_CATALOG",                8 ) ;
   /** Get the snapshot of all the transactions. */
   define( "SDB_SNAP_TRANSACTIONS",        9 ) ;
   /** Get the snapshot of current transaction. */
   define( "SDB_SNAP_TRANSACTIONS_CURRENT",10 ) ;
   /** Get the snapshot of cached access plans. */
   define( "SDB_SNAP_ACCESSPLANS",         11 ) ;
   /** Get the snapshot of node health detection. */
   define( "SDB_SNAP_HEALTH", 12 ) ;

   /**
    * Get the snapshot of all the collections.
    *
    * @deprecated
    *
    * @see SDB_SNAP_COLLECTIONS
   */
   define( "SDB_SNAP_COLLECTION",          4 ) ;

   /**
    * Get the snapshot of all the collection spaces.
    *
    * @deprecated
    *
    * @see SDB_SNAP_COLLECTIONSPACES
   */
   define( "SDB_SNAP_COLLECTIONSPACE",     5 ) ;

   /**
    * Get the snapshot of the catalog.
    *
    * @deprecated
    *
    * @see SDB_SNAP_CATALOG
   */
   define( "SDB_SNAP_CATA",             8 ) ;

   /**
    * Get the snapshot of all the transactions.
    *
    * @deprecated
    *
    * @see SDB_SNAP_TRANSACTIONS
   */
   define( "SDB_SNAP_TRANSACTION",         9 ) ;

   /**
    * Get the snapshot of current transaction.
    *
    * @deprecated
    *
    * @see SDB_SNAP_TRANSACTIONS_CURRENT
   */
   define( "SDB_SNAP_TRANSACTION_CURRENT", 10 ) ;

   /** Get the list of the contexts. */
   define( "SDB_LIST_CONTEXTS",         0 ) ;
   /** Get the list of current context. */
   define( "SDB_LIST_CONTEXTS_CURRENT", 1 ) ;
   /** Get the list of the sessions. */
   define( "SDB_LIST_SESSIONS",         2 ) ;
   /** Get the list of current session. */
   define( "SDB_LIST_SESSIONS_CURRENT", 3 ) ;
   /** Get the list of the collections. */
   define( "SDB_LIST_COLLECTIONS",      4 ) ;
   /** Get the list of the collecion spaces. */
   define( "SDB_LIST_COLLECTIONSPACES", 5 ) ;
   /** Get the list of the storage units. */
   define( "SDB_LIST_STORAGEUNITS",     6 ) ;
   /** Get the list of the replica groups ( only applicable in sharding env ). */
   define( "SDB_LIST_GROUPS",           7 ) ;
   /** Get the list of the stored procedures ( only applicable in sharding env ). */
   define( "SDB_LIST_STOREPROCEDURES",  8 ) ;
   /** Get the list of the domains ( only applicable in sharding env ). */
   define( "SDB_LIST_DOMAINS",          9 ) ;
   /** Get the list of the tasks ( only applicable in sharding env ). */
   define( "SDB_LIST_TASKS",            10 ) ;
   /** Get all the transactions information. */
   define( "SDB_LIST_TRANSACTIONS",     11 ) ;
   /** Get the transactions information of current session. */
   define( "SDB_LIST_TRANSACTIONS_CURRENT", 12 ) ;
   /** Get the list of the collections in specified domain. */
   define( "SDB_LIST_CL_IN_DOMAIN",     129 ) ;
   /** Get the list of the collection spaces in specified domain. */
   define( "SDB_LIST_CS_IN_DOMAIN",     130 ) ;

   /**
    * SequoiaDB class constructor.
    *
    * @param $address	an array or the string argument. The Host Name or IP Address and The Service Name or Port of Database Server.
    *
    * @param $userName	the string argument. The User's Name of the account.
    *
    * @param $password	the string argument. The Password of the account.
    *
    * @param $useSSL	a boolean argument. Connect to database with ssl.
    *
    * Example: 1. Using the connect function to connect to the database.
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: 2. Connect to the database.
    * @code
    * $db = new SequoiaDB( "192.168.1.10:11810" ) ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: 3. Connect to the database, using the default service name. The default service name is 11810.
    * @code
    * $db = new SequoiaDB( "192.168.1.10" ) ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: 4. Connect to the running database one.
    * @code
    * $db = new SequoiaDB( [ "192.168.1.10:11810", "192.168.1.11:11810", "192.168.1.12:11810" ] ) ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: 5. Connect to the authentication database.
    * @code
    * $db = new SequoiaDB( "192.168.1.10:11810", "admin", "123456" ) ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: 6. Connect to the running authentication database one.
    * @code
    * $db = new SequoiaDB( [ "192.168.1.10:11810", "192.168.1.11:11810", "192.168.1.12:11810" ], "admin", "123456" ) ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: 7. Connect to the database with ssl.
    * @code
    * $db = new SequoiaDB( "192.168.1.10:11810", "", "", true ) ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database with ssl, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: 8. Using the connect function to connect to the database with ssl.
    * @code
    * $db = new SecureSdb() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database with ssl, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: 9. Connect to the database with ssl.
    * @code
    * $db = new SecureSdb( "192.168.1.10:11810" ) ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database with ssl, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function __construct( array|string $address, string $userName = "", string $password = "", boolean $useSSL = false ){}

   /**
    * SequoiaDB class constructor.
    *
    * @param $options	an array or the string argument. Set driver options and return the current options.
    *                              the options are as below:
    *                              @code
    *                              install: Set the type of return value for all return result function.
    *                                       The default is true, return an array;
    *                                       set the false, return the string.
    *                              @endcode
    *
    * @return Returns the current options.
    *
    * @retval array   array( 'install' => true )
    * @retval string  { "install": false }
    *
    * Example: 1. Set to return an array type
    * @code
    * $db = new SequoiaDB() ;
    * $db -> install( array( 'install' => true ) ) ;
    * $err = $db -> getError() ;
    * echo "The result is an array type. " ;
    * var_dump( $err ) ;
    * //output: array(1){ ["errno"] => int(0) }
    * @endcode
    *
    * Example: 2. Set to return the string type
    * @code
    * $db = new SequoiaDB() ;
    * $db -> install( array( 'install' => false ) ) ;
    * $err = $db -> getError() ;
    * echo "The result is the string type. " ;
    * var_dump( $err ) ;
    * //output: string(14) "{ "errno": 0 }"
    * @endcode
    *
    * Example: 3. Get the driver current options
    * @code
    * $db = new SequoiaDB() ;
    * $options = $db -> install() ;
    * var_dump( $options ) ;
    * @endcode
   */
   public function install( array|string $options = null ){}

   /**
    * When function return value is result, the return content contains the error code. but a small part of function does not return an error code, So you can call getError() to retrieve the error code.
    *
    * @return Returns the result of the last operation, default return array, set the return type by using the install() function.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 )
    * {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cl = $db -> getCL( "foo.bar" ) ;
    * echo "Get the error code for the getCL function." ;
    * $err = $db -> getError() ;
    * var_dump( $err ) ;
    * @endcode
   */
   public function getError(){}

   /**
    * Connect to database.
    *
    * @param $address	an array or the string argument. The Host Name or IP Address and The Service Name or Port of Database Server.
    *
    * @param $userName	the string argument. The User's Name of the account.
    *
    * @param $password	the string argument. The Password of the account.
    *
    * @param $useSSL	a boolean argument. Connect to database with ssl.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example: 1. Connect to the default address of the database. The default address is 127.0.0.1:11810.
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: 2. Connect to the database.
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: 3. Connect to the database, using the default service name. The default service name is 11810.
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: 4. Connect to the running database one.
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( [ "192.168.1.10:11810", "192.168.1.11:11810", "192.168.1.12:11810" ] ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: 5. Connect to the authentication database.
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810", "admin", "123456" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: 6. Connect to the running authentication database one.
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( [ "192.168.1.10:11810", "192.168.1.11:11810", "192.168.1.12:11810" ], "admin", "123456" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: 7. Connect to the database with ssl.
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810", "", "", true ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database with ssl, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function connect( array|string $address = "127.0.0.1:11810", string $userName = "", string $password = "", boolean $useSSL = false ){}

   /**
    * Disconnect to database.
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $db -> close() ;
    * @endcode
   */
   public function close(){}

   /**
    * Judge whether the connection is valid.
    *
    * @return Returns the result.
    *
    * @retval boolean isConnect
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $status = $db -> isValid() ;
    * echo "Current connection status: ".$status ;
    * $db -> close() ;
    * $status = $db -> isValid() ;
    * echo "Current connection status: ".$status ;
    * @endcode
   */
   public function isValid(){}

   /**
    * Sync database which are specified.
    *
    * @param $options an array or the string argument. The control options:
    *                                               @code
    *                                               Deep              : (INT32) Flush with deep mode or not. 1 in default. 0 for non-deep mode,1 for deep mode,-1 means use the configuration with server.
    *                                               Block             : (Bool) Flush with block mode or not. false in default.
    *                                               CollectionSpace   : (String) Specify the collectionspace to sync. If not set, will sync all the collectionspaces and logs, otherwise, will only sync the collectionspace specified.
    *                                               Location Elements	: (Only take effect in coordinate nodes) GroupID:INT32, GroupName:String, NodeID:INT32, HostName:String, svcname:String ...
    *                                               @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * 1. Deep persistence for all collections and logs.
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $result = $db -> syncDB() ;
    * var_dump( $result ) ;
    * @endcode
    * 2. Deep persistence for collection space foo.
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $result = $db -> syncDB( array( 'CollectionSpace' => 'foo' ) ) ;
    * var_dump( $result ) ;
    * @endcode
    * 3. For the data group group1 for depth and blocking persistence.
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $result = $db -> syncDB( array( 'GroupName' => 'group1', 'Block' => true ) ) ;
    * var_dump( $result ) ;
    * @endcode
   */
   public function syncDB( array|string $options = null ){}

   /**
    * Get the snapshot.
    *
    * @param $type	an integer argument. The snapshot type as below: @code
    *                                                               SDB_SNAP_CONTEXTS
    *                                                               SDB_SNAP_CONTEXTS_CURRENT
    *                                                               SDB_SNAP_SESSIONS
    *                                                               SDB_SNAP_SESSIONS_CURRENT
    *                                                               SDB_SNAP_COLLECTIONS
    *                                                               SDB_SNAP_COLLECTIONSPACES
    *                                                               SDB_SNAP_DATABASE
    *                                                               SDB_SNAP_SYSTEM
    *                                                               SDB_SNAP_CATALOG
    *                                                               SDB_SNAP_TRANSACTIONS
    *                                                               SDB_SNAP_TRANSACTIONS_CURRENT
    *                                                               SDB_SNAP_ACCESSPLANS
    *                                                               SDB_SNAP_HEALTH
    *                                                               @endcode
    *
    * @param $condition an array or the string argument. The matching rule, match all the documents if null.
    *
    * @param $selector an array or the string argument. The selective rule, return the whole document if null.
    *
    * @param $orderBy an array or the string argument. The ordered rule, never sort if null.
    *
    * @param $hint	an array or the string argument. This parameter is reserved and must be null.
    *
    * @return Returns a new SequoiaCursor object.
    *
    * @retval SequoiaCursor Object
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cursor = $db -> snapshot( SDB_SNAP_CONTEXTS ) ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call snapshot, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function snapshot( integer $type, array|string $condition = null, array|string $selector = null, array|string $orderBy = null, array|string $hint = null ){}

   /**
    * Reset the snapshot.
    *
    * @param $options an array or the string argument. The control options:
    *        @code
    *        Type            : (String) Specify the snapshot type to be reset (default is "all"):
    *                          "sessions"
    *                          "sessions current"
    *                          "database"
    *                          "health"
    *                          "all"
    *        SessionID       : (Int32) Specify the session ID to be reset.
    *        Other Options   : Some of other options are as below:(please visit the official website to search "Location Elements" for more detail.)
    *                          GroupID:INT32,
    *                          GroupName:String,
    *                          NodeID:INT32,
    *                          HostName:String,
    *                          svcname:String
    *                          ...
    *        @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> resetSnapshot() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to reset snapshot, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function resetSnapshot( array|string $options = null ){}

   /**
    * Get the specified list.
    *
    * @param $type	an integer argument. The list type as below: @code
    *                                                           SDB_LIST_CONTEXTS
    *                                                           SDB_LIST_CONTEXTS_CURRENT
    *                                                           SDB_LIST_SESSIONS
    *                                                           SDB_LIST_SESSIONS_CURRENT
    *                                                           SDB_LIST_COLLECTIONS
    *                                                           SDB_LIST_COLLECTIONSPACES
    *                                                           SDB_LIST_STORAGEUNITS
    *                                                           SDB_LIST_GROUPS
    *                                                           SDB_LIST_STOREPROCEDURES
    *                                                           SDB_LIST_DOMAINS
    *                                                           SDB_LIST_TASKS
    *                                                           SDB_LIST_TRANSACTIONS
    *                                                           SDB_LIST_TRANSACTIONS_CURRENT
    *                                                           SDB_LIST_CL_IN_DOMAIN
    *                                                           SDB_LIST_CS_IN_DOMAIN
    *                                                           @endcode
    *
    * @param $condition an array or the string argument. The matching rule, match all the documents if null.
    *
    * @param $selector an array or the string argument. The selective rule, return the whole document if null.
    *
    * @param $orderBy an array or the string argument. The ordered rule, never sort if null.
    *
    * @param $hint	an array or the string argument. This parameter is reserved and must be null.
    *
    * @return Returns a new SequoiaCursor object.
    *
    * @retval SequoiaCursor Object
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cursor = $db -> list( SDB_LIST_CONTEXTS ) ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call list, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function list( integer $type, array|string $condition = null, array|string $selector = null, array|string $orderBy = null, array|string $hint = null ){}

   /**
    * List all collection space of current database(include temporary collection space)
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
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cursor = $db -> listCS() ;
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
    * Get the specified collection space, if is not exist,will auto create.
    *
    * @param $name	the string argument. The collection space name.
    *
    * @param $options an array or the string argument. When the collection space is created, $options into force. The options specified by use.
    *                                                  e.g. @code
    *                                                       array( 'PageSize' => 4096, 'Domain' => 'mydomain' )
    *                                                       @endcode
    *
    *                                                  @code
    *                                                  PageSize   : Assign the pagesize of the collection space
    *                                                  Domain     : Assign which domain does current collection space belong to
    *                                                  @endcode
    *
    * @return Returns a new SequoiaCS object.
    *
    * @retval SequoiaCS Object
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cs = $db -> selectCS( 'foo', array( 'PageSize' => 4096 ) ) ;
    * if( empty( $cs ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call selectCS, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function selectCS( string $name, array|string $options = null ){}

   /**
    * Get the specified collection space, if is not exist,will auto create.
    *
    * @param $name	the string argument. The collection space name.
    *
    * @param $pageSize an integer argument. Assign the pagesize of the collection space.
    *
    * @return Returns a new SequoiaCS object.
    *
    * @retval SequoiaCS Object
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cs = $db -> selectCS( 'foo', 4096 ) ;
    * if( empty( $cs ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call selectCS, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function selectCS( string $name, integer $pageSize = null ){}

   /**
    * Create the specified collection space.
    *
    * @param $name	the string argument. The collection space name.
    *
    * @param $options an array or the string argument. The options specified by use.
    *                                                  e.g. @code
    *                                                       array( 'PageSize' => 4096, 'Domain' => 'myDomain' )
    *                                                       @endcode
    *
    *                                                  @code
    *                                                  PageSize   : Assign the pagesize of the collection space
    *                                                  Domain     : Assign which domain does current collection space belong to
    *                                                  @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> createCS( 'foo', array( 'PageSize' => 4096 ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call createCS, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function createCS( string $name, array|string $options = null ){}

   /**
    * Get the specified collection space.
    *
    * @param $name	the string argument. The collection space name.
    *
    * @return Returns a new SequoiaCS object.
    *
    * @retval SequoiaCS Object
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cs = $db -> getCS( 'foo' ) ;
    * if( empty( $cs ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call getCS, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function getCS( string $name ){}

   /**
    * Drop the specified collection space.
    *
    * @param $name	the string argument. The collection space name.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> dropCS( 'foo' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to drop collection space, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function dropCS( string $name ){}

   /**
    * List all collection of current database(not include temporary collection of temporary collection space)
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
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cursor = $db -> listCL() ;
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
    * Get the specified collection.
    *
    * @param $fullName	the string argument. The collection full name.
    *
    * @return Returns a new SequoiaCL object.
    *
    * @retval SequoiaCL Object
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cl = $db -> getCL( 'foo.bar' ) ;
    * if( empty( $cl ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call getCL, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function getCL( string $fullName ){}

   /**
    * Truncate the collection.
    *
    * @param $fullName	the string argument. The collection full name.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> truncate( 'foo.bar' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to truncate the collection, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function truncate( string $fullName ){}

   /**
    * List the domain.
    *
    * @param $condition	an array or the string argument. The matching rule, return all the record if null.
    *
    * @param $selector	an array or the string argument. The selective rule, return the whole record if null.
    *
    * @param $orderBy	an array or the string argument. The The ordered rule, never sort if null.
    *
    * @param $hint	an array or the string argument. This parameter is reserved and must be null.
    *
    * @return Returns a new SequoiaCursor object.
    *
    * @retval SequoiaCursor Object
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cursor = $db -> listDomain() ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call listDomains, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function listDomain( array|string $condition = null, array|string $selector = null, array|string $orderBy = null, array|string $hint = null ){}

   /**
    * Create a domain.
    *
    * @param $name	the string argument. The name of the domain.
    *
    * @param $options an array or the string argument. The options for the domain. The options are as below:
    *                                                  @code
    *                                                  Groups     : The list of replica groups names which the domain is going to contain.
    *                                                               eg: array( 'Groups' => array( "group1", "group2", "group3" ) )
    *                                                               If this argument is not included, the domain will contain all replica groups in the cluster.
    *                                                  AutoSplit  : If this option is set to be true, while creating collection(ShardingType is "hash") in this domain,
    *                                                               the data of this collection will be split(hash split) into all the groups in this domain automatically.
    *                                                               However, it not automatically split data into those groups which were add into this domain later.
    *                                                               eg: array( 'Groups' => array( "group1", "group2", "group3" ), 'AutoSplit' => true )
    *                                                  @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> createDomain( 'myDomain', array( 'Groups' => array( "group1", "group2", "group3" ), 'AutoSplit' => true ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to create domain, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function createDomain( string $name, array|string $options = null ){}

   /**
    * Get a domain.
    *
    * @param $name	the string argument. The name of the domain.
    *
    * @return Returns a new SequoiaDomain object.
    *
    * @retval SequoiaDomain Object
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $domainObj = $db -> getDomain( 'myDomain' ) ;
    * if( empty( $domainObj ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call getDomain, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function getDomain( string $name ){}

   /**
    * Drop a domain.
    *
    * @param $name	the string argument. The name of the domain.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> dropDomain( 'myDomain' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to drop domain, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function dropDomain( string $name ){}

   /**
    * List all the replica groups of current database.
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
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cursor = $db -> listGroup() ;
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

   /**
    * Get the specified replica group.
    *
    * @return Returns a new SequoiaGroup object.
    *
    * @retval SequoiaGroup Object
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $groupObj = $db -> getGroup( 'myGroup' ) ;
    * if( empty( $groupObj ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call getGroup, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function getGroup( string $name ){}

   /**
    * Create the specified replica group.
    *
    * @param $name	the string argument. The name of the replica group.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> createGroup( 'myGroup' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to create group, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function createGroup( string $name ){}

   /**
    * Remove the specified replica group.
    *
    * @param $name	the string argument. The name of the replica group.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> removeGroup( 'myGroup' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to remove group, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function removeGroup( string $name ){}

   /**
    * Create a catalog replica group.
    *
    * @param $hostName	the string argument. The hostname for the catalog replica group.
    *
    * @param $serviceName	the string argument. The servicename for the catalog replica group.
    *
    * @param $databasePath	the string argument. The path for the catalog replica group.
    *
    * @param $configure	the string argument. The configurations for the catalog replica group.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> createCataGroup( 'host1', '11900', '/opt/sequoiadb/database/catalog/11900' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to create catalog group, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function createCataGroup( string $hostName, string $serviceName, string $databasePath, array|string $configure = null ){}

   /**
    * Executing SQL command.
    *
    * @param $sql	the string argument. The SQL command.
    *
    * @return Returns a new SequoiaCursor object.
    *
    * @retval SequoiaCursor Object
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cursor = $db -> execSQL( 'select * from foo.bar' ) ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call execSQL, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function execSQL( string $sql ){}

   /**
    * Executing SQL command for updating.
    *
    * @param $sql	the string argument. The SQL command.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> execUpdateSQL( 'create collectionspace foo' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call execUpdateSQL, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function execUpdateSQL( string $sql ){}

   /**
    * Create an account.
    *
    * @param $userName	the string argument. The user name of the account.
    *
    * @param $passwd	the string argument. The password of the account.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> createUser( 'mike', '123456789' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to create user, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function createUser( string $userName, string $passwd ){}

   /**
    * Delete an account.
    *
    * @param $userName	the string argument. The user name of the account.
    *
    * @param $passwd	the string argument. The password of the account.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> removeUser( 'mike', '123456789' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to remove user, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function removeUser( string $userName, string $passwd ){}

   /**
    * Flush the options to configure file.
    *
    * @param $options an array or the string argument. The configure infomation.
    *                                                  e.g. @code
    *                                                       array( 'Global' => true )
    *                                                       @endcode
    *                                                       @code
    *                                                       Global :  In cluster environment, passing array( 'Global' => true ) will flush data and catalog configuration file, while passing array( 'Global' => false ) will flush coord configuration file.
    *                                                                 In stand-alone environment, both them have the same behaviour.
    *                                                       @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> flushConfigure( array( 'Global' => true ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call flushConfigure, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function flushConfigure( array|string $options ){}

   /**
    * List store procedures.
    *
    * @param $condition an array or the string argument. The condition of list.
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
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cursor = $db -> listProcedure( array( 'name' => 'sum' ) ) ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call listProcedure, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function listProcedure( array|string $condition = null, array|string $selector = null, array|string $orderBy = null, array|string $hint = null ){}

   /**
    * Create a store procedure.
    *
    * @param $code the string argument. The code of store procedures.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> createJsProcedure( 'function sum( a,b ){ return a + b ; }' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call createJsProcedure, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function createJsProcedure( string $code ){}

   /**
    * Remove a store procedure.
    *
    * @param $name the string argument. The name of store procedure.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> removeProcedure( 'sum' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call removeProcedure, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function removeProcedure( string $name ){}

   /**
    * Eval javascript code.
    *
    * @param $code the string argument. The code to eval.
    *
    * @return Returns javascript return value
    *
    * @retval void
    *
    * @retval string
    * @retval integer
    * @retval SequoiaINT64
    * @retval double
    * @retval array	record
    * @retval string	record
    * @retval boolean
    * @retval SequoiaCursor Object
    * @retval SequoiaCS Object
    * @retval SequoiaCL Object
    * @retval SequoiaGroup Object
    * @retval SequoiaNode Object
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $result = $db -> evalJs( 'sum( 1, 2 );' ) ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) ) {
    *    echo "Failed to call evalJs, error code: ".$err['errno'] ;
    *    if( strlen( $result ) > 0 )
    *    {
    *       echo $result ;
    *    }
    *    return ;
    * }
    * $type = gettype( $result ) ;
    * switch( $type )
    * {
    * case 'boolean':
    * case 'integer':
    * case 'double':
    * case 'string':
    *    echo "Value is ".$result ;
    *    break ;
    * case 'array':
    *    echo "Value is " ;
    *    var_dump( $result ) ;
    *    break ;
    * case 'resource':
    *    $resourceType = get_resource_type( $result ) ;
    *    if( $resourceType == 'SequoiaINT64' )
    *    {
    *        echo "Value is ".$result ;
    *    }
    *    else if( $resourceType == 'SequoiaCS' )
    *    {
    *        echo "Get a collection space object" ;
    *    }
    *    else if( $resourceType == 'SequoiaCL' )
    *    {
    *        echo "Get a collection object" ;
    *    }
    *    else if( $resourceType == 'SequoiaGroup' )
    *    {
    *        echo "Get a group object" ;
    *    }
    *    else if( $resourceType == 'SequoiaNode' )
    *    {
    *        echo "Get a node object" ;
    *    }
    *    else if( $resourceType == 'SequoiaCursor' )
    *    {
    *       while( $record = $result -> next() ) {
    *          var_dump( $record ) ;
    *       }
    *    }
    *    break ;
    * }
    * @endcode
   */
   public function evalJs( string $code ){}

   /**
    * Transaction begin.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cl = $db -> getCL( 'foo.bar' ) ;
    * if( empty( $cl ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call getCL, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> transactionBegin() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call transactionBegin, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $cl -> insert( array( 'a' => 1 ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call insert, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> transactionCommit() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call transactionCommit, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function transactionBegin(){}

   /**
    * Transaction commit.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cl = $db -> getCL( 'foo.bar' ) ;
    * if( empty( $cl ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call getCL, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> transactionBegin() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call transactionBegin, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $cl -> insert( array( 'a' => 1 ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call insert, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> transactionCommit() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call transactionCommit, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function transactionCommit(){}

   /**
    * Transaction rollback.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cl = $db -> getCL( 'foo.bar' ) ;
    * if( empty( $cl ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call getCL, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> transactionBegin() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call transactionBegin, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $cl -> insert( array( 'a' => 1 ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call insert, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> transactionRollback() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call transactionRollback, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function transactionRollback(){}

   /**
    * Backup the whole database or specifed replica group.
    *
    * @param $options an array or the string argument. Contains a series of backup configuration infomations.
    *                                                  Backup the whole cluster if null. The "options" contains 5 options as below.
    *                                                  All the elements in options are optional.
    *                                                  e.g. @code
    *                                                       array( 'GroupName' => array( 'RGName1', 'RGName2' ), 'Path' => '/opt/sequoiadb/backup', 'Name' => 'backupName', 'Description' => 'It is my backup', 'EnsureInc' => true, 'OverWrite' => true )
    *                                                       @endcode
    *                                                       @code
    *                                                       GroupID       : The id(s) of replica group(s) which to be backuped
    *                                                       GroupName     : The replica groups which to be backuped
    *                                                       Path          : The backup path, if not assign, use the backup path assigned in the configuration file,
    *                                                                       the path support to use wildcard(%g/%G:group name, %h/%H:host name, %s/%S:service name).
    *                                                                       e.g.
    *                                                                       array( 'Path' => '/opt/sequoiadb/backup/%g' )
    *
    *                                                       isSubDir      : Whether the path specified by paramer "Path" is a subdirectory of the path specified in the configuration file, default to be false
    *                                                       Name          : The name for the backup
    *                                                       Prefix        : The prefix of name for the backup, default to be null.
    *                                                                       e.g.
    *                                                                       array( 'Prefix' => '%g_bk_' )
    *
    *                                                       EnableDateDir : Whether turn on the feature which will create subdirectory named to current date like "YYYY-MM-DD" automatically, default to be false
    *                                                       Description   : The description for the backup
    *                                                       EnsureInc     : Whether excute increment synchronization, default to be false
    *                                                       OverWrite     : Whether overwrite the old backup file, default to be false
    *                                                       @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> backupOffline( array( 'Name' => 'myBackup_1' ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call backupOffline, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function backupOffline( array|string $options ){}

   /**
    * List the backups.
    *
    * @param $options	  an array or the string argument. Contains configuration infomations for remove backups, list all the backups in the default backup path if null.
    *                                                    The "options" contains 3 options as below. All the elements in options are optional.
    *                                                    e.g.
    *                                                    @code
    *                                                    array( 'GroupName' => array( 'RGName1', 'RGName2' ), 'Path' => '/opt/sequoiadb/backup', 'Name' => 'backupName' )
    *                                                    @endcode
    *                                                    @code
    *                                                    GroupName   : Assign the backups of specifed replica groups to be list
    *                                                    Path        : Assign the backups in specifed path to be list, if not assign, use the backup path asigned in the configuration file
    *                                                    Name        : Assign the backups with specifed name to be list
    *                                                    @endcode
    *
    * @param $condition	an array or the string argument. The matching rule, return all the record if null.
    *
    * @param $selector 	an array or the string argument. The selective rule, return the whole record if null.
    *
    * @param $orderBy  	an array or the string argument. The ordered rule, never sort if null.
    *
    * @param $hint	an array or the string argument. This parameter is reserved and must be null.
    *
    * @return Returns a new SequoiaCursor object.
    *
    * @retval SequoiaCursor Object
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cursor = $db -> listBackup( array( 'Name' => 'myBackup_1' ) ) ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call listBackup, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function listBackup( array|string $options, array|string $condition = null, array|string $selector = null, array|string $orderBy = null, array|string $hint = null ){}

   /**
    * Remove the backups.
    *
    * @param $options	  an array or the string argument. Contains configuration infomations for remove backups, list all the backups in the default backup path if null.
    *                                                    The "options" contains 3 options as below. All the elements in options are optional.
    *                                                    eg: array( 'GroupName' => array( 'RGName1', 'RGName2' ), 'Path' => '/opt/sequoiadb/backup', 'Name' => 'backupName' )
    *                                                    @code
    *                                                    GroupName   : Assign the backups of specifed replica groups to be list
    *                                                    Path        : Assign the backups in specifed path to be list, if not assign, use the backup path asigned in the configuration file
    *                                                    Name        : Assign the backups with specifed name to be list
    *                                                    @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> removeBackup( array( 'Name' => 'myBackup_1' ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to remove backup, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function removeBackup( array|string $options ){}

   /**
    * List the tasks.
    *
    * @param $condition	an array or the string argument. The matching rule, return all the record if null.
    *
    * @param $selector	an array or the string argument. The selective rule, return the whole record if null.
    *
    * @param $orderBy	an array or the string argument. The The ordered rule, never sort if null.
    *
    * @param $hint	an array or the string argument. The hint, automatically match the optimal hint if null.
    *
    * @return Returns a new SequoiaCursor object.
    *
    * @retval SequoiaCursor Object
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $cursor = $db -> listTask() ;
    * if( empty( $cursor ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call listTask, error code: ".$err['errno'] ;
    *    return ;
    * }
    * while( $record = $cursor -> next() ) {
    *    var_dump( $record ) ;
    * }
    * @endcode
   */
   public function listTask( array|string $condition = null, array|string $selector = null, array|string $orderBy = null, array|string $hint = null ){}

   /**
    * Remove the backups.
    *
    * @param $taskID an array or an integer argument. The array of task id or the integer of task id ;
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example: array type
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> waitTask( array( 1, 2, new SequoiaInt64( '3' ) ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call waitTask, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: integer type
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> waitTask( 1 ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call waitTask, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: SequoiaInt64 type
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> waitTask( new SequoiaInt64( '1' ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call waitTask, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function waitTask( array|integer|SequoiaInt64 $taskID ){}

   /**
    * Cancel the specified task.
    *
    * @param $taskID an integer argument. The task id.
    *
    * @param $isAsync a boolean argument. The operation "cancel task" is async or not, "true" for async, "false" for sync. Default sync.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> cancelTask( 1, false ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call cancelTask, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function cancelTask( integer|SequoiaInt64 $taskID, boolean $isAsync = true ){}

   /**
    * Set the attributes of the session.
    *
    * @param $options an array or the string argument. The configuration options for session.The options are as below:
    *                                                  @code
    *        PreferedInstance : Preferred instance for read request in the current session. Could be single value in 'M', 'm', 'S', 's', 'A', 'a', 1-255, or BSON Array to include multiple values.
    *                           "M", "m": read and write instance( master instance ). If multiple numeric instances are given with "M", matched master instance will be chosen in higher priority. If multiple numeric instances are given with "M" or "m", master instance will be chosen if no numeric instance is matched.
    *                           "S", "s": read only instance( slave instance ). If multiple numeric instances are given with "S", matched slave instances will be chosen in higher priority. If multiple numeric instances are given with "S" or "s", slave instance will be chosen if no numeric instance is matched.
    *                           'A', 'a': any instance.
    *                           1-255: the instance with specified instance ID.
    *                           If multiple alphabet instances are given, only first one will be used.
    *                           If matched instance is not found, will choose instance by random.
    *
    *                           e.g. array( 'PreferedInstance' => 'm' ) or array( 'PreferedInstance' => array( 1, 7 ) )
    *
    *        PreferedInstanceMode : The mode to choose query instance when multiple preferred instances are found in the current session.
    *                               'random': choose the instance from matched instances by random.
    *                               'ordered': choose the instance from matched instances by the order of "PreferedInstance".
    *
    *                               e.g. array( 'PreferedInstanceMode' => 'random' )
    *
    *        Timeout : The timeout (in ms) for operations in the current session. -1 means no timeout for operations.
    *
    *                  e.g. array( 'Timeout' => 10000 )
    *                                                  @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> setSessionAttr( array( 'PreferedInstance' => 'm' ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call setSessionAttr, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function setSessionAttr( array|string $options ){}

   /**
    * Get the attributes of the session.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   result
    * @retval string  result
    *
    * Example:
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $result = $db -> getSessionAttr() ;
    * var_dump( $result )
    * @endcode
   */
   public function getSessionAttr(){}

   /**
    * Interrupte the session.
    *
    * @param $sessionID an integer or the SequoiaINT64 Object argument. The id of the session which we want to inerrupt.
    *
    * @param $options	an array or the string argument. The location information, such as NodeID, HostName and svcname.
    *                              the options are as below:
    *                              @code
    *                              HostName:  Node's HostName.
                                   svcname:   Node's svcname.
                                   NodeID:    Node ID.
                                   GroupID:   Replica group ID.
                                   GroupName: Replica group name.
    *                              @endcode
    *
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example: integer type
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> forceSession( 1 ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call forceSession, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: SequoiaINT64 type
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> forceSession( new SequoiaINT64( '1' ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call forceSession, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: Force specified node session
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $err = $db -> forceSession( 1, array( "HostName" => "host-01", "svcname" => "11810" ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call forceSession, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function forceSession( integer|SequoiaINT64 $sessionID, array|string $options = null ){}

   /**
    * Analyze collection or index to collect statistics information
    *
    * @param $options an array or the string argument. The control options:
    *        @code
    *        CollectionSpace : (String) Specify the collection space to be analyzed.
    *        Collection      : (String) Specify the collection to be analyzed.
    *        Index           : (String) Specify the index to be analyzed.
    *        Mode            : (Int32) Specify the analyze mode (default is 1):
    *                          Mode 1 will analyze with data samples.
    *                          Mode 2 will analyze with full data.
    *                          Mode 3 will generate default statistics.
    *                          Mode 4 will reload statistics into memory cache.
    *                          Mode 5 will clear statistics from memory cache.
    *        Location Elements : (Only take effect in coordinate nodes) GroupID:INT32, GroupName:String, NodeID:INT32, HostName:String, svcname:String ...
    *        @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * 1. Analyze all collections by default.
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $result = $db -> analyze() ;
    * var_dump( $result ) ;
    * @endcode
    * 2. Analyze collection "foo.bar"
    * @code
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * $result = $db -> analyze( array( 'Collection' => 'foo.bar' ) ) ;
    * var_dump( $result ) ;
    * @endcode
   */
   public function analyze( array|string $options = null ){}

}
