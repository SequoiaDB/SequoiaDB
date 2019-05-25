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

"""Module of client for python driver of SequoiaDB

"""
import random
import socket

try:
    from . import sdb
except:
    raise Exception("Cannot find extension: sdb")

import bson
from bson.py3compat import (str_type, long_type)
from pysequoiadb.collectionspace import collectionspace
from pysequoiadb.collection import collection
from pysequoiadb.cursor import cursor
from pysequoiadb.domain import domain
from pysequoiadb.replicagroup import (replicagroup, SDB_COORD_GROUP_NAME, SDB_CATALOG_GROUP_NAME)
from pysequoiadb.error import (SDBBaseError, SDBSystemError, SDBTypeError, SDBError, raise_if_error)
from pysequoiadb.errcode import *

SDB_LIST_CONTEXTS = 0
SDB_LIST_CONTEXTS_CURRENT = 1
SDB_LIST_SESSIONS = 2
SDB_LIST_SESSIONS_CURRENT = 3
SDB_LIST_COLLECTIONS = 4
SDB_LIST_COLLECTIONSPACES = 5
SDB_LIST_STORAGEUNITS = 6
SDB_LIST_GROUPS = 7
SDB_LIST_STOREPROCEDURES = 8
SDB_LIST_DOMAINS = 9
SDB_LIST_TASKS = 10
SDB_LIST_TRANSACTIONS = 11
SDB_LIST_TRANSACTIONS_CURRENT = 12
SDB_LIST_CL_IN_DOMAIN = 129
SDB_LIST_CS_IN_DOMAIN = 130


