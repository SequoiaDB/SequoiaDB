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

/** \file class_group.php
    \brief group class
 */

/**
 * SequoiaGroup Class. To get this Class object must be call SequoiaDB::getGroup.
 */
class SequoiaGroup
{
   /**
    * Judge whether the specified replica group is catalog.
    *
    * @return Returns the result.
    *
    * @retval boolean isCatalog
    *
    * Example:
    * @code
    * $isCata = $groupObj -> isCatalog() ;
    * if( $isCata == true ) {
    *    echo "This is the catalog group." ;
    * } else {
    *    echo "This is not the catalog group" ;
    * }
    * @endcode
   */
   public function isCatalog(){}

   /**
    * Get the specified replica group name.
    *
    * @return Returns the group name.
    *
    * @retval string &lt;group_name&gt;
    *
    * Example:
    * @code
    * $groupName = $groupObj -> getName() ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to get group name, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Group name is: ".$groupName ;
    * @endcode
   */
   public function getName(){}

   /**
    * Start and activate the specified replica group.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $groupObj -> start() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to start group, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function start(){}

   /**
    * Stop the specified replica group.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $groupObj -> stop() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to stop group, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function stop(){}

   /**
    * Re-elected in the replication group.
    *
    * @param $options an array or the string argument. 
    *                                           @code
    *                                           Seconds:  New elections needed within the number of seconds to complete, default 30 seconds.
    *                                           @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example: 60 seconds for new elections.
    * @code
    * $err = $groupObj -> reelect( array( 'Seconds' => 60 ) ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call reelect, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function reelect( array|string $options = NULL ){}
   
   /**
    * Get the status node number.
    *
    * @param $status	an integer argument. The node status, can not be null
    *
    * @return Returns the number of node.
    *
    * @retval integer Nodes number
    *
    * Example:
    * @code
    * $NodeNum = $groupObj -> getNodeNum() ;
    * if( $NodeNum < 0 ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call getNodeNum, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Node number: ".$NodeNum ;
    * @endcode
   */
   public function getNodeNum( integer $status ){}

   /**
    * Get the group detail.
    *
    * @return Returns the group detail, default return array.
    *
    * @retval array   detail
    * @retval string  detail
    *
    * Example:
    * @code
    * $detail = $groupObj -> getDetail() ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to call getDetail, error code: ".$err['errno'] ;
    *    return ;
    * }
    * var_dump( $detail ) ;
    * @endcode
   */
   public function getDetail(){}

   /**
    * Get the master node.
    *
    * @return Returns a new SequoiaNode object.
    *
    * @retval SequoiaNode Object
    *
    * Example:
    * @code
    * $nodeObj = $groupObj -> getMaster() ;
    * if( empty( $nodeObj ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to get the master node, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function getMaster(){}

   /**
    * Get the slave node.
    *
    * @param $positionsArray an array argument. The array of node's position, the array elements can be 1-7.
    *
    * @return Returns a new SequoiaNode object.
    *
    * @retval SequoiaNode Object
    *
    * Example: Get one of slave node
    * @code
    * $nodeObj = $groupObj -> getSlave() ;
    * if( empty( $nodeObj ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to get the slave node, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
    *
    * Example: Get a specified slave node
    * @code
    * $nodeObj = $groupObj -> getSlave( array( 1, 2, 3 ) ) ;
    * if( empty( $nodeObj ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to get the slave node, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function getSlave( $positionsArray = void ){}

   /**
    * Get the node.
    *
    * @param $name	the string argument. The name of the node.
    *
    * @return Returns a new SequoiaNode object.
    *
    * @retval SequoiaNode Object
    *
    * Example:
    * @code
    * $nodeObj = $groupObj -> getNode( 'host1:11910' ) ;
    * if( empty( $nodeObj ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to get the node, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function getNode( string $name ){}

   /**
    * Create node.
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
    * $err = $groupObj -> createNode( 'host1', '11900', '/opt/sequoiadb/database/catalog/11900' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to create node, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function createNode( string $hostName, string $serviceName, string $databasePath, array|string $configure = NULL ){}

   /**
    * Remove node in a given replica group.
    *
    * @param $hostName	the string argument. The hostname for the catalog replica group.
    *
    * @param $serviceName	the string argument. The servicename for the catalog replica group.
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
    * $err = $groupObj -> removeNode( 'host1', '11900' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to remove node, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function removeNode( string $hostName, string $serviceName, array|string $configure = NULL ){}

   /**
    * Attach a node to the group.
    *
    * @param $hostName	the string argument. The hostname for the catalog replica group.
    *
    * @param $serviceName	the string argument. The servicename for the catalog replica group.
    *
    * @param $options	an array or the string argument. The options of attach.
    *                                                  @code
    *                                                  KeepData : Whether the node data is preserved, default false.
    *                                                  @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $groupObj -> attachNode( 'host1', '11900' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to attach node, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function attachNode( string $hostName, string $serviceName, array|string $options = NULL ){}

   /**
    * Detach a node from the group.
    *
    * @param $hostName	the string argument. The hostname for the catalog replica group.
    *
    * @param $serviceName	the string argument. The servicename for the catalog replica group.
    *
    * @param $options	an array or the string argument. The options of detach.
    *                                                  @code
    *                                                  KeepData : Whether the node data is preserved, default false.
    *                                                  @endcode
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $groupObj -> detachNode( 'host1', '11900' ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to detach node, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function detachNode( string $hostName, string $serviceName, array|string $options = NULL ){}
}