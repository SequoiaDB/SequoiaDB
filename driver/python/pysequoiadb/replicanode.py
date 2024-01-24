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

"""Module of replica node for python driver of SequoiaDB
"""

try:
    from . import sdb
except:
    raise Exception("Cannot find extension: sdb")

import pysequoiadb
from pysequoiadb.error import (SDBSystemError, raise_if_error)
from pysequoiadb.errcode import SDB_OOM


class replicanode(object):
    """Replica Node of SequoiaDB

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
        """constructor of replica node

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        self._client = client
        try:
            self._node = sdb.create_node()
        except SystemError:
            raise SDBSystemError(SDB_OOM, "Failed to alloc node")

    def __del__(self):
        """release replica node

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if self._node is not None:
            rc = sdb.release_node(self._node)
            raise_if_error(rc, "Failed to release node")
            self._node = None
        self._client = None

    def __repr__(self):
        return "Replica Node: %s" % self.get_nodename()

    def connect(self):
        """Connect to the current node.

        Return values:
           client of current node
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Deprecated:
           This function is deprecated
        """
        return pysequoiadb.client(self.get_hostname(), self.get_servicename())

    def get_status(self):
        """Get status of the current node

        Return values:
           the status of node
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, node_status = sdb.nd_get_status(self._node)
        raise_if_error(rc, "Failed to get node status")
        return node_status

    def get_hostname(self):
        """Get host name of the current node.

        Return values:
           the name of host
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, hostname = sdb.nd_get_hostname(self._node)
        raise_if_error(rc, "Failed to get host name")
        return hostname

    def get_servicename(self):
        """Get service name of the current node.

        Return values:
           the name of service
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, service_name = sdb.nd_get_servicename(self._node)
        raise_if_error(rc, "Failed to get service name")
        return service_name

    def get_nodename(self):
        """Get node name of the current node.

        Return values:
           the name of node
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, node_name = sdb.nd_get_nodename(self._node)
        raise_if_error(rc, "Failed to get node name")
        return node_name

    def stop(self):
        """Stop the node.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.nd_stop(self._node)
        raise_if_error(rc, "Failed to stop node")

    def start(self):
        """Start the node.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.nd_start(self._node)
        raise_if_error(rc, "Filed to start node")