class client(object):
    """SequoiaDB Client Driver

    The client support interfaces to connect to SequoiaDB.
    In order to connect to SequoiaDB, you need use the class first.
    And you should make sure the instance of it released when you don't use it
    any more.

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
    HOST = "localhost"
    SERVICE = "11810"
    USER = ""
    PSW = ""

    def __init__(self, host=None, service=None, user=None, psw=None, ssl=False):
        """initialize when product a object.

           it will try to connect to SequoiaDB using host and port given,
           localhost and 11810 are the default value of host and port,
           user and password are "".

        Parameters:
           Name       Type      Info:
           host       str       The hostname or IP address of dbserver,
                                      if None, "localhost" will be insteaded
           service    str/int   The service name or port number of dbserver,
                                      if None, "11810" will be insteaded
           user       str       The user name to access to database,
                                      if None, "" will be insteaded
           psw        str       The user password to access to database,
                                      if None, "" will be insteaded
           ssl        bool      decide to use ssl or not, default is False.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        self.__connected = False
        if host is None:
            self.__host = self.HOST
        elif isinstance(host, str_type):
            self.__host = host
        else:
            raise SDBTypeError("host must be an instance of str_type")

        if service is None:
            self.__service = self.SERVICE
        elif isinstance(service, int):
            self.__service = str(service)
        elif isinstance(service, str_type):
            self.__service = service
        else:
            raise SDBTypeError("service name must be an instance of int or str_type")

        if user is None:
            _user = self.USER
        elif isinstance(user, str_type):
            _user = user
        else:
            raise SDBTypeError("user name must be an instance of str_type")

        if psw is None:
            _psw = self.PSW
        elif isinstance(psw, str_type):
            _psw = psw
        else:
            raise SDBTypeError("password must be an instance of str_type")

        if isinstance(ssl, bool):
            self.__ssl = ssl
        else:
            raise SDBTypeError("ssl must be an instance of bool")

        try:
            self._client = sdb.sdb_create_client(self.__ssl)
        except SystemError:
            raise SDBSystemError(SDB_OOM, "Failed to alloc client")

        # try to connect with default user and password
        self.connect(self.__host, self.__service, user=_user, password=_psw)

    def __del__(self):
        """release resource when del called.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        self.__host = self.HOST
        self.__service = self.SERVICE
        if self._client is not None:
            rc = sdb.sdb_release_client(self._client)
            raise_if_error(rc, "Failed to release client")
            self._client = None

    def __repr__(self):
        if self.__connected:
            return "Client, connect to: %s:%s" % (self.__host, self.__service)

    def __getitem__(self, name):
        """support [] to access to collection space.

           eg.
           cc = client()
           cs = cc['test'] # access to collection space named 'test'.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        return self.__getattr__(name)

    def __getattr__(self, name):
        """support client.cs to access to collection space.

           eg.
           cc = client()
           cs = cc.test # access to collection space named 'test'

           and we should pass '__members__' and '__methods__',
           becasue dir(cc) will invoke __getattr__("__members__") and
           __getattr__("__methods__").

           if success, a collection object will be returned, or None.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if '__members__' == name or '__methods__' == name:
            pass
        else:
            cs = collectionspace()
            try:
                rc = sdb.sdb_get_collection_space(self._client, name, cs._cs)
                raise_if_error(rc, "Failed to get collection space: %s" % name)
            except SDBBaseError:
                del cs
                raise

            return cs

    @staticmethod
    def __get_local_ip():
        local_ip = []
        import socket
        host = socket.gethostname()
        ips = socket.gethostbyname_ex(host)
        for ip in ips:
            if ip == host:
                pass
            if isinstance(ip, list):
                for one in ip:
                    if one.startswith('127.'):
                        pass
                    else:
                        local_ip.append(one)

        return local_ip

    def connect_to_hosts(self, hosts, **kwargs):
        """try to connect a host in specified hosts

        Parameters:
           Name        Type  Info:
           hosts       list  The list contains hosts.
                                   eg.
                                   [ {'host':'localhost',     'service':'11810'},
                                     {'host':'192.168.10.30', 'service':'11810'},
                                     {'host':'192.168.20.63', 'service':11810}, ]
           **kwargs          Useful options are below:
           -  user     str   The user name to access to database.
           -  password str   The user password to access to database.
           -  policy   str   The policy of select hosts. it must be string
                                of 'random' or 'local_first' or 'one_by_one', default is 'random'.
                                'local_first' will choose local host firstly,
                                then use 'random' if no local host.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(hosts, list):
            raise SDBTypeError("hosts must be an instance of list")
        if "policy" in kwargs:
            policy = kwargs.get("policy")
        else:
            policy = "random"
        if not isinstance(policy, str):
            raise SDBTypeError("policy must be an instance of str_type")

        if len(hosts) == 0:
            raise SDBTypeError("hosts must hava at least 1 item")

        local = socket.gethostname()
        local_ip = self.__get_local_ip()
        if "user" in kwargs:
            if not isinstance(kwargs.get("user"), str):
                raise SDBTypeError("user name in kwargs must be \
                            an instance of str_type")
            _user = kwargs.get("user")
        else:
            _user = self.USER

        if "password" in kwargs:
            if not isinstance(kwargs.get("password"), str):
                raise SDBTypeError("password in kwargs must be \
                            an instance of str_type")
            _psw = kwargs.get("password")
        else:
            _psw = self.PSW

        # connect to localhost first
        if "local_first" == policy:
            for ip in hosts:
                if ("localhost" in ip.values() or
                            local in ip.values() or
                            ip['host'] in local_ip):

                    host = ip['host']
                    svc = ip['service']
                    if isinstance(host, str_type):
                        self.__host = host
                    else:
                        raise SDBTypeError("policy must be an instance of str_type")

                    if isinstance(svc, int):
                        self.__service = str(svc)
                    elif isinstance(svc, str_type):
                        self.__service = svc
                    else:
                        raise SDBTypeError("policy must be an instance of int or str_type")

                    try:
                        self.connect(self.__host, self.__service,
                                     user=_user, password=_psw)
                    except SDBBaseError:
                        continue

                    return

        # without local host in hosts, check policy
        size = len(hosts)
        if "random" == policy or "local_first" == policy:
            position = random.randint(0, size - 1)
        elif "one_by_one" == policy:
            position = 0
        else:
            raise SDBTypeError("policy must be 'random' or 'one_by_one'.")

        # try to connect to host one by one
        for index in range(size):
            ip = hosts[position]
            host = ip['host']
            svc = ip['service']

            if isinstance(host, str_type):
                self.__host = host
            else:
                raise SDBTypeError("policy must be an instance of str_type")

            if isinstance(svc, int):
                self.__service = str(svc)
            elif isinstance(svc, str_type):
                self.__service = svc
            else:
                raise SDBTypeError("policy must be an instance of int or str")

            try:
                self.connect(self.__host, self.__service,
                             user=_user, password=_psw)
            except SDBBaseError:
                position += 1
                if position >= size:
                    position %= size
                continue
            # with no error
            return

        # raise a exception for failed to connect to any host
        raise SDBError(SDB_NET_CANNOT_CONNECT, "Failed to connect all specified hosts")

    def connect(self, host, service, **kwargs):
        """connect to specified database

        Parameters:
           Name        Type     Info:
           host        str      The host name or IP address of database server.
           service     int/str  The service name of database server.
           **kwargs             Useful options are below:
           -  user     str      The user name to access to database.
           -  password str      The user password to access to database.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if isinstance(host, str_type):
            self.__host = host
        else:
            raise SDBTypeError("host must be an instance of str_type")

        if isinstance(service, int):
            self.__service = str(service)
        elif isinstance(service, str_type):
            self.__service = service
        else:
            raise SDBTypeError("service name must be an instance of int or str_type")

        if "user" in kwargs:
            user = kwargs.get("user")
        else:
            user = self.USER
        if isinstance(user, str_type):
            _user = user
        else:
            raise SDBTypeError("user name must be an instance of str_type")

        if "password" in kwargs:
            psw = kwargs.get("password")
        else:
            psw = self.PSW
        if isinstance(psw, str_type):
            _psw = psw
        else:
            raise SDBTypeError("password must be an instance of str_type")

        rc = sdb.sdb_connect(self._client, self.__host, self.__service,
                             _user, _psw)
        raise_if_error(rc, "Failed to connect to %s:%s" %
                       (self.__host, self.__service))

        # success to connect
        self.__connected = True

    def disconnect(self):
        """disconnect to current server.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.sdb_disconnect(self._client)
        raise_if_error(rc, "Failed to disconnect")

        # success to disconnect
        self.__host = self.HOST
        self.__service = self.PSW
        self.__connected = False

    def create_user(self, name, psw):
        """Add an user in current database.

        Parameters:
           Name         Type     Info:
           name         str      The name of user to be created.
           psw          str      The password of user to be created.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(name, str_type):
            raise SDBTypeError("user name must be an instance of str_type")

        if not isinstance(psw, str_type):
            raise SDBTypeError("password must be an instance of str_type")

        rc = sdb.sdb_create_user(self._client, name, psw)
        raise_if_error(rc, "Failed to create user: %s" % name)

    def remove_user(self, name, psw):
        """Remove the specified user from current database.

        Parameters:
           Name     Type     Info:
           name     str      The name of user to be removed.
           psw      str      The password of user to be removed.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(name, str_type):
            raise SDBTypeError("user name must be an instance of str_type")

        if not isinstance(psw, str_type):
            raise SDBTypeError("password must be an instance of str_type")

        rc = sdb.sdb_remove_user(self._client, name, psw)
        raise_if_error(rc, "Failed to remove user: %s" % name)

    def get_snapshot(self, snap_type, **kwargs):
        """Get the snapshots of specified type.

        Parameters:
           Name           Type  Info:
           snap_type      int   The type of snapshot, see Info as below
           **kwargs             Useful options are below
           - condition    dict  The matching rule, match all the documents
                                      if not provided.
           - selector     dict  The selective rule, return the whole
                                      document if not provided.
           - order_by     dict  The ordered rule, result set is unordered
                                      if not provided.
        Return values:
           a cursor object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
          snapshot type:
                    0     : Get all contexts' snapshot
                    1     : Get the current context's snapshot
                    2     : Get all sessions' snapshot
                    3     : Get the current session's snapshot
                    4     : Get the collections' snapshot
                    5     : Get the collection spaces' snapshot
                    6     : Get database's snapshot
                    7     : Get system's snapshot
                    8     : Get catalog's snapshot
                    9     : Get transactions' snapshot
                    10    : Get current session's transaction snapshot
                    11    : Get cached access plan snapshot
                    12    : Get node health detection snapshot
        """
        if not isinstance(snap_type, int):
            raise SDBTypeError("snap type must be an instance of int")
        if snap_type < 0 or snap_type > 12:
            raise SDBTypeError("snap_type value is invalid")

        bson_condition = None
        bson_selector = None
        bson_order_by = None

        if "condition" in kwargs:
            if not isinstance(kwargs.get("condition"), dict):
                raise SDBTypeError("condition in kwargs must be an instance of dict")
            bson_condition = bson.BSON.encode(kwargs.get("condition"))
        if "selector" in kwargs:
            if not isinstance(kwargs.get("selector"), dict):
                raise SDBTypeError("selector in kwargs must be an instance of dict")
            bson_selector = bson.BSON.encode(kwargs.get("selector"))
        if "order_by" in kwargs:
            if not isinstance(kwargs.get("order_by"), dict):
                raise SDBTypeError("order_by in kwargs must be an instance of dict")
            bson_order_by = bson.BSON.encode(kwargs.get("order_by"))

        result = cursor()
        try:
            rc = sdb.sdb_get_snapshot(self._client, result._cursor, snap_type,
                                      bson_condition, bson_selector, bson_order_by)
            raise_if_error(rc, "Failed to get snapshot: %d" % snap_type)
        except SDBBaseError:
            del result
            raise

        return result

    def reset_snapshot(self, options=None):
        """Reset the snapshot.

        Parameters:
           Name         Type     Info:
           options      dict     The control options:
                                 Type:
                                    (Int32) Specify the snapshot type to be reset (default is "all"):
                                    "sessions"
                                    "sessions current"
                                    "database"
                                    "health"
                                    "all"
                                 SessionID:
                                    (String) Specify the session ID to be reset.
                                 Others options:
                                    Some of other options are as below:(please visit the official website to search "Location Elements" for more detail.)
                                    GroupID:INT32,
                                    GroupName:String,
                                    NodeID:INT32,
                                    HostName:String,
                                    svcname:String,
                                    ...
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if options is None:
            options = {}
        if not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of dict")
        bson_options = bson.BSON.encode(options)

        rc = sdb.sdb_reset_snapshot(self._client, bson_options)
        raise_if_error(rc, "Failed to reset snapshot")

    def get_list(self, list_type, **kwargs):
        """Get information of the specified type.

        Parameters:
           Name        Type     Info:
           list_type   int      Type of list option, see Info as below.
           **kwargs             Useful options are below
           - condition dict     The matching rule, match all the documents
                                      if None.
           - selector  dict     The selective rule, return the whole
                                      documents if None.
           - order_by  dict     The ordered rule, never sort if None.
        Return values:
           a cursor object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           list type:
                  0          : Get all contexts list
                  1          : Get contexts list for the current session
                  2          : Get all sessions list
                  3          : Get the current session
                  4          : Get all collections list
                  5          : Get all collection spaces' list
                  6          : Get storage units list
                  7          : Get replicaGroup list ( only applicable in sharding env )
                  8          : Get store procedure list
                  9          : Get domains list
                  10         : Get tasks list
                  11         : Get transactions list
                  12         : Get current session's transaction list
                  129        : Get collection space list in domain
                  130        : Get collection list in domain
        """
        if not isinstance(list_type, int):
            raise SDBTypeError("list type must be an instance of int")
        if list_type < 0 or (12 < list_type < 129) or list_type > 130:
            raise SDBTypeError("list type value %d is not defined" %
                               list_type)

        bson_condition = None
        bson_selector = None
        bson_order_by = None

        if "condition" in kwargs:
            bson_condition = bson.BSON.encode(kwargs.get("condition"))
        if "selector" in kwargs:
            bson_selector = bson.BSON.encode(kwargs.get("selector"))
        if "order_by" in kwargs:
            bson_order_by = bson.BSON.encode(kwargs.get("order_by"))

        result = cursor()
        try:
            rc = sdb.sdb_get_list(self._client, result._cursor, list_type,
                                  bson_condition, bson_selector, bson_order_by)
            raise_if_error(rc, "Failed to get list: %d" % list_type)
        except SDBBaseError:
            del result
            raise

        return result

    def get_collection(self, cl_full_name):
        """Get the specified collection.

        Parameters:
           Name         Type     Info:
           cl_full_name str      The full name of collection
        Return values:
           a collection object of query.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(cl_full_name, str_type):
            raise SDBTypeError("full name of collection must be an instance of str_type")
        if '.' not in cl_full_name:
            raise SDBTypeError("Full name must included '.'")

        cl = collection()
        try:
            rc = sdb.sdb_get_collection(self._client, cl_full_name, cl._cl)
            raise_if_error(rc, "Failed to get collection: %s" % cl_full_name)
        except SDBBaseError:
            del cl
            raise

        return cl

    def get_collection_space(self, cs_name):
        """Get the specified collection space.

        Parameters:
           Name         Type     Info:
           cs_name      str      The name of collection space.
        Return values:
           a collection space object of query.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(cs_name, str_type):
            raise SDBTypeError("name of collection space must be an instance of str_type")

        cs = collectionspace()
        try:
            rc = sdb.sdb_get_collection_space(self._client, cs_name, cs._cs)
            raise_if_error(rc, "Failed to get collection space: %s" % cs_name)
        except SDBBaseError:
            del cs
            raise

        return cs

    def create_collection_space(self, cs_name, options=0):
        """Create collection space with specified pagesize.

        Parameters:
           Name          Type     Info:
           cs_name       str      The name of collection space to be created.
           options       int/dict The options to create collection space.
                                  When type is int, means setting PageSize.
            -PageSize    int      The page size of collection space. See Info
                                       as below.
            -Domain      str      The domain of collection space to belongs
            -LobPageSize int      The page size when stored lob, see Info as below
        Return values:
           collection space object created.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           valid page size value:
                             0  :  64k default page size
                          4096  :  4k
                          8192  :  8k
                         16384  :  16k
                         32768  :  32k
                         65536  :  64k
           valid LOB page size value:
                             0  :  256k default Lob page size
                          4096  :  4k
                          8192  :  8k
                         16384  :  16k
                         32768  :  32k
                         65536  :  64k
                        131072  :  128k
                        262144  :  256k
                        524288  :  512k
        """
        ops = {}
        if not isinstance(cs_name, str_type):
            raise SDBTypeError("name of collection space must be an instance of str_type")
        if isinstance(options, int):
            if options not in [0, 4096, 8192, 16384, 32768, 65536]:
                raise SDBTypeError("page size is invalid")
            ops["PageSize"] = options
        elif isinstance(options, dict):
            ops = options
        else:
            raise SDBTypeError("options must be an instance of int")

        bson_options = bson.BSON.encode(ops)

        cs = collectionspace()
        try:
            rc = sdb.sdb_create_collection_space(self._client, cs_name,
                                                 bson_options, cs._cs)
            raise_if_error(rc, "Failed to create collection space: %s" % cs_name)
        except SDBBaseError:
            del cs
            raise

        return cs

    def drop_collection_space(self, cs_name):
        """Remove the specified collection space.

        Parameters:
           Name         Type     Info:
           cs_name      str      The name of collection space to be dropped
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(cs_name, str_type):
            raise SDBTypeError("name of collection space must be\
                         an instance of str_type")

        rc = sdb.sdb_drop_collection_space(self._client, cs_name)
        raise_if_error(rc, "Failed to drop collection space: %s" % cs_name)

    def list_collection_spaces(self):
        """List all collection space of current database, include temporary
           collection space.

        Return values:
           a cursor object of collection spaces.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        result = cursor()
        try:
            rc = sdb.sdb_list_collection_spaces(self._client, result._cursor)
            raise_if_error(rc, "Failed to list collection spaces")
        except SDBBaseError:
            del result
            raise

        return result

    def list_collections(self):
        """List all collections in current database.

        Return values:
           a cursor object of collection.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        result = cursor()
        try:
            rc = sdb.sdb_list_collections(self._client, result._cursor)
            raise_if_error(rc, "Failed to list collections")
        except SDBBaseError:
            del result
            raise

        return result

    def list_replica_groups(self):
        """List all replica groups of current database.

        Return values:
           a cursor object of replication groups.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        result = cursor()
        try:
            rc = sdb.sdb_list_replica_groups(self._client, result._cursor)
            raise_if_error(rc, "Failed to list replica groups")
        except SDBBaseError:
            del result
            raise

        return result

    def get_replica_group_by_name(self, group_name):
        """Get the specified replica group of specified group name.

        Parameters:
           Name         Type     Info:
           group_name   str      The name of replica group.
        Return values:
           the replicagroup object of query.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(group_name, str_type):
            raise SDBTypeError("group name must be an instance of str_type")

        result = replicagroup(self._client)
        try:
            rc = sdb.sdb_get_replica_group_by_name(self._client, group_name,
                                                   result._group)
            raise_if_error(rc, "Failed to get specified group: %s" % group_name)
        except SDBBaseError:
            del result
            raise

        return result

    def get_replica_group_by_id(self, group_id):
        """Get the specified replica group of specified group id.

        Parameters:
           Name       Type     Info:
           group_id   int      The id of replica group.
        Return values:
           the replicagroup object of query.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(group_id, int):
            raise SDBTypeError("group id must be an instance of int")

        result = replicagroup(self._client)
        try:
            rc = sdb.sdb_get_replica_group_by_id(self._client, group_id, result._group)
            raise_if_error(rc, "Failed to get specified group: %d" % group_id)
        except SDBBaseError:
            del result
            raise

        return result

    def get_cata_replica_group(self):
        """Get catalog replica group

        Return values:
           The catalog replicagroup object.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        return self.get_replica_group_by_name(SDB_CATALOG_GROUP_NAME)

    def get_coord_replica_group(self):
        """Get coordinator replica group

        Return values:
           The coordinator replicagroup object.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        return self.get_replica_group_by_name(SDB_COORD_GROUP_NAME)

    def create_replica_group(self, group_name):
        """Create the specified replica group.

        Parameters:
           Name        Type     Info:
           group_name  str      The name of replica group to be created.
        Return values:
           The created replicagroup object.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(group_name, str_type):
            raise SDBTypeError("group name must be an instance of str_type")

        replica_group = replicagroup(self._client)
        try:
            rc = sdb.sdb_create_replica_group(self._client, group_name,
                                              replica_group._group)
            raise_if_error(rc, "Failed to create replica group: %s" % group_name)
        except SDBBaseError:
            del replica_group
            raise

        return replica_group

    def create_cata_replica_group(self, host, service, path, configure=None):
        """Create catalog replica group.

        Parameters:
           Name         Type     Info:
           host         str      The hostname for the catalog replica group.
           service      str      The service name for the catalog replica group.
           path         str      The path for the catalog replica group.
           configure    dict     The optional configurations for the catalog replica group.
        Return values:
           The created catalog replicagroup object.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if configure is None:
            configure = {}
        if not isinstance(host, str_type):
            raise SDBTypeError("host must be an instance of str_type")
        if not isinstance(service, str_type):
            raise SDBTypeError("service name must be an instance of str_type")
        if not isinstance(path, str_type):
            raise SDBTypeError("path must be an instance of str_type")
        if not isinstance(configure, dict):
            raise SDBTypeError("configure must be an instance of dict")

        bson_configure = bson.BSON.encode(configure)

        rc = sdb.sdb_create_replica_cata_group(self._client, host, service,
                                               path, bson_configure)
        raise_if_error(rc, "Failed to create catalog group")

        return self.get_cata_replica_group()

    def create_coord_replica_group(self):
        """Create coordinator replica group.

        Return values:
           The created coordinator replicagroup object.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        return self.create_replica_group(SDB_COORD_GROUP_NAME)

    def create_replica_cata_group(self, host, service, path, configure=None):
        """Use create_cata_replica_group instead.
        """
        return self.create_cata_replica_group(host, service, path, configure)

    def remove_replica_group(self, group_name):
        """Remove the specified replica group.

        Parameters:
           Name         Type     Info:
           group_name   str      The name of replica group to be removed
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(group_name, str_type):
            raise SDBTypeError("group name must be an instance of str_type")

        rc = sdb.sdb_remove_replica_group(self._client, group_name)
        raise_if_error(rc, "Failed to remove replica group: %s" % group_name)

    def remove_cata_replica_group(self):
        """Remove the catalog replica group.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        return self.remove_replica_group(SDB_CATALOG_GROUP_NAME)

    def remove_coord_replica_group(self):
        """Remove the coordinator replica group.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        return self.remove_replica_group(SDB_COORD_GROUP_NAME)

    def start_replica_group(self, group_name):
        """Start the specified replica group.

        Parameters:
           Name         Type     Info:
           group_name   str      The name of replica group to be started
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(group_name, str_type):
            raise SDBTypeError("group name must be an instance of str_type")
        self.get_replica_group_by_name(group_name).start()

    def stop_replica_group(self, group_name):
        """Stop the specified replica group.

        Parameters:
           Name         Type     Info:
           group_name   str      The name of replica group to be stopped
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(group_name, str_type):
            raise SDBTypeError("group name must be an instance of str_type")
        self.get_replica_group_by_name(group_name).stop()

    def exec_update(self, sql):
        """Executing SQL command for updating, inserting and deleting.

        Parameters:
           Name         Type     Info:
           sql          str      The SQL command.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(sql, str_type):
            raise SDBTypeError("update sql must be an instance of str_type")

        rc = sdb.sdb_exec_update(self._client, sql)
        raise_if_error(rc, "Failed to execute update sql: %s" % sql)

    def exec_sql(self, sql):
        """Executing SQL command for query.

        Parameters:
           Name         Type     Info:
           sql          str      The SQL command.
        Return values:
           a cursor object of matching documents.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(sql, str_type):
            raise SDBTypeError("sql must be an instance of str_type")

        result = cursor()
        try:
            rc = sdb.sdb_exec_sql(self._client, sql, result._cursor)
            raise_if_error(rc, "Failed to execute sql: %s" % sql)
        except SDBBaseError:
            del result
            raise

        return result

    def transaction_begin(self):
        """Transaction begin.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.sdb_transaction_begin(self._client)
        raise_if_error(rc, "Transaction begin error")

    def transaction_commit(self):
        """Transaction commit.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.sdb_transaction_commit(self._client)
        raise_if_error(rc, "Transaction commit error")

    def transaction_rollback(self):
        """Transaction rollback

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.sdb_transaction_rollback(self._client)
        raise_if_error(rc, "Transaction rollback error")

    def flush_configure(self, options):
        """Flush the options to configure file.
        Parameters:
           Name      Type  Info:
           options   dict  The configure information, pass {"Global":true} or
                                 {"Global":false} In cluster environment, passing
                                 {"Global":true} will flush data's and catalog's
                                 configuration file, while passing {"Global":false} will
                                 flush coord's configuration file. In stand-alone
                                 environment, both them have the same behaviour.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of dict")

        bson_options = bson.BSON.encode(options)
        rc = sdb.sdb_flush_configure(self._client, bson_options)
        raise_if_error(rc, "Failed to flush configure")

    def create_procedure(self, code):
        """Create a store procedures

        Parameters:
           Name         Type     Info:
           code         str      The JS code of store procedures.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(code, str_type):
            raise SDBTypeError("code must be an instance of str_type")

        rc = sdb.sdb_create_JS_procedure(self._client, code)
        raise_if_error(rc, "Failed to crate procedure")

    def remove_procedure(self, name):
        """Remove a store procedures.

        Parameters:
           Name         Type     Info:
           name         str      The name of store procedure.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(name, str_type):
            raise SDBTypeError("procedure name must be an instance of str_type")

        rc = sdb.sdb_remove_procedure(self._client, name)
        raise_if_error(rc, "Failed to remove procedure: %s" % name)

    def list_procedures(self, condition):
        """List store procedures.

        Parameters:
           Name         Type     Info:
           condition    dict     The condition of list.
        Return values:
           an cursor object of result
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(condition, dict):
            raise SDBTypeError("condition must be an instance of dict")

        bson_condition = bson.BSON.encode(condition)
        result = cursor()
        try:
            rc = sdb.sdb_list_procedures(self._client, result._cursor,
                                         bson_condition)
            raise_if_error(rc, "Failed to list procedures")
        except SDBBaseError:
            del result
            raise

        return result

    def eval_procedure(self, name):
        """Eval a func.

        Parameters:
           Name         Type     Info:
           name         str      The name of store procedure.
        Return values:
           cursor object of current eval.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(name, str_type):
            raise SDBTypeError("code must be an instance of str_type")

        result = cursor()
        try:
            rc, _type, bson_errmsg = sdb.sdb_eval_JS(self._client, result._cursor, name)
            if SDB_OK != rc:
                record, size = bson._bson_to_dict(bson_errmsg, dict, False,
                                                  bson.OLD_UUID_SUBTYPE, True)
                raise_if_error(rc, "Failed to eval procedure: %s" % record)
        except SDBBaseError:
            del result
            raise

        return result

    def backup_offline(self, options=None):
        """Backup the whole database or specified replica group.

        Parameters:
           Name      Type  Info:
           options   dict  Contains a series of backup configuration
                                 information. Backup the whole cluster if None.
                                 The "options" contains 5 options as below.
                                 All the elements in options are optional.
                                 eg:
                                 { "GroupName":["rgName1", "rgName2"],
                                   "Path":"/opt/sequoiadb/backup",
                                   "Name":"backupName", "Description":description,
                                   "EnsureInc":true, "OverWrite":true }
                                 See Info as below.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           GroupName   :  The replica groups which to be backuped.
           Path        :  The backup path, if not assign, use the backup path assigned in configuration file.
           Name        :  The name for the backup.
           Description :  The description for the backup.
           EnsureInc   :  Whether execute increment synchronization,
                                default to be false.
           OverWrite   :  Whether overwrite the old backup file,
                                default to be false.
        """
        bson_options = None
        if options is not None:
            if not isinstance(options, dict):
                raise SDBTypeError("options must be an instance of dict")
            bson_options = bson.BSON.encode(options)

        rc = sdb.sdb_backup_offline(self._client, bson_options)
        raise_if_error(rc, "Failed to backup offline")

    def list_backup(self, options, **kwargs):
        """List the backups.

        Parameters:
           Name        Type     Info:
           options     dict     Contains configuration information for backups,
                                list all the backups if the default backup path is None.
                                       The "options" contains 3 options as below.
                                       All the elements in options are optional.
                                       eg:
                                       { "GroupName":["rgame1", "rgName2"],
                                         "Path":"/opt/sequoiadb/backup",
                                         "Name":"backupName" }
                                       See Info as below.
           **kwargs             Useful option arw below
           - condition dict     The matching rule, return all the documents
                                      if None.
           - selector  dict     The selective rule, return the whole document
                                      if None.
           - order_by  dict     The ordered rule, never sort if None.
        Return values:
           a cursor object of backup list
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           GroupName   :  Assign the backups of specified replica groups to be list.
           Path        :  Assign the backups in specified path to be list,
                                if not assign, use the backup path assigned in the
                                configuration file.
           Name        :  Assign the backups with specified name to be list.
        """

        bson_condition = None
        bson_selector = None
        bson_order_by = None

        if not isinstance(options, dict):
            raise SDBTypeError("options in kwargs must be an instance of dict")

        bson_options = bson.BSON.encode(options)
        if "condition" in kwargs:
            if not isinstance(kwargs.get("condition"), dict):
                raise SDBTypeError("condition in kwargs must be an instance of dict")
            bson_condition = bson.BSON.encode(kwargs.get("condition"))
        if "selector" in kwargs:
            if not isinstance(kwargs.get("selector"), dict):
                raise SDBTypeError("selector in kwargs must be an instance of dict")
            bson_selector = bson.BSON.encode(kwargs.get("selector"))
        if "order_by" in kwargs:
            if not isinstance(kwargs.get("order_by"), dict):
                raise SDBTypeError("order_by in kwargs must be an instance of dict")
            bson_order_by = bson.BSON.encode(kwargs.get("order_by"))

        result = cursor()
        try:
            rc = sdb.sdb_list_backup(self._client, result._cursor, bson_options,
                                     bson_condition, bson_selector, bson_order_by)
            raise_if_error(rc, "Failed to list backup")
        except SDBBaseError:
            del result
            raise

        return result

    def remove_backup(self, options):
        """Remove the backups

        Parameters:
           Name      Type  Info:
           options   dict  Contains configuration information for remove
                                 backups, remove all the backups in the default
                                 backup path if null. The "options" contains 3
                                 options as below. All the elements in options are
                                 optional.
                                 eg:
                                 { "GroupName":["rgName1", "rgName2"],
                                   "Path":"/opt/sequoiadb/backup",
                                   "Name":"backupName" }
                                 See Info as below.
        Return values:
           an cursor object of result
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           GroupName   : Assign the backups of specified replica groups to be
                               remove.
           Path        : Assign the backups in specified path to be remove, if not
                               assign, use the backup path assigned in the configuration
                               file.
           Name        : Assign the backups with specified name to be remove.
        """
        if not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of dict")
        bson_options = bson.BSON.encode(options)

        rc = sdb.sdb_remove_backup(self._client, bson_options)
        raise_if_error(rc, "Failed to remove backup")

    def list_tasks(self, **kwargs):
        """List the tasks.

        Parameters:
           Name           Type     Info:
           **kwargs                Useful options are below
           - condition    dict     The matching rule, return all the documents
                                         if None.
           - selector     dict     The selective rule, return the whole
                                         document if None.
           - order_by     dict     The ordered rule, never sort if None.
                                         bson.SON may need if it is order-sensitive.
                                         eg.
                                         bson.SON([("name",-1), ("age":1)]) it will
                                         be ordered descending by 'name' first, and
                                         be ordered ascending by 'age'
           - hint         dict     The hint, automatically match the optimal
                                         hint if None.
        Return values:
           a cursor object of task list
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        bson_condition = None
        bson_selector = None
        bson_order_by = None
        bson_hint = None

        if "condition" in kwargs:
            if not isinstance(kwargs["condition"], dict):
                raise SDBTypeError("condition in kwargs must be an instance of dict")
            bson_condition = bson.BSON.encode(kwargs["condition"])
        if "selector" in kwargs:
            if not isinstance(kwargs["selector"], dict):
                raise SDBTypeError("selector in kwargs must be an instance of dict")
            bson_selector = bson.BSON.encode(kwargs["selector"])
        if "order_by" in kwargs:
            if not isinstance(kwargs["order_by"], dict):
                raise SDBTypeError("order_by in kwargs must be an instance of dict")
            bson_order_by = bson.BSON.encode(kwargs["order_by"])
        if "hint" in kwargs:
            if not isinstance(kwargs["hint"], dict):
                raise SDBTypeError("hint in kwargs must be an instance of dict")
            bson_hint = bson.BSON.encode(kwargs["hint"])

        result = cursor()
        try:
            rc = sdb.sdb_list_tasks(self._client, result._cursor, bson_condition,
                                    bson_selector, bson_order_by, bson_hint)
            raise_if_error(rc, "Failed to list tasks")
        except SDBBaseError:
            del result
            raise

        return result

    def wait_task(self, task_ids, num):
        """Wait the tasks to finish.

        Parameters:
           Name         Type     Info:
           task_ids     list     The list of task id.
           num          int      The number of task id.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(task_ids, list):
            raise SDBTypeError("task id must be an instance of list")
        if not isinstance(num, int):
            raise SDBTypeError("size of tasks must be an instance of int")

        rc = sdb.sdb_wait_task(self._client, task_ids, num)
        raise_if_error(rc, "Failed to wait task")

    def cancel_task(self, task_id, is_async):
        """Cancel the specified task.

        Parameters:
           Name         Type     Info:
           task_id      long     The task id to be canceled.
           is_async     bool     The operation "cancel task" is async or not,
                                       "True" for async, "False" for sync.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(task_id, long_type):
            raise SDBTypeError("task id must be an instance of list")

        async = 0
        if isinstance(is_async, bool):
            if is_async:
                async = 1
        else:
            raise SDBTypeError("size of tasks must be an instance of int")

        rc = sdb.sdb_cancel_task(self._client, task_id, async)
        raise_if_error(rc, "Failed to cancel task")

    def set_session_attri(self, options):
        """Set the attributes of the session.

        Parameters:
           Name         Type     Info:
           options      dict     The configuration options for session. Could have "PreferedInstance", "PreferedInstanceMode" and "Timeout"
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           PreferedInstance     : Preferred instance for read request in the current session. Could be single value in "M", "m", "S", "s", "A", "a", 1-255, or BSON Array to include multiple values. e.g. { "PreferedInstance" : [ 1, 7 ] }.
                                        "M", "m": read and write instance( master instance ). If multiple numeric instances are given with "M", matched master instance will be chosen in higher priority. If multiple numeric instances are given with "M" or "m", master instance will be chosen if no numeric instance is matched.
                                        "S", "s": read only instance( slave instance ). If multiple numeric instances are given with "S", matched slave instances will be chosen in higher priority. If multiple numeric instances are given with "S" or "s", slave instance will be chosen if no numeric instance is matched.
                                        "A", "a": any instance.
                                        1-255: the instance with specified instance ID.
                                        If multiple alphabet instances are given, only first one will be used.
                                        If matched instance is not found, will choose instance by random.
           PreferedInstanceMode : The mode to choose query instance when multiple preferred instances are found in the current session. e.g. { "PreferedInstanceMode : "random" }.
                                        "random": choose the instance from matched instances by random.
                                        "ordered": choose the instance from matched instances by the order of "PreferedInstance".
           Timeout              : The timeout (in ms) for operations in the current session. -1 means no timeout for operations. e.g. { "Timeout" : 10000 }.
        """
        if not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of dict")
        bson_options = bson.BSON.encode(options)

        rc = sdb.sdb_set_session_attri(self._client, bson_options)
        raise_if_error(rc, "Failed to set session attribute")

    def get_session_attri(self):
        """Get the attributes of the session.

        Return values:
           a dict object of attributes
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, result = sdb.sdb_get_session_attri(self._client)
        raise_if_error(rc, "Failed to get session attributes")
        record, size = bson._bson_to_dict(result, dict, False,
                                          bson.OLD_UUID_SUBTYPE, True)
        return record

    def close_all_cursors(self):
        """Close all the cursors in current thread, we can't use those cursors to
        get data again.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.sdb_close_all_cursors(self._client)
        raise_if_error(rc, "Failed to close all cursors")

    def is_valid(self):
        """Judge whether the connection is valid.

        Return values:
           bool
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc, valid = sdb.sdb_is_valid(self._client)
        raise_if_error(rc, "connection is invalid")

        return valid

    def list_domains(self, **kwargs):
        """Get information of domains.

        Parameters:
           Name        Type     Info:
           **kwargs             Useful options are below
           - condition dict     The matching rule, match all the documents
                                      if None.
           - selector  dict     The selective rule, return the whole
                                      documents if None.
           - order_by  dict     The ordered rule, never sort if None.
        Return values:
           a cursor object of domains
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        return self.get_list(SDB_LIST_DOMAINS, **kwargs)

    def is_domain_existing(self, domain_name):
        """Check the existence of specified domain.

        Parameters:
            Name        Type     Info
            domain_name  str     The domain name.
        Return values:
           True if domain exists and False if not exists
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(domain_name, str_type):
            raise SDBTypeError("domain name must be an instance of str_type")
        domain_cursor = self.list_domains(condition={"Name": domain_name})
        try:
            domain_cursor.next()
            del domain_cursor
            return True
        except SDBBaseError as e:
            if e.errcode == SDB_DMS_EOC:
                return False
            else:
                raise e

    def create_domain(self, domain_name, options=None):
        """Create domain.

        Parameters:
           Name        Type     Info
           domain_name  str     The domain name.
           options      dict    The options for the domain. The options are as below:
                                Groups: the list of the replica groups' names which the domain is going to contain.
                                        eg: { "Groups": [ "group1", "group2", "group3" ] }
                                        If this argument is not included, the domain will contain all replica groups in the cluster.
                                AutoSplit: If this option is set to be true, while creating collection(ShardingType is "hash") in this domain,
                                           the data of this collection will be split(hash split) into all the groups in this domain automatically.
                                           However, it won't automatically split data into those groups which were add into this domain later.
                                           eg: { "Groups": [ "group1", "group2", "group3" ], "AutoSplit: true" }
        Return values:
           The created domain object.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if options is None:
            options = {}
        if not isinstance(domain_name, str_type):
            raise SDBTypeError("domain name must be an instance of str_type")
        if not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of dict")
        bson_options = bson.BSON.encode(options)

        dm = domain(domain_name)
        try:
            rc = sdb.sdb_create_domain(self._client, domain_name, bson_options, dm._domain)
            raise_if_error(rc, "Failed to create domain: %s" % domain_name)
        except SDBBaseError:
            del dm
            raise
        return dm

    def drop_domain(self, domain_name):
        """Drop the specified domain.

        Parameters:
            Name        Type     Info
            domain_name  str     The domain name.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(domain_name, str_type):
            raise SDBTypeError("domain name must be an instance of str_type")

        rc = sdb.sdb_drop_domain(self._client, domain_name)
        raise_if_error(rc, "Failed to drop domain: %s" % domain_name)

    def get_domain(self, domain_name):
        """Get the specified domain.

        Parameters:
            Name        Type     Info
            domain_name  str     The domain name.
        Return values:
           The specified domain object.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(domain_name, str_type):
            raise SDBTypeError("domain name must be an instance of str_type")

        dm = domain(domain_name)
        try:
            rc = sdb.sdb_get_domain(self._client, domain_name, dm._domain)
            raise_if_error(rc, "Failed to get domain: %s" % domain_name)
        except SDBBaseError:
            del dm
            raise

        return dm

    def sync(self, options=None):
        """Sync database which are specified.

        Parameters:
           Name         Type     Info:
           options      dict     The control options:
                                 Deep:
                                     (INT32) Flush with deep mode or not. 1 in default.
                                     0 for non-deep mode,1 for deep mode,-1 means use the configuration with server
                                 Block:
                                     (Bool) Flush with block mode or not. false in default.
                                 CollectionSpace:
                                     (String) Specify the collectionspace to sync.
                                     If not set, will sync all the collectionspaces and logs,
                                     otherwise, will only sync the collectionspace specified.
                                 Others:(Only take effect in coordinate nodes)
                                     GroupID:INT32,
                                     GroupName:String,
                                     NodeID:INT32,
                                     HostName:String,
                                     svcname:String
                                     ...
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        bson_options = None
        if options is not None:
            if not isinstance(options, dict):
                raise SDBTypeError("options must be an instance of dict")
            bson_options = bson.BSON.encode(options)

        rc = sdb.sdb_sync(self._client, bson_options)
        raise_if_error(rc, "Failed to sync")

    def analyze(self, options=None):
        """Analyze collection or index to collect statistics information

        Parameters:
           Name         Type     Info:
           options      dict     The control options:
                                 CollectionSpace:
                                    (String) Specify the collection space to be analyzed.
                                 Collection:
                                    (String) Specify the collection to be analyzed.
                                 Index:
                                    (String) Specify the index to be analyzed.
                                 Mode:
                                    (Int32) Specify the analyze mode (default is 1):
                                    Mode 1 will analyze with data samples.
                                    Mode 2 will analyze with full data.
                                    Mode 3 will generate default statistics.
                                    Mode 4 will reload statistics into memory cache.
                                    Mode 5 will clear statistics from memory cache.
                                 Others:
                                    Only take effect in coordinate nodes)
                                    GroupID:INT32,
                                    GroupName:String,
                                    NodeID:INT32,
                                    HostName:String,
                                    svcname:String,
                                    ...
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        bson_options = None
        if options is not None:
            if not isinstance(options, dict):
                raise SDBTypeError("options must be an instance of dict")
            bson_options = bson.BSON.encode(options)

        rc = sdb.sdb_analyze(self._client, bson_options)
        raise_if_error(rc, "Failed to analyze")

