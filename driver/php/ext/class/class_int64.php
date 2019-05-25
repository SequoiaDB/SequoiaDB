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

/** \file class_int64.php
    \brief int64 class
 */

/**
 * Class for create an object of the int64 type
 */
class SequoiaINT64
{
   /**
    * Constructor.
    * @param string $number is a string of 64-bit integer
    *
    * @code
    * $int64Obj = new SequoiaINT64( '10000000000' ) ;
    * $arr = array( 'number' => $int64Obj ) ; // json ==> { "number": 10000000000 }
    * @endcode
   */
   public function __construct( $number ){}

   /**
    * PHP Magic Methods, the class as string output.
    * @return numeric string
    */
   public function __toString(){}
}