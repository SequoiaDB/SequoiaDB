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

/** \file class_regex.php
    \brief regex class
 */

/**
 * Class for create an object of the regex type
 */
class SequoiaRegex
{
   /**
    * Constructor.
    *
    * @param string $regex is regex string
    * @param string $options is regex options string
    *
    * @code
    * $regexObj = new SequoiaRegex( 'a', 'i' ) ;
    * $arr = array( 'regex' => $regexObj ) ; // json ==> { "regex": { "$regex": "a", "$options": "i" } }
    * @endcode
   */
   public function __construct( $regex, $options ){}

   /**
    * PHP Magic Methods, the class as string output.
    *
    * @return regex string
    *
    * @code
    * $regexObj = new SequoiaRegex( 'a', 'i' ) ;
    * echo $regexObj ; // output ==> /a/i
    * @endcode
    */
   public function __toString(){}
}