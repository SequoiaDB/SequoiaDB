#   Copyright (C) 2012-2018 SequoiaDB Ltd.
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
try:
    from . import sdb
except:
    raise Exception("Cannot find extension: sdb")

import bson
from pysequoiadb.cursor import cursor
from pysequoiadb.errcode import SDB_OOM
from pysequoiadb.error import (SDBSystemError, SDBTypeError, raise_if_error)


class domain(object):
    """Domain of SequoiaDB
    """

    def __init__(self, domain_name):
        """constructor of domain

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        self._domain_name = domain_name
        self._domain = None
        try:
            self._domain = sdb.create_domain()
        except SystemError:
            raise SDBSystemError(SDB_OOM, "Failed to alloc cursor")

    def __del__(self):
        """release domain

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        self._domain_name = None
        if self._domain is not None:
            rc = sdb.release_domain(self._domain)
            raise_if_error(rc, "Failed to release domain")
            self._domain = None

    def __repr__(self):
        return "Domain: %s" % self._domain_name

    def __str__(self):
        return "Domain: %s" % self._domain_name

    @property
    def name(self):
        """Get the domain name.

        Return values:
           The domain name.
        """
        return self._domain_name

    def alter(self, options):
        """Alter properties of this domain

        Parameters:
           Name        Type     Info
           options      dict    The options for the domain. The options are as below:
                                Groups: the list of the replica groups' names which the domain is going to contain.
                                        eg: { "Groups": [ "group1", "group2", "group3" ] }
                                        If this argument is not included, the domain will contain all replica groups in the cluster.
                                AutoSplit: If this option is set to be true, while creating collection(ShardingType is "hash") in this domain,
                                           the data of this collection will be split(hash split) into all the groups in this domain automatically.
                                           However, it won't automatically split data into those groups which were add into this domain later.
                                           eg: { "Groups": [ "group1", "group2", "group3" ], "AutoSplit: true" }
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of dict")
        bson_options = bson.BSON.encode(options)

        rc = sdb.domain_alter(self._domain, bson_options)
        raise_if_error(rc, "Failed to alter domain: %s" % self._domain_name)

    def list_collection_spaces(self):
        """List all collection spaces in this domain.

        Return values:
           The cursor object of collection spaces.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        result = cursor()
        try:
            rc = sdb.domain_list_cs(self._domain, result._cursor)
            raise_if_error(rc, "Failed to list collection spaces of %s" % self._domain_name)
        except:
            del result
            raise

        return result

    def list_collections(self):
        """List all collections in this domain.

        Return values:
           The cursor object of collections.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        result = cursor()
        try:
            rc = sdb.domain_list_cl(self._domain, result._cursor)
            raise_if_error(rc, "Failed to list collections of %s" % self._domain_name)
        except:
            del result
            raise

        return result
