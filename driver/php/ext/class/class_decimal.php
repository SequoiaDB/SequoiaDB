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

/** \file class_decimal.php
    \brief decimal class
 */

/**
 * Class for create an object of the decimal type
 */
class SequoiaDecimal
{
   /**
    * Constructor.
    *
    * @param string|integer|double $decimal decimal number
    *
    * @param integer $precision is the precision of decimal, if not limit precision, then $precision $scale is -1.
    *
    * @param integer $scale is the scale of decimal, if not limit precision, then $precision $scale is -1.
    *
    * @code
    * $decimalObj = new SequoiaDecimal( '1000.123456', 20, 6 ) ;
    * $arr = array( 'money' => $decimalObj ) ; // json ==> { "money": { "$decimal": "1000.123456", "$precision": [ 20, 6 ] } }
    * @endcode
   */
   public function __construct( $decimal, $precision = -1, $scale = -1 ){}

   /**
    * PHP Magic Methods, the class as string output.
    *
    * @return decimal string
    *
    * @code
    * $decimalObj = new SequoiaDecimal( '1000.123456' ) ;
    * echo $decimalObj ; // output ==> 1000.123456
    * @endcode
    */
   public function __toString(){}
}