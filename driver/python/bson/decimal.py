#   Copyright (C) 2012-2016 SequoiaDB Ltd.
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

"""decimal supported for Python driver of SequoiaDB
"""
from bson.py3compat import (long_type)
from bson.errors import InvalidDecimal

try:
    from . import bsondecimal as decimal
except:
    raise Exception("failed to import extension: decimal")


class Decimal(object):
    """Decimal type for SequoiaDB
    decimal is an type of element of bson, for the seak of double(IEEE)
    usage:

    >>> import bson
    >>> from bson import Decimal
    >>> obj =  Decimal("12345.6789098765", 1000, 100) # precision 1000, scale 100

    # use a decimal into bson as blow:
    >>> doc = { 'rest':obj }

    >>> print (doc)
    { "$decimal": "12345.6789098765", "$precision": [1000, 10] }

    >>> doc = { "decimal": doc }

    when a doc with decimal element encoded to an bson obj, it can be used to insert into SequoiaDB
    and it also can be decoded into an decimal object from bson object

    >>> bobj = bson.BSON.encode(doc)
    >>> dd = bson.BSON.decode(bobj)
    >>> v = dd['decimal']
    >>> v
    { "$decimal": "12345.6789098765", "$precision": [1000, 10] }
    >>> type(v)
    <class 'bson.decimal.Decimal'>

    methods can be listed by using dir(obj)
    """

    def __init__(self, value, precision=None, scale=None):
        """ create an decimal object, precision and scale are 0 by default
        and precision is limited under 1000
        """
        if (precision is None and scale is not None) or (precision is not None and scale is None):
            raise TypeError("precision and scale should be set both or neither")
        if precision is not None and not isinstance(precision, int):
            raise TypeError("precision must be an instance of int")
        if scale is not None and not isinstance(scale, int):
            raise TypeError("scale must be an instance of int")

        _, self.__decimal = decimal.create()
        if _ != 0:
            raise InvalidDecimal("cannot create decimal entity, out of memory")

        if precision is None and scale is None:
            _ = decimal.init(self.__decimal)
        else:
            _ = decimal.init2(self.__decimal, precision, scale)
        if 0 != _:
            raise InvalidDecimal("invalid parameter, code: %d" % _)

        self.__parse(value)

    def __del__(self):
        decimal.destroy(self.__decimal)

    def __str__(self):
        return self.__to_json_string()

    def __repr__(self):
        return self.__to_json_string()

    def __eq__(self, other):
        if isinstance(other, Decimal) or \
                isinstance(other, long_type) or \
                isinstance(other, int) or \
                isinstance(other, float):
            return 0 == self.compare(other)
        else:
            return False

    def __ne__(self, other):
        return not self.__eq__(other)

    def set_zero(self):
        """set the decimal object as an instance initalized by 0
        """
        _ = decimal.setZero(self.__decimal)
        if 0 != _:
            raise InvalidDecimal("invalid parameter, code: %d" % _)

    def is_zero(self):
        """charge the value of decimal is zero ir not
        """
        _, zero_ = decimal.isZero(self.__decimal)
        if _ != 0:
            raise InvalidDecimal("invalid parameter, code: %d" % _)
        return True if zero_ != 0 else False

    def set_min(self):
        """set the value of decimal is the min value
        """
        _ = decimal.setMin(self.__decimal)
        if _ != 0:
            raise InvalidDecimal("invalid parameter, code: %d" % _)

    def is_min(self):
        """charge the value of decimal is min value or not
        """
        _, min_ = decimal.isMin(self.__decimal)
        if _ != 0:
            raise InvalidDecimal("invalid parameter, code: %d" % _)
        return True if min_ != 0 else False

    def set_max(self):
        """set the value of decimal is the max value
        """
        _ = decimal.setMax(self.__decimal)
        if _ != 0:
            raise InvalidDecimal("invalid parameter, code: %d" % _)

    def is_max(self):
        """charge the value of decimal is min value or not
        """
        _, max_ = decimal.isMax(self.__decimal)
        if _ != 0:
            raise InvalidDecimal("invalid parameter, code: %d" % _)
        return True if max_ != 0 else False

    def __from_int(self, value):
        _ = decimal.fromInt(self.__decimal, value)
        if _ != 0:
            raise InvalidDecimal("invalid parameter, code: %d" % _)

    def to_int(self):
        """force the decimal to be an int(long), and show it regularized by scale
        """
        _, v = decimal.toInt(self.__decimal)
        if _ != 0:
            raise InvalidDecimal("invalid parameter, code: %d" % _)
        return v

    def __from_float(self, value):
        _ = decimal.fromFloat(self.__decimal, value)
        if _ != 0:
            raise InvalidDecimal("invalid parameter, code: %d" % _)

    def to_float(self):
        """force the decimal to be an float(double is supported), and show it regularized by scale
        """
        _, v = decimal.toFloat(self.__decimal)
        if _ != 0:
            raise InvalidDecimal("invalid parameter, code: %d" % _)
        return v

    def __from_string(self, value):
        _ = decimal.fromString(self.__decimal, value)
        if _ != 0:
            raise InvalidDecimal("invalid parameter, code: %d" % _)

    def to_string(self):
        """force the decimal to be an string, and show it regularized by scale
        """
        _, v = decimal.toString(self.__decimal)
        if _ != 0:
            raise InvalidDecimal("invalid parameter, code: %d" % _)
        return v

    def __parse(self, value):
        """set the value of decimal, only int(long)/float(double)/str is accepted
        """
        if isinstance(value, long_type):
            return self.__from_string(str(value))
        elif isinstance(value, int):
            return self.__from_int(value)
        elif isinstance(value, float):
            return self.__from_float(value)
        elif isinstance(value, str):
            return self.__from_string(value)
        else:
            raise TypeError("invalid value to parse")

    def __to_json_string(self):
        _, v = decimal.toJsonString(self.__decimal)
        if _ != 0:
            raise InvalidDecimal("invalid parameter, code: %d" % _)
        return v

    def compare(self, rhs):
        """compare between two decimal object.
        if int is specified, int value will be converted to an decimal object, then compare
        """
        if isinstance(rhs, long_type):
            _ = self.compare(Decimal(rhs))
        elif isinstance(rhs, int):
            _ = decimal.compareInt(self.__decimal, rhs)
        elif isinstance(rhs, float):
            _ = self.compare(Decimal(rhs))
        elif isinstance(rhs, Decimal):
            _ = decimal.compare(self.__decimal, rhs.__decimal)
        else:
            raise TypeError('invalid comparision between Decimal and %s' % type(rhs))

        if _ not in (-1, 0, 1):
            raise InvalidDecimal("invalid return value or process failed, code %d" % _)
        return _

    def _from_bson_element_value(self, value):
        """the method is used by bson.BSON.decode to decode the binary string(an bson element value) into an decimal value
        """
        _, l = decimal.fromBsonValue(self.__decimal, value)
        if _ != 0:
            raise InvalidDecimal("invalid parameter, code: %d" % _)
        return l

    def _to_bson_element_value(self):
        """the method is used to encode an decimal object into the value of bson element
        """
        _, s = decimal.toBsonElement(self.__decimal)
        if _ != 0:
            raise InvalidDecimal("invalid parameter, code: %d" % _)
        return s
