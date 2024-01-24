# coding=utf-8
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
from pysequoiadb.sequence import sequence
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
SDB_LIST_SVCTASKS = 14
SDB_LIST_SEQUENCES = 15
SDB_LIST_USERS = 16
SDB_LIST_BACKUPS = 17
#SDB_LIST_RESERVED1 = 18
#SDB_LIST_RESERVED2 = 19
#SDB_LIST_RESERVED3 = 20
#SDB_LIST_RESERVED4 = 21
SDB_LIST_CL_IN_DOMAIN = 129
SDB_LIST_CS_IN_DOMAIN = 130

SDB_LIST_TYPE = [
    SDB_LIST_CONTEXTS,
    SDB_LIST_CONTEXTS_CURRENT,
    SDB_LIST_SESSIONS,
    SDB_LIST_SESSIONS_CURRENT,
    SDB_LIST_COLLECTIONS,
    SDB_LIST_COLLECTIONSPACES,
    SDB_LIST_STORAGEUNITS,
    SDB_LIST_GROUPS,
    SDB_LIST_STOREPROCEDURES,
    SDB_LIST_DOMAINS,
    SDB_LIST_TASKS,
    SDB_LIST_TRANSACTIONS,
    SDB_LIST_TRANSACTIONS_CURRENT,
    SDB_LIST_SVCTASKS,
    SDB_LIST_SEQUENCES,
    SDB_LIST_USERS,
    SDB_LIST_BACKUPS,
    SDB_LIST_CL_IN_DOMAIN,
    SDB_LIST_CS_IN_DOMAIN
]

SDB_SNAP_CONTEXTS = 0
SDB_SNAP_CONTEXTS_CURRENT = 1
SDB_SNAP_SESSIONS = 2
SDB_SNAP_SESSIONS_CURRENT = 3
SDB_SNAP_COLLECTIONS = 4
SDB_SNAP_COLLECTIONSPACES = 5
SDB_SNAP_DATABASE = 6
SDB_SNAP_SYSTEM = 7
SDB_SNAP_CATALOG = 8
SDB_SNAP_TRANSACTIONS = 9
SDB_SNAP_TRANSACTIONS_CURRENT = 10
SDB_SNAP_ACCESSPLANS = 11
SDB_SNAP_HEALTH = 12
SDB_SNAP_CONFIGS = 13
SDB_SNAP_SVCTASKS = 14
SDB_SNAP_SEQUENCES = 15
#SDB_SNAP_RESERVED1 = 16
#SDB_SNAP_RESERVED2 = 17
SDB_SNAP_QUERIES = 18
SDB_SNAP_LATCHWAITS = 19
SDB_SNAP_LOCKWAITS = 20
SDB_SNAP_INDEXSTATS = 21
SDB_SNAP_TRANSWAITS = 25
SDB_SNAP_TRANSDEADLOCK = 26

