#   Copyright (C) 2012-2014 SequoiaDB Ltd.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

"""Module of replicagroup for python driver of SequoiaDB
"""

try:
    from . import sdb
except:
    raise Exception("Cannot find extension: sdb")

import bson
from bson.py3compat import (str_type)
from pysequoiadb.errcode import SDB_OOM
from pysequoiadb.error import (SDBBaseError, SDBSystemError, SDBTypeError, raise_if_error)
from pysequoiadb.replicanode import replicanode

NODE_STATUS_ALL = 0
NODE_STATUS_ACTIVE = 1
NODE_STATUS_INACTIVE = 2
NODE_STATUS_UNKNOWN = 3
TRUE = 1

SDB_COORD_GROUP_NAME = 'SYSCoord'
SDB_CATALOG_GROUP_NAME = 'SYSCatalogGroup'


class replicagroup(object):
    """Replica group of SequoiaDB

    All operation need deal with the error code returned first, if it has.
    Every error code is not SDB_OK(or 0), it means something error has appeared,
    and user should deal with it according the meaning of error code printed.

    @version: execute to get version
              >>> import pysequoiadb
              >>> print pysequoiadb.get_version()

    @notice : The dict of built-in Python is hashed and non-ordered. so the
              element in dict may not the order we make it. we make a dict and
              print it like this:
              ...
              >>> a = {"avg_age":24, "major":"computer science"}
              >>> a
              >>> {'major': 'computer science', 'avg_age': 24}
              ...
              the elements order it is not we make it!!
              therefore, we use bson.SON to make the order-sensitive dict if the
              order is important such as operations in "$sort", "$group",
              "split_by_condition", "aggregate","create_collection"...
              In every scene which the order is important, please make it using
              bson.SON and list. It is a subclass of built-in dict
              and order-sensitive
    """

    def __init__(self, client):
        """constructor of replica group

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """

        self._client = client
        try:
            self._group = sdb.create_group()
        except SystemError:
            raise SDBSystemError(SDB_OOM, "Failed to alloc replica group")

    def __del__(self):
        """release replica group object

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if self._group is not None:
            rc = sdb.release_group(self._group)
            raise_if_error(rc, "Failed to release replica group")
            self._group = None

        self._client = None

    def __repr__(self):
        detail = self.get_detail()
        name = detail['GroupName']
        group_id = detail['GroupID']
        return "Replica Group: %s, ID:%d" % (name, group_id)

    def get_nodenum(self, node_status):
        """Get the count of node with given status in current replica group.

        Parameters:
           Name         Type     Info:
           node_status  int      The specified status, see Info as below.
        Return values:
           the count of node
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           flags : 0 or 1.
               0 : count of all node
               1 : count of actived node
               2 : count of inactived node
               3 : count of unknown node
        """
        if not isinstance(node_status, int):
            raise SDBTypeError("node status be an instance of int")

        if node_status not in (NODE_STATUS_ALL, NODE_STATUS_ACTIVE, NODE_STATUS_INACTIVE, NODE_STATUS_UNKNOWN):
            raise SDBTypeError("node status invalid")

        rc, node_num = sdb.gp_get_nodenum(self._group, node_status)
        raise_if_error(rc, "Failed to get count of node")
        return node_num

    def get_detail(self):
        """Get the detail of the replica group.

        Return values:
           a dict object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, bson_string = sdb.gp_get_detail(self._group)
        raise_if_error(rc, "Failed to get detail")
        detail, size = bson._bson_to_dict(bson_string, dict, False,
                                          bson.OLD_UUID_SUBTYPE, True)
        return detail

    def get_master(self):
        """Get the master node of the current replica group.

        Return values:
           a replica node object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        node = replicanode(self._client)
        try:
            rc = sdb.gp_get_master(self._group, node._node)
            raise_if_error(rc, "Failed to get master")
        except SDBBaseError:
            del node
            raise

        return node

    def get_slave(self, *positions):
        """Get one of slave node of the current replica group, if no slave exists
           then get master.

        Parameters:
           Name         Type                  Info:
           positions    int           The positions of nodes.
        Return values:
           a replicanode object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        for i in range(len(positions)):
            if not isinstance(positions[i], int):
                raise SDBTypeError("elements of positions should be instance of int")

        node = replicanode(self._client)
        
        try:
            rc = sdb.gp_get_slave(self._group, node._node, positions)
            raise_if_error(rc, "Failed to get slave")
        except SDBBaseError:
            del node
            raise

        return node

    def get_nodebyendpoint(self, hostname, servicename):
        """Get specified node from current replica group.

        Parameters:
           Name         Type     Info:
           hostname     str      The host name of the node.
           servicename  str      The service name of the node.
        Return values:
           a replicanode object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(hostname, str_type):
            raise SDBTypeError("hostname must be an instance of str_type")
        if not isinstance(servicename, str_type):
            raise SDBTypeError("servicename must be an instance of str_type")

        node = replicanode(self._client)
        try:
            rc = sdb.gp_get_nodebyendpoint(self._group, node._node,
                                           hostname, servicename)
            raise_if_error(rc, "Failed to get node")
        except SDBBaseError:
            del node
            raise

        return node

    def get_nodebyname(self, node_name):
        """Get specified node from current replica group.

        Parameters:
           Name         Type     Info:
           node_name    str      The host name of the node.
        Return values:
           a replicanode object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(node_name, str_type):
            raise SDBTypeError("node_name must be an instance of str_type")

        node = replicanode(self._client)
        try:
            rc = sdb.gp_get_nodebyname(self._group, node._node, node_name)
            raise_if_error(rc, "Failed to get node")
        except SDBBaseError:
            del node
            raise

        return node

    def create_node(self, hostname, servicename, dbpath, config=None):
        """Create node in a given replica group.

        Parameters:
           Name         Type     Info:
           hostname     str      The host name for the node.
           servicename  str      The servicename for the node.
           dbpath       str      The database path for the node.
           config       dict     The configurations for the node.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(hostname, str_type):
            raise SDBTypeError("host must be an instance of str_type")
        if not isinstance(servicename, str_type):
            raise SDBTypeError("service name must be an instance of str_type")
        if not isinstance(dbpath, str_type):
            raise SDBTypeError("path must be an instance of str_type")
        if config is not None and not isinstance(config, dict):
            raise SDBTypeError("config must be an instance of dict")

        if config is None:
            config = {}
        bson_options = bson.BSON.encode(config)

        rc = sdb.gp_create_node(self._group, hostname, servicename,
                                dbpath, bson_options)
        raise_if_error(rc, "Failed to create node")

    def remove_node(self, hostname, servicename, config=None):
        """Remove node in a given replica group.

        Parameters:
           Name         Type     Info:
           hostname     str      The host name for the node.
           servicename  str      The servicename for the node.
           config       dict     The configurations for the node.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(hostname, str_type):
            raise SDBTypeError("host must be an instance of str_type")
        if not isinstance(servicename, str_type):
            raise SDBTypeError("service name must be an instance of str_type")
        if config is not None and not isinstance(config, dict):
            raise SDBTypeError("config must be an instance of dict")

        if config is not None:
            bson_config = bson.BSON.encode(config)
            rc = sdb.gp_remove_node(self._group, hostname,
                                    servicename, bson_config)
        else:
            rc = sdb.gp_remove_node(self._group, hostname, servicename)
        raise_if_error(rc, "Failed to remove node")

    def start(self):
        """Start up current replica group.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.gp_start(self._group)
        raise_if_error(rc, "Failed to start")

    def stop(self):
        """Stop current replica group.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.gp_stop(self._group)
        raise_if_error(rc, "Failed to stop")

    def is_catalog(self):
        """Test whether current replica group is catalog replica group.

        Return values:
           bool
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        iscatalog = False
        rc, is_cata = sdb.gp_is_catalog(self._group)
        raise_if_error(rc, "Failed to check if is catalog")
        if TRUE == is_cata:
            iscatalog = True
        return iscatalog

    def attach_node(self, hostname, servicename, config):
        """Attach node in a given replica group.

        Parameters:
           Name         Type     Info:
           hostname     str      The host name for the node.
           servicename  str      The servicename for the node.
           config       dict     The configurations for the node. Can not be null or empty. Can be the follow options: 
                                 KeepData: Whether to keep the original data of the new node. This option has no default value. User should specify its value explicitly. 
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(hostname, str_type):
            raise SDBTypeError("host must be an instance of str_type")
        if not isinstance(servicename, str_type):
            raise SDBTypeError("service name must be an instance of str_type")
        if not isinstance(config, dict):
            raise SDBTypeError("config must be an instance of dict")

        bson_options = bson.BSON.encode(config)

        rc = sdb.gp_attach_node(self._group,
                                hostname, servicename, bson_options)
        raise_if_error(rc, "Failed to attach node")

    def detach_node(self, hostname, servicename, config):
        """Detach node in a given replica group.

        Parameters:
           Name         Type     Info:
           hostname     str      The host name for the node.
           servicename  str      The servicename for the node.
           config       dict     The configurations for the node. Can not be null or empty. Can be the follow options:
                                 KeepData: Whether to keep the original data of the detached node. This option has no default value. User should specify its value explicitly. 
                                 Enforced: Whether to detach the node forcibly, default to be false. 
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(hostname, str_type):
            raise SDBTypeError("host must be an instance of str_type")
        if not isinstance(servicename, str_type):
            raise SDBTypeError("service name must be an instance of str_type")
        if not isinstance(config, dict):
            raise SDBTypeError("config must be an instance of dict")

        bson_options = bson.BSON.encode(config)

        rc = sdb.gp_detach_node(self._group,
                                hostname, servicename, bson_options)
        raise_if_error(rc, "Failed to detach node")

    def reelect(self, options=None):
        """Reelect in current group.

        Parameters:
           Name         Type     Info:
           options      dict     The options of reelection. Please visit this url:
                                         "http://doc.sequoiadb.com/cn/sequoiadb-cat_id-1432190873-edition_id-@SDB_SYMBOL_VERSION"
                                         for more detail.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if options is not None and not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of dict")
        if options is None:
            options = {}
        bson_options = bson.BSON.encode(options)

        rc = sdb.gp_reelect(self._group, bson_options)
        raise_if_error(rc, "Failed to reelect")
