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

/** \file deprecated_function.php
    \brief deprecated function
 */

/**
 * Class for create an object of the db
 */
class SequoiaDB
{
   /**
    * Get the specified list.
    *
    * @see list().
    *
    * @deprecated This function is deprecated, use the function list() instead.
   */
   public function getList( integer $type, array|string $condition = null, array|string $selector = null, array|string $orderBy = null ){}

   /**
    * Get the snapshot.
    *
    * @see snapshot().
    *
    * @deprecated This function is deprecated, use the function snapshot() instead.
   */
   public function getSnapshot( integer $type, array|string $condition = null, array|string $selector = null, array|string $orderBy = null ){}

   /**
    * Get the specified collection space, if is not exist,will auto create.
    *
    * @param $name	the string argument. The collection space name.
    *
    * @param $pageSize an integer argument. When the collection space is created, $pageSize into force.
    *                                      The $pageSize as below: @code
    *                                                              4096
    *                                                              8192
    *                                                              16384
    *                                                              32768
    *                                                              65536
    *                                                              @endcode
    *
    * @return Returns a new SequoiaCS object.
    *
    * @retval SequoiaCS Object
    *
    * @deprecated This function is deprecated, use the function selectCS( string $name, array|string $options = NULL ) instead.
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
   public function selectCS( string $name, integer $pageSize = 4096 ){}

   /**
    * Get the specified list.
    *
    * @see listCS().
    *
    * @deprecated This function is deprecated, use the function listCS() instead.
   */
   public function listCSs(){}

   /**
    * Drop the specified collection space.
    *
    * @see dropCS().
    *
    * @deprecated This function is deprecated, use the function dropCS() instead.
   */
   public function dropCollectionSpace( string $name ){}

   /**
    * List all collection of current database(not include temporary collection of temporary collection space)
    *
    * @see listCL().
    *
    * @deprecated This function is deprecated, use the function listCL() instead.
   */
   public function listCollections(){}

   /**
    * List the domain.
    *
    * @see listDomain().
    *
    * @deprecated This function is deprecated, use the function listDomain() instead.
   */
   public function listDomains(){}

   /**
    * Get the specified replica group.
    *
    * @see getGroup().
    *
    * @deprecated This function is deprecated, use the function getGroup() instead.
   */
   public function selectGroup( string $name ){}
}

/**
 * Class for create an object of the collection space
 */
class SequoiaCS
{
   /**
    * Get the specified collection, if is not exist,will auto create.
    *
    * @see selectCL().
    *
    * @deprecated This function is deprecated, use the function selectCL() instead.
   */
   public function selectCollection( string $name, array|string $options = NULL ){}

   /**
    * Drop the specified collection.
    *
    * @see dropCL().
    *
    * @deprecated This function is deprecated, use the function dropCL() instead.
   */
   public function dropCollection( string $name ){}
}

/**
 * Class for create an object of the collection
 */
class SequoiaCL
{
   /**
    * Get the specified collection name.
    *
    * @see getName().
    *
    * @deprecated This function is deprecated, use the function getName() instead.
   */
   public function getCollectionName(){}

   /**
    * Delete $id index in collection.
    *
    * @see dropIndex().
    *
    * @deprecated This function is deprecated, use the function dropIndex() instead.
   */
   public function deleteIndex(){}
}

/**
 * Class for create an object of the cursor
 */
class SequoiaCursor
{
   /**
    * Drop the specified collection.
    *
    * @see next().
    *
    * @deprecated This function is deprecated, use the function next() instead.
   */
   public function getNext(){}
}

/**
 * SequoiaGroup Class. To get this Class object must be call SequoiaDB::getGroup.
 */
class SequoiaGroup
{
   /**
    * Get the status node number.
    *
    * @deprecated This function is deprecated.
   */
   public function getNodeNum( integer $status ){}
}

/**
 * Class for create an object of the node
 */
class SequoiaNode
{
   /**
    * Get the node name.
    *
    * @see getName().
    *
    * @deprecated This function is deprecated, use the function getName() instead.
   */
   public function getNodeName(){}

   /**
    * Get node status.
    *
    * @deprecated This function is deprecated.
   */
   public function getStatus(){}
}