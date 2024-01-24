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

try:
    from . import sdb
except:
    raise Exception("Cannot find extension: sdb")

import bson
from bson.py3compat import (text_type, str_type)
from pysequoiadb.errcode import *


class SDBBaseError(Exception):
    """Base Exception for SequoiaDB
    """

    def __init__(self, code, detail=None, error_obj=None):
        if not isinstance(code, Errcode):
            raise TypeError("code should be Errcode type")
        if detail is not None and not isinstance(detail, (text_type, str_type)):
            raise TypeError("detail should be str type")
        if error_obj is not None and not isinstance(error_obj, dict):
            raise TypeError("error_obj should be dict type")
        self.__code = code
        self.__error_obj = error_obj
        if error_obj is not None:
            if "detail" in error_obj:
                server_detail = error_obj["detail"]
                if len(server_detail) > 0:
                    if detail is None or len(detail) == 0:
                        detail = server_detail
                    else:
                        detail += ", " + server_detail
                        error_obj["detail"] = detail
                else:
                    if detail is not None and len(detail) > 0:
                        error_obj["detail"] = detail
            else:
                if detail is not None and len(detail) > 0:
                    error_obj["detail"] = detail
        self.__detail = detail
        Exception.__init__(self, code.desc)

    def __repr__(self):
        return "%s: %s" % (self.__code, self.__detail)

    def __str__(self):
        if self.__detail is not None and self.__detail != "":
            return "%s(%d), %s, detail: %s" % \
                   (self.__code.name, self.__code.code, self.__code.desc, self.__detail)
        else:
            return "%s(%d), %s" % \
                   (self.__code.name, self.__code.code, self.__code.desc)

    @property
    def code(self):
        """The error code returned by the server, if any.
        """
        return self.__code.code

    @property
    def errcode(self):
        """Errcode of current error.
        """
        return self.__code

    @property
    def detail(self):
        """return the detail error message
        """
        return self.__detail

    @property
    def error_object(self):
        """Return the error object of last operation which type is dict.

        None if no error object. The error object contains the follow 3 fields:
           * errno : The error number.
           * description : The description of the errno.
           * detail : The error detail.
           When an error happen in the data nodes, the error object will also contains the follow field:
           * ErrNodes: The error detail of the data nodes.
        eg. This is a Redefine index error:
        {
            'errno': -247,
            'ErrNodes': [{ 'GroupName': 'datagroup', 'Flag': -247, 'NodeName': 'localhost:11820',
                                'ErrInfo': { 'errno': -247, 'description': 'Redefine index', 'detail': '' }}],
            'description': 'Redefine index',
            'detail': ''
        }

        Visit this url:
        "http://doc.sequoiadb.com/cn/sequoiadb-cat_id-1482317447-edition_id-@SDB_SYMBOL_VERSION"
        to get more details about the error object.
        """
        return self.__error_obj


class SDBTypeError(SDBBaseError):
    """Type Error of SequoiaDB
    """

    def __init__(self, detail, error_obj=None):
        SDBBaseError.__init__(self, SDB_INVALIDARG, detail, error_obj)


class SDBEndOfCursor(SDBBaseError):
    """End of cursor
    """

    def __init__(self):
        SDBBaseError.__init__(self, SDB_DMS_EOC, "end of cursor")


class SDBIOError(SDBBaseError):
    """IO Error of SequoiaDB
    """

    def __init__(self, code, detail, error_obj=None):
        SDBBaseError.__init__(self, code, detail, error_obj)


class SDBNetworkError(SDBBaseError):
    """Network Error of SequoiaDB
    """

    def __init__(self, code, detail, error_obj=None):
        SDBBaseError.__init__(self, code, detail, error_obj)


class SDBInvalidArgument(SDBBaseError):
    """Invalid Argument Error
    """

    def __init__(self, code, detail, error_obj=None):
        SDBBaseError.__init__(self, code, detail, error_obj)


class SDBSystemError(SDBBaseError):
    """System Error of SequoiaDB
    """

    def __init__(self, code, detail, error_obj=None):
        SDBBaseError.__init__(self, code, detail, error_obj)


class SDBUnknownError(SDBBaseError):
    """Unknown Error of SequoiaDB
    """

    def __init__(self, code, detail, error_obj=None):
        SDBBaseError.__init__(self, Errcode("SDB_UNKNOWN", code, "Unknown error"), detail, error_obj)


class SDBError(SDBBaseError):
    """General Error of SequoiaDB
    """

    def __init__(self, code, detail, error_obj=None):
        SDBBaseError.__init__(self, code, detail, error_obj)


io_error = [
    SDB_IO,
    SDB_FNE,
    SDB_FE,
    SDB_NOSPC
]

net_error = [
    SDB_NETWORK,
    SDB_NETWORK_CLOSE,
    SDB_NET_ALREADY_LISTENED,
    SDB_NET_CANNOT_LISTEN,
    SDB_NET_CANNOT_CONNECT,
    SDB_NET_NOT_CONNECT,
    SDB_NET_SEND_ERR,
    SDB_NET_TIMER_ID_NOT_FOUND,
    SDB_NET_ROUTE_NOT_FOUND,
    SDB_NET_BROKEN_MSG,
    SDB_NET_INVALID_HANDLE
]

invalid_error = [
    SDB_INVALIDARG,
    SDB_INVALIDSIZE,
    SDB_INVALIDPATH,
    SDB_INVALID_FILE_TYPE
]

system_error = [
    SDB_OOM,
    SDB_SYS
]


def raise_if_error(rc, detail):
    """Check return value, raise a SDBBaseError if error occurred.
    """
    if (not isinstance(rc, int)) and (not isinstance(rc, Errcode)):
        raise TypeError("rc should be int or Errcode type")
    if SDB_OK != rc:
        if isinstance(rc, Errcode):
            err_code = rc
        else:
            err_code = get_errcode(rc)

        rc, bson_string = sdb.sdb_get_last_error()
        error_obj = None
        if bson_string is not None:
            error_obj, size = bson._bson_to_dict(bson_string, dict, False, bson.OLD_UUID_SUBTYPE, True)
        sdb.sdb_clear_last_error()

        if err_code is None:
            raise SDBUnknownError(rc, detail, error_obj)

        if err_code == SDB_DMS_EOC:
            raise SDBEndOfCursor
        if err_code in io_error:
            raise SDBIOError(err_code, detail, error_obj)
        elif err_code in net_error:
            raise SDBNetworkError(err_code, detail, error_obj)
        elif err_code in invalid_error:
            raise SDBInvalidArgument(err_code, detail, error_obj)
        elif err_code in system_error:
            raise SDBSystemError(err_code, detail, error_obj)
        else:
            raise SDBError(err_code, detail, error_obj)
