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

/** \file class_node.php
    \brief node class
 */

/**
 * SequoiaNode Class. To get this Class object must be call SequoiaGroup::getMaster or SequoiaGroup::getNode or SequoiaGroup::getSlave.
 */
class SequoiaNode
{
   /**
    * Get the node name.
    *
    * @return Returns the node name.
    *
    * @retval string &lt;node_name&gt;
    *
    * Example:
    * @code
    * $nodeName = $nodeObj -> getName() ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to get node name, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Node name is: ".$nodeName ;
    * @endcode
   */
   public function getName(){}
   
   /**
    * Get the node host name.
    *
    * @return Returns the node host name.
    *
    * @retval string &lt;node_hostname&gt;
    *
    * Example:
    * @code
    * $hostName = $nodeObj -> getName() ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to get host name, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Host name is: ".$hostName ;
    * @endcode
   */
   public function getHostName(){}
   
   /**
    * Get the node service name.
    *
    * @return Returns the node service name.
    *
    * @retval string &lt;node_servicename&gt;
    *
    * Example:
    * @code
    * $servicename = $nodeObj -> getServiceName() ;
    * $err = $db -> getError() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to get service name, error code: ".$err['errno'] ;
    *    return ;
    * }
    * echo "Service name is: ".$servicename ;
    * @endcode
   */
   public function getServiceName(){}

   /**
    * Connect the node.
    *
    * @return Returns a new SequoiaDB object.
    *
    * @retval SequoiaDB Object
    *
    * Example:
    * @code
    * //connect to the coord
    * $db = new SequoiaDB() ;
    * $err = $db -> connect( "192.168.1.10:11810" ) ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to connect database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * //get group object
    * $groupObj = $db -> getGroup( 'myGroup' ) ;
    * if( empty( $groupObj ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to call getGroup, error code: ".$err['errno'] ;
    *    return ;
    * }
    * //get node object
    * $nodeObj = $groupObj -> getNode( 'host1:11910' ) ;
    * if( empty( $nodeObj ) ) {
    *    $err = $db -> getError() ;
    *    echo "Failed to get the node, error code: ".$err['errno'] ;
    *    return ;
    * }
    * //connect to the node
    * $nodeDB = $nodeObj -> connect() ;
    * if( empty( $nodeDB ) ) {
    *    echo "Failed to connect node database, error code: ".$err['errno'] ;
    *    return ;
    * }
    * //get node contexts
    * $cursor = $nodeDB -> list( SDB_LIST_CONTEXTS ) ;
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
   public function connect(){}
   
   /**
    * Start the node.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $nodeObj -> start() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to start node, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function start(){}

   /**
    * Stop the node.
    *
    * @return Returns the result, default return array.
    *
    * @retval array   array( 'errno' => 0 )
    * @retval string  { "errno": 0 }
    *
    * Example:
    * @code
    * $err = $nodeObj -> stop() ;
    * if( $err['errno'] != 0 ) {
    *    echo "Failed to stop node, error code: ".$err['errno'] ;
    *    return ;
    * }
    * @endcode
   */
   public function stop(){}
}