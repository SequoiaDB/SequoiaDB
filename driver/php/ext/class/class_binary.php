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

/** \file class_binary.php
    \brief binary class
 */

/**
 * Class for create an object of the binary type
 */
class SequoiaBinary
{
   /**
    * Constructor.
    *
    * @param string $binary is Base64 encoded string
    * @param integer|string $type is a binary type, range is 0-255
    *
    * @code
    * $binObj = new SequoiaBinary( 'aGVsbG8=', '1' ) ;
    * $arr = array( 'data' => $binObj ) ; // json ==> { "data": { "$binary": "aGVsbG8=", "$type": "1" } }
    * @endcode
   */
   public function __construct( $binary, $type ){}

   /**
    * PHP Magic Methods, the class as string output.
    *
    * @return Base64 encoded string
    *
    * @code
    * $binObj = new SequoiaBinary( 'aGVsbG8=', '1' ) ;
    * echo $binObj ; // output ==> (1)aGVsbG8=
    * @endcode
    */
   public function __toString(){}
}