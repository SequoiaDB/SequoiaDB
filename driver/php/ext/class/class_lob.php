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

/** \file class_lob.php
    \brief lob class
 */

/**
 * SequoiaLob Class. To get this Class object must be call SequoiaCL::openLob.
 */
class SequoiaLob
{
   /** Set position equal to offset bytes. */
   define( "SDB_LOB_SET", 0 ) ;
   /** Set position to current location plus offset. */
   define( "SDB_LOB_CUR", 1 ) ;
   /** Set position to end of lob plus offset. */
   define( "SDB_LOB_END", 2 ) ;
   
   /**
    * Close lob.
    *
    * @return Returns the result, default return array.
    *
    * @retval array	 array( 'errno' => 0 )
    * @retval string	{ "errno": 0 }
    *
    * Example:
    * @code
    * $err = $lobObj -> close() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to close lob, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function close(){}

   /**
    * Get the lob size.
    *
    * @return Returns the number of lob size.
    *
    * @retval integer|SequoiaINT64 lob size
    *
    * Example:
    * @code
    * $lobSize = $lobObj -> getSize() ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to get lob size, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Lob size is: ".$lobSize ;
    * @endcode
   */
   public function getSize(){}

   /**
    * Get the lob create time (millisecond).
    *
    * @return Returns the create time.
    *
    * @retval integer|SequoiaINT64 millisecond
    *
    * Example:
    * @code
    * $times = $lobObj -> getCreateTime() ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to get lob create time, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Lob create time is: ".$times ;
    * @endcode
   */
   public function getCreateTime(){}

   /**
    * Get the lob last modification time (millisecond).
    *
    * @return Returns the create time.
    *
    * @retval integer|SequoiaINT64 millisecond
    *
    * Example:
    * @code
    * $times = $lobObj -> getModificationTime() ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to get lob modification time, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Lob modification time is: ".$times ;
    * @endcode
   */
   public function getModificationTime(){}

   /**
    * Write lob.
    *
    * @param $buffer	the string argument. The buffer of write.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $lobObj -> write( 'hello' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to write lob, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function write( string $buffer ){}

   /**
    * Read lob.
    *
    * @param $length	an integer or the SequoiaINT64 object argument. The length want to read.
    *
    * @return Returns the string of read.
    *
    * @retval string read buffer
    *
    * Example:
    * @code
    * $buffer = $lobObj -> read( 5 ) ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to read lob, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function read( integer|SequoiaINT64 $length ){}
   
   /**
    * Seek the place to read or write.
    *
    * @param $offset	an integer or the SequoiaINT64 object argument. Lob offset position.
    *
    * @param $whence	an integer argument. Where to start position.
    *                                     @code
    *                                     SDB_LOB_SET : Set position equal to offset bytes.
    *                                     SDB_LOB_CUR : Set position to current location plus offset.
    *                                     SDB_LOB_END : Set position to end of lob plus offset.
    *                                     @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $lobObj -> seek( 10, SDB_LOB_CUR ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call seek, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function seek( integer|SequoiaINT64 $offset, integer $whence = SDB_LOB_SET ){}

   /**
    * Lock LOB section for write mode.
    *
    * @param $offset	an integer or the SequoiaINT64 object argument. The lock start position.
    *
    * @param $length	an integer or the SequoiaINT64 object argument. The lock length, -1 means lock to the end of lob.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $lobObj -> lock( 10, 100 ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call lock, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function lock( integer|SequoiaINT64 offset, integer|SequoiaINT64 length ){}

   /**
    * Lock LOB section for write mode and seek to the offset position.
    *
    * @param $offset	an integer or the SequoiaINT64 object argument. The lock start position.
    *
    * @param $length	an integer or the SequoiaINT64 object argument. The lock length, -1 means lock to the end of lob.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $lobObj -> lockAndSeek( 10, 100 ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call lockAndSeek, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function lockAndSeek( integer|SequoiaINT64 offset, integer|SequoiaINT64 length ){}
}