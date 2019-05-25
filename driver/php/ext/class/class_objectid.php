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

/** \file class_objectid.php
    \brief object id class
 */

/**
 * Class for create an object of the object id type
 */
class SequoiaID
{
   /**
    * Constructor.
    *
    * @param string $oid a string to uses as id
    *
    * @code
    * $oid = new SequoiaID( '123456789012345678901234' ) ;
    * $arr = array( 'id' => $oid ) ; // json ==> { "id": { "$oid": "123456789012345678901234" } }
    * @endcode
   */
   public function __construct( $oid ){}

   /**
    * PHP Magic Methods, the class as string output.
    *
    * @return object id string
    *
    * @code
    * $oid = new SequoiaID( '123456789012345678901234' ) ;
    * echo $oid ; // output ==> 123456789012345678901234
    * @endcode
    */
   public function __toString(){}
}