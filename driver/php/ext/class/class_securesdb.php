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

/** \file class_securesdb.php
    \brief secure SequoiaDB, use SSL connection
 */

/**
 * secure SequoiaDB, use SSL connection
 */
class SecureSdb: public SequoiaDB
{
   /**
    * SecureSdb class constructor.
    *
    * @param $address	an array or the string argument. The Host Name or IP Address and The Service Name or Port of Database Server.
    *
    * @param $userName	the string argument. The User's Name of the account.
    *
    * @param $password	the string argument. The Password of the account.
    *
    * Example: 1. Connect to the database with ssl.
    * @code
    * $db = new SecureSdb( "192.168.1.10:11810" ) ;
    * $err = $db -> getLastErrorMsg() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database with ssl, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: 2. Using the connect function to connect to the database with ssl.
    * @code
    * $db = new SecureSdb() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database with ssl, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function __construct( array|string $address, string $userName = "", string $password = "" ){}
   
   /**
    * Connect to database with ssl.
    *
    * @param $address	an array or the string argument. The Host Name or IP Address and The Service Name or Port of Database Server.
    *
    * @param $userName	the string argument. The User's Name of the account.
    *
    * @param $password	the string argument. The Password of the account.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $db = new SecureSdb() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database with ssl, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function connect( array|string $address = "127.0.0.1:11810", string $userName = "", string $password = "" ){}
} ;