SDB_SNAP_TYPE = [
    SDB_SNAP_CONTEXTS,
    SDB_SNAP_CONTEXTS_CURRENT,
    SDB_SNAP_SESSIONS,
    SDB_SNAP_SESSIONS_CURRENT,
    SDB_SNAP_COLLECTIONS,
    SDB_SNAP_COLLECTIONSPACES,
    SDB_SNAP_DATABASE,
    SDB_SNAP_SYSTEM,
    SDB_SNAP_CATALOG,
    SDB_SNAP_TRANSACTIONS,
    SDB_SNAP_TRANSACTIONS_CURRENT,
    SDB_SNAP_ACCESSPLANS,
    SDB_SNAP_HEALTH,
    SDB_SNAP_CONFIGS,
    SDB_SNAP_SVCTASKS,
    SDB_SNAP_SEQUENCES,
    SDB_SNAP_QUERIES,
    SDB_SNAP_LATCHWAITS,
    SDB_SNAP_LOCKWAITS,
    SDB_SNAP_INDEXSTATS,
    SDB_SNAP_TRANSWAITS,
    SDB_SNAP_TRANSDEADLOCK
]

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

    def __init__(self, host=None, service=None, user=None, psw=None, ssl=False, host_list=None, policy=None, token=None,
                 cipher_file=None):
        """initialize when product an object.

           it will try to connect to SequoiaDB.

        Parameters:
           Name           Type      Info:
           host           str       The hostname or IP address of SequoiaDB server. If None, "localhost" will be used.
           service        str/int   The service name or port number of SequoiaDB server. If None, "11810" will be used.
           user           str       The user name to access to SequoiaDB server. If None, "" will be used.
           psw            str       The user password to access to SequoiaDB server. If None, "" will be used.
           ssl            bool      Decide whether to use ssl or not, default is False.
           host_list      list      The list contains hosts. If both 'host' and 'host_list' exist, the 'host' is
                                    preferred, if the size of the 'host_list' is 0, "localhost" will be used.
                                    eg.
                                    [ {'host':'sdbservre1', 'service':11810},
                                      {'host':'sdbservre2', 'service':11810},
                                      {'host':'sdbservre3', 'service':11810} ]
           policy         str       The policy of select hosts. it must be string of 'random' or 'local_first' or
                                    'one_by_one', default is 'random'. 'local_first' will choose local host firstly,
                                    then use 'random' if no local host.
           token          str       The Password encryption token, it needs to used with 'cipher_file'.
           cipher_file    str       The cipher file location, if both 'psw' and 'cipher_file' exist, the 'psw' is
                                    preferred.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        self.__connected = False
        _host_list = None

        if host is not None:
            if isinstance(host, str_type):
                self.__host = host
            else:
                raise SDBTypeError("host must be an instance of str_type")
        else:
            if host_list is None:
                self.__host = self.HOST
            elif isinstance(host_list, list):
                if len(host_list) == 0:
                    self.__host = self.HOST
                else:
                    _host_list = host_list
            else:
                raise SDBTypeError("host_list must be an instance of list")

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

        if _host_list is None:
            self.connect(self.__host, self.__service, user=_user, password=_psw, token=token,
                         cipher_file=cipher_file)
        else:
            self.connect_to_hosts(_host_list, user=_user, password=_psw, token=token,
                                  cipher_file=cipher_file, policy=policy)

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
        if '_client' == name:
           self._client = None
        elif '__members__' == name or '__methods__' == name:
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
           Name           Type     Info:
           hosts          list     The list contains hosts.
                                   eg.
                                   [ {'host':'sdbservre1', 'service':11810},
                                     {'host':'sdbservre2', 'service':11810},
                                     {'host':'sdbservre3', 'service':11810} ]
           **kwargs       Useful options are below:
           -  user        str      The user name to access to database.
           -  password    str      The user password to access to database.
           -  policy      str      The policy of select hosts. it must be string of 'random' or 'local_first' or
                                   'one_by_one', default is 'random'. 'local_first' will choose local host firstly,
                                   then use 'random' if no local host.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(hosts, list):
            raise SDBTypeError("hosts must be an instance of list")

        policy = kwargs.get("policy")
        if policy is None:
            policy = "random"
        elif not isinstance(policy, str):
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

        token = kwargs.get("token")
        if token is None:
            token = ""
        elif not isinstance(token, str_type):
            raise SDBTypeError("token must be an instance of str_type")

        cipher_file = kwargs.get("cipher_file")
        if cipher_file is None:
            cipher_file = ""
        elif not isinstance(cipher_file, str_type):
            raise SDBTypeError("cipher_file must be an instance of str_type")

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
                                     user=_user, password=_psw, token=token, cipher_file=cipher_file)
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
                             user=_user, password=_psw, token=token, cipher_file=cipher_file)
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
           Name           Type     Info:
           host           str      The host name or IP address of database server.
           service        int/str  The service name of database server.
           **kwargs                Useful options are below:
           -  user        str      The user name to access to database.
           -  password    str      The user password to access to database,

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

        token = kwargs.get("token")
        if token is None:
            token = ""
        elif not isinstance(token, str_type):
            raise SDBTypeError("token must be an instance of str_type")

        cipher_file = kwargs.get("cipher_file")
        if cipher_file is None:
            cipher_file = ""
        elif not isinstance(cipher_file, str_type):
            raise SDBTypeError("cipher_file must be an instance of str_type")

        hosts_list = []
        hosts_list.append(self.__host + ":" + self.__service)
        rc = sdb.sdb_connect(self._client, hosts_list, len(hosts_list),
                             _user, _psw, token, cipher_file)
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
           Name              Type     Info:
           snap_type         int      The type of snapshot, see Info as below
           **kwargs                   Useful options are below
           - condition       dict     The matching rule, match all the documents if not provided.
           - selector        dict     The selective rule, return the whole document if not provided.
           - order_by        dict     The ordered rule, result set is unordered if not provided.
           - hint            dict     The options provided for specific snapshot type. Format:{ '$Options': { <options> } }
           - num_to_skip     int     Skip the first numToSkip documents, default is 0.
           - num_to_return   int     Only return numToReturn documents, default is -1 for returning all results.
        Return values:
           a cursor object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
          snapshot type:
                    SDB_SNAP_CONTEXTS              : Get all contexts' snapshot
                    SDB_SNAP_CONTEXTS_CURRENT      : Get the current context's snapshot
                    SDB_SNAP_SESSIONS              : Get all sessions' snapshot
                    SDB_SNAP_SESSIONS_CURRENT      : Get the current session's snapshot
                    SDB_SNAP_COLLECTIONS           : Get the collections' snapshot
                    SDB_SNAP_COLLECTIONSPACES      : Get the collection spaces' snapshot
                    SDB_SNAP_DATABASE              : Get database's snapshot
                    SDB_SNAP_SYSTEM                : Get system's snapshot
                    SDB_SNAP_CATALOG               : Get catalog's snapshot
                    SDB_SNAP_TRANSACTIONS          : Get transactions' snapshot
                    SDB_SNAP_TRANSACTIONS_CURRENT  : Get current session's transaction snapshot
                    SDB_SNAP_ACCESSPLANS           : Get cached access plan snapshot
                    SDB_SNAP_HEALTH                : Get node health detection snapshot
                    SDB_SNAP_CONFIGS               : Get node configuration's snapshot
                    SDB_SNAP_SVCTASKS              : Get the snapshot of service tasks
                    SDB_SNAP_SEQUENCES             : Get the snapshot of sequences
                    SDB_SNAP_QUERIES               : Get the snapshot of queries
                    SDB_SNAP_LATCHWAITS            : Get the snapshot of latch waits
                    SDB_SNAP_LOCKWAITS             : Get the snapshot of lock waits
                    SDB_SNAP_INDEXSTATS            : Get the snapshot of index statistics
                    SDB_SNAP_TRANSWAITS            : Get the snapshot of transaction waits
                    SDB_SNAP_TRANSDEADLOCK         : Get the snapshot of transaction deadlock
        """
        if not isinstance(snap_type, int):
            raise SDBTypeError("snap type must be an instance of int")
        if snap_type not in SDB_SNAP_TYPE:
            raise SDBTypeError("snap_type value is invalid")

        bson_condition = None
        bson_selector = None
        bson_order_by = None
        bson_hint = None
        num_to_skip = 0
        num_to_return = -1

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
        if "hint" in kwargs:
            if not isinstance(kwargs.get("hint"), dict):
                raise SDBTypeError("hint in kwargs must be an instance of dict")
            bson_hint = bson.BSON.encode(kwargs.get("hint"))
        if "num_to_skip" in kwargs:
            if not isinstance(kwargs.get("num_to_skip"), int):
                raise SDBTypeError("num_to_skip must be an instance of int")
            num_to_skip = kwargs.get("num_to_skip")
        if "num_to_return" in kwargs:
            if not isinstance(kwargs.get("num_to_return"), int):
                raise SDBTypeError("num_to_return must be an instance of int")
            num_to_return = kwargs.get("num_to_return")

        result = cursor()
        try:
            rc = sdb.sdb_get_snapshot(self._client, result._cursor, snap_type,
                                      bson_condition, bson_selector, bson_order_by,
                                      bson_hint, num_to_skip, num_to_return)
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
           Name              Type     Info:
           list_type         int      Type of list option, see Info as below.
           **kwargs                   Useful options are below
           - condition       dict     The matching rule, match all the documents
                                              if None.
           - selector        dict     The selective rule, return the whole
                                              documents if None.
           - order_by        dict     The ordered rule, never sort if None.
           - hint            dict     The options provided for specific list type. Reserved.
           - num_to_skip     long     Skip the first numToSkip documents,
                                              default is 0L.
           - num_to_return   long     Only return numToReturn documents,
                                              default is -1L for returning
                                              all results.
        Return values:
           a cursor object of query
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           list type:
                     SDB_LIST_CONTEXTS              : Get all contexts list
                     SDB_LIST_CONTEXTS_CURRENT      : Get contexts list for the current session
                     SDB_LIST_SESSIONS              : Get all sessions list
                     SDB_LIST_SESSIONS_CURRENT      : Get the current session
                     SDB_LIST_COLLECTIONS           : Get all collections list
                     SDB_LIST_COLLECTIONSPACES      : Get all collection spaces' list
                     SDB_LIST_STORAGEUNITS          : Get storage units list
                     SDB_LIST_GROUPS                : Get replicaGroup list ( only applicable in sharding env )
                     SDB_LIST_STOREPROCEDURES       : Get store procedure list
                     SDB_LIST_DOMAINS               : Get domains list
                     SDB_LIST_TASKS                 : Get tasks list
                     SDB_LIST_TRANSACTIONS          : Get transactions list
                     SDB_LIST_TRANSACTIONS_CURRENT  : Get current session's transaction list
                     SDB_LIST_SVCTASKS              : Get all the schedule task informations
                     SDB_LIST_SEQUENCES             : Get all the sequence informations
                     SDB_LIST_USERS                 : Get all the user informations
                     SDB_LIST_BACKUPS               : Get backups list
        """
        if not isinstance(list_type, int):
            raise SDBTypeError("list type must be an instance of int")
        if list_type not in SDB_LIST_TYPE:
            raise SDBTypeError("list type value %d is not defined" %list_type)

        bson_condition = None
        bson_selector = None
        bson_order_by = None
        bson_hint = None
        num_to_skip = 0
        num_to_return = -1

        if "condition" in kwargs:
            if not isinstance(kwargs.get("condition"), dict):
                raise SDBTypeError("condition must be an instance of dict")
            bson_condition = bson.BSON.encode(kwargs.get("condition"))
        if "selector" in kwargs:
            if not isinstance(kwargs.get("selector"), dict):
                raise SDBTypeError("selector must be an instance of dict")
            bson_selector = bson.BSON.encode(kwargs.get("selector"))
        if "order_by" in kwargs:
            if not isinstance(kwargs.get("order_by"), dict):
                raise SDBTypeError("order_by must be an instance of dict")
            bson_order_by = bson.BSON.encode(kwargs.get("order_by"))
        if "hint" in kwargs:
            if not isinstance(kwargs.get("hint"), dict):
                raise SDBTypeError("hint must be an instance of dict")
            bson_hint = bson.BSON.encode(kwargs.get("hint"))
        if "num_to_skip" in kwargs:
            if not isinstance(kwargs.get("num_to_skip"), long_type):
                raise SDBTypeError("num_to_skip must be an instance of long")
            num_to_skip = kwargs.get("num_to_skip")
        if "num_to_return" in kwargs:
            if not isinstance(kwargs.get("num_to_return"), long_type):
                raise SDBTypeError("num_to_return must be an instance of long")
            num_to_return = kwargs.get("num_to_return")

        result = cursor()
        try:
            rc = sdb.sdb_get_list(self._client, result._cursor, list_type,
                                  bson_condition, bson_selector, bson_order_by,
                                  bson_hint, num_to_skip, num_to_return)
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

    def drop_collection_space(self, cs_name, options=None):
        """Remove the specified collection space.

        Parameters:
           Name             Type     Info:
           cs_name          str      The name of collection space to be dropped
           options          dict     The options for dropping collection, default to be None
            - EnsureEmpty   bool     Ensure the collection space is empty or not, default to be false.
                                      * True : Delete fails when the collection space is not empty
                                      * False: Directly delete the collection space
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        ops = {}
        if not isinstance(cs_name, str_type):
            raise SDBTypeError("name of collection space must be\
                         an instance of str_type")
        bson_options = None
        if options is not None:
            if not isinstance(options, dict):
                raise SDBTypeError("options must be an instance of dict")
            bson_options = bson.BSON.encode(options)

        rc = sdb.sdb_drop_collection_space(self._client, cs_name, bson_options)
        raise_if_error(rc, "Failed to drop collection space: %s" % cs_name)

    def rename_collection_space(self, old_name, new_name, options=None):
        """Rename the specified collection space.

        Parameters:
           Name         Type     Info:
           old_name     str      The original name of collection space.
           new_name     str      The new name of collection space.
           options      dict     Options for renaming.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(old_name, str_type):
            raise SDBTypeError("old name of collection space must be\
                         an instance of str_type")
        if not isinstance(new_name, str_type):
            raise SDBTypeError("new name of collection space must be\
                         an instance of str_type")
        bson_options = None
        if options is not None:
            if not isinstance(options, dict):
                raise SDBTypeError("options must be an instance of dict")
            bson_options = bson.BSON.encode(options)

        rc = sdb.sdb_rename_collection_space(self._client, old_name, new_name, bson_options)
        raise_if_error(rc, "Failed to rename collection space [%s] to [%s]"
                       % (old_name, new_name))

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

    def update_config(self, configs, options):
        """Force the node to update configs online.
        Parameters:
           Name      Type  Info:
           configs   dict  The specific configuration parameters to update
           options   dict  The configure information, pass {"Global":true} or
                                 {"Global":false} In cluster environment, passing
                                 {"Global":true} will flush data's and catalog's
                                 configuration file, while passing {"Global":false} will
                                 flush coord's configuration file. In stand-alone
                                 environment, both them have the same behaviour.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(configs, dict):
            raise SDBTypeError("configs must be an instance of dict")
        if not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of dict")

        bson_configs = bson.BSON.encode(configs)
        bson_options = bson.BSON.encode(options)
        rc = sdb.sdb_update_config(self._client, bson_configs, bson_options)
        raise_if_error(rc, "Failed to flush configure")

    def delete_config(self, configs, options):
        """Force the node to delete configs online.
        Parameters:
           Name      Type  Info:
           configs   dict  The specific configuration parameters to delete
           options   dict  The configure information, pass {"Global":true} or
                                 {"Global":false} In cluster environment, passing
                                 {"Global":true} will flush data's and catalog's
                                 configuration file, while passing {"Global":false} will
                                 flush coord's configuration file. In stand-alone
                                 environment, both them have the same behaviour.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(configs, dict):
            raise SDBTypeError("configs must be an instance of dict")
        if not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of dict")

        bson_configs = bson.BSON.encode(configs)
        bson_options = bson.BSON.encode(options)
        rc = sdb.sdb_delete_config(self._client, bson_configs, bson_options)
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

    def backup(self, options=None):
        """Backup the whole database or specified replica group.

        Parameters:
           Name      Type  Info:
           options   dict  Contains a series of backup configuration
                                 information. Backup the whole cluster if None.
                                 The "options" contains 6 options as below.
                                 All the elements in options are optional.
                                 eg:
                                 { "GroupName":["rgName1", "rgName2"],
                                   "Path":"/opt/sequoiadb/backup",
                                   "Name":"backupName", "Description":description,
                                   "EnsureInc":True, "OverWrite":True }
                                 See Info as below.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        Info:
           GroupName   :  The replica groups which to be backuped, if not assign, default all replica groups.
           Path        :  The backup path, if not assign, use the backup path assigned in configuration file.
           Name        :  The name for the backup.
           Description :  The description for the backup.
           EnsureInc   :  Whether execute increment synchronization,
                                default to be False.
           OverWrite   :  Whether overwrite the old backup file,
                                default to be False.
        """
        bson_options = None
        if options is not None:
            if not isinstance(options, dict):
                raise SDBTypeError("options must be an instance of dict")
            bson_options = bson.BSON.encode(options)

        rc = sdb.sdb_backup_offline(self._client, bson_options)
        raise_if_error(rc, "Failed to backup")

    def backup_offline(self, options = None):
        """
        Backup the whole database or specified replica group.
        Knowing the specific params to function backup
        """
        self.backup(options)

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

        async_flag = 0
        if isinstance(is_async, bool):
            if is_async:
                async_flag = 1
        else:
            raise SDBTypeError("size of tasks must be an instance of int")

        rc = sdb.sdb_cancel_task(self._client, task_id, async_flag)
        raise_if_error(rc, "Failed to cancel task")

    def set_session_attri(self, options):
        """Set the attributes of the session.

        Parameters:
           Name         Type     Info:
           options      dict     The options for setting session attributes. Can not be None.
                                         While it's a empty options, the local session attributes cache
                                         will be cleanup. Please visit this url:
                                         "http://doc.sequoiadb.com/cn/SequoiaDB-cat_id-1432190808-edition_id-@SDB_SYMBOL_VERSION"
                                         to get more details.
        Exceptions:
           pysequoiadb.error.SDBBaseError

        """
        if not isinstance(options, dict):
            raise SDBTypeError("options must be an instance of dict")
        bson_options = bson.BSON.encode(options)

        rc = sdb.sdb_set_session_attri(self._client, bson_options)
        raise_if_error(rc, "Failed to set session attribute")

    def get_session_attri(self, useCache=True):
        """Get the attributes of the current session.

        Parameters:
           Name         Type     Info:
           useCache     bool     Whether to use cache in local, default to be TRUE.

        Return values:
           a dict object of attributes
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(useCache, bool):
            raise SDBTypeError("useCache must be an instance of bool")
        
        rc, result = sdb.sdb_get_session_attri(self._client, useCache)
        raise_if_error(rc, "Failed to get session attributes")
        record, size = bson._bson_to_dict(result, dict, False,
                                          bson.OLD_UUID_SUBTYPE, True)
        return record

    def interrupt_operation(self):
        """Send "INTERRUPT_SELF" message to engine to stop the current operation. When the current operation had finish,
        nothing happened, Otherwise, the current operation will be stop, and return error.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.sdb_interrupt_operation(self._client)
        raise_if_error(rc, "Failed to interrupt")

    def interrupt(self):
        """Send a "Interrupt" message to engine, as a result, all the cursors and lobs created by
        current connection will be closed.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        rc = sdb.sdb_interrupt(self._client)
        raise_if_error(rc, "Failed to interrupt")

    def close_all_cursors(self):
        """Send a "Interrupt" message to engine, as a result, all the cursors and lobs created by
         current connection will be closed.

        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        self.interrupt()

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
                                           eg: { "Groups": [ "group1", "group2", "group3" ], "AutoSplit": True }
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

    def invalidate_cache(self, options=None):
        """Clean up cache in nodes(Data/Coordinator nodes).

        Parameters:
            Name         Type     Info:
            options      dict     The control options:(Only take effect in coordinate nodes).
                                    About the parameter 'options', please reference to the official
                                    website(www.sequoiadb.com) and then search "命令位置参数"
                                    for more details. Some of its optional parameters are as bellow:
                                  
                                    Global(Bool): execute this command in global or not. While 'options' is null, it's equals to {Global: true}.
                                    GroupID(INT32 or INT32 Array): specified one or several groups by their group IDs. e.g. {GroupID:[1001, 1002]}.
                                    GroupName(String or String Array): specified one or several groups by their group names. e.g. {GroupID:"group1"}.
                                    ...
                
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        bson_options = None
        if options is not None:
            if not isinstance(options, dict):
                raise SDBTypeError("options must be an instance of dict")
            bson_options = bson.BSON.encode(options)

        rc = sdb.sdb_invalidate_cache(self._client, bson_options)
        raise_if_error(rc, "Failed to invalidate cache")

    def force_session(self, session_id, options=None):
        """Terminate current operation of the specified session.

        Parameters:
            Name         Type     Info
            session_id   int/long The id of session whose current operation is to be terminated.
            options      dict     Command location parameters.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        bson_options = None
        if options is not None:
            if not isinstance(options, dict):
                raise SDBTypeError("options must be an instance of dict")
            bson_options = bson.BSON.encode(options)
        if not isinstance(session_id, int) and not isinstance(session_id, long_type):
            raise SDBTypeError("session_id must be an instance of int or long")
        rc = sdb.sdb_force_session(self._client, session_id, bson_options)
        raise_if_error(rc, "Failed to force session[%d] in %s" % (session_id, str(options)))

    def reload_config(self, options=None):
        """Reload configurations.

        Parameters:
            Name         Type     Info
            options      dict     Command location parameters.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        bson_options = None
        if options is not None:
            if not isinstance(options, dict):
                raise SDBTypeError("options must be an instance of dict")
            bson_options = bson.BSON.encode(options)

        rc = sdb.sdb_reload_config(self._client, bson_options)
        raise_if_error(rc, "Failed to reload config")

    def set_pdlevel(self, level, options=None):
        """Set PD log level of node.

        Parameters:
            Name         Type     Info
            level        int      PD log level, the value can be 0~5.
                                  0: SEVERE
                                  1: ERROR
                                  2: EVENT
                                  3: WARNING
                                  4: INFO
                                  5: DEBUG
            options      dict     Command location parameters.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        bson_options = None
        if options is not None:
            if not isinstance(options, dict):
                raise SDBTypeError("options must be an instance of dict")
            bson_options = bson.BSON.encode(options)
        if not isinstance(level, int) or not (0 <= level <= 5):
            raise SDBTypeError("session_id must be an instance of int and in the range [0, 5]")
        rc = sdb.sdb_set_pdlevel(self._client, level, bson_options)
        raise_if_error(rc, "Failed to set pd level[%d] in %s" % (level, str(options)))

    def force_stepup(self, options=None):
        """Force a slave node to be master.

        Parameters:
            Name         Type     Info
            options      dict     The control parameters:
                                  Seconds: (Type: int) Duration to be master. Default is 120.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        bson_options = None
        if options is not None:
            if not isinstance(options, dict):
                raise SDBTypeError("options must be an instance of dict")
            bson_options = bson.BSON.encode(options)

        rc = sdb.sdb_force_stepup(self._client, bson_options)
        raise_if_error(rc, "Failed to force step up")

    def create_sequence(self, sequence_name, options=None):
        """Create the sequence with specified options.

        Parameters:
            Name             Type    Info
            sequence_name    str     The name of sequence.
            options          dict    The options for create sequence:
            - StartValue     int     The start value of sequence
            - MinValue       int     The minimum value of sequence
            - MaxValue       int     The maxmun value of sequence
            - Increment      int     The increment value of sequence
            - CacheSize      int     The cache size of sequence
            - AcquireSize    int     The acquire size of sequence
            - Cycled         bool    The cycled flag of sequence
        Return values:
           A sequence object
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(sequence_name, str_type):
            raise SDBTypeError("sequence name must be an instance of str")

        bson_options = None
        if options is not None:
            if not isinstance(options, dict):
                raise SDBTypeError("options must be an instance of dict")
            bson_options = bson.BSON.encode(options)

        sequence_obj = sequence()
        try:
            if bson_options is None:
                rc = sdb.sdb_create_sequence(self._client, sequence_name, sequence_obj._seq)
            else:
                rc = sdb.sdb_create_sequence_use_opt(self._client, sequence_name,
                                                     bson_options, sequence_obj._seq)
            raise_if_error(rc, "Failed to create sequence: %s" % sequence_name)
        except SDBBaseError:
            del sequence_obj
            raise
        return sequence_obj

    def drop_sequence(self, sequence_name):
        """Drop the specified sequence.

        Parameters:
            Name             Type     Info
            sequence_name    str      The name of sequence.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(sequence_name, str_type):
            raise SDBTypeError("sequence name must be an instance of str")

        rc = sdb.sdb_drop_sequence(self._client, sequence_name)
        raise_if_error(rc, "Failed to drop sequence: %s" % sequence_name)

    def get_sequence(self, sequence_name):
        """Get the named sequence.

        Parameters:
            Name             Type     Info
            sequence_name    str      The name of sequence.
        Return values:
           A sequence object
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(sequence_name, str_type):
            raise SDBTypeError("sequence name must be an instance of str")

        sequence_obj = sequence()
        try:
            rc = sdb.sdb_get_sequence(self._client, sequence_name, sequence_obj._seq)
            raise_if_error(rc, "Failed to get sequence: %s" % sequence_name)
        except SDBBaseError:
            del sequence_obj
            raise
        return sequence_obj

    def rename_sequence(self, old_name, new_name):
        """Rename sequence.

        Parameters:
            Name        Type     Info
            old_name    str      The old name of sequence.
            new_name    str      The new name of sequence.
        Exceptions:
           pysequoiadb.error.SDBBaseError
        """
        if not isinstance(old_name, str_type) :
            raise SDBTypeError("Old sequence name must be an instance of str")
        if not isinstance(new_name, str_type) :
            raise SDBTypeError("New sequence name must be an instance of str")

        rc = sdb.sdb_rename_sequence(self._client, old_name, new_name)
        raise_if_error(rc, "Failed to rename sequence, old name: %s, new name: %s" %(old_name, new_name))
