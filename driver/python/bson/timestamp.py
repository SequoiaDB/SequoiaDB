# Copyright 2010-2014 MongoDB, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Tools for representing MongoDB internal Timestamps.
"""

import calendar
import datetime

from bson.py3compat import long_type
from bson.tz_util import utc

UPPERBOUND = 2147483648
LOWERBOUND = -2147483648


class Timestamp(object):
    """Timestamp of bson
    """

    _type_marker = 17

    def __init__(self, time, inc):
        """Create a new :class:`Timestamp`.

        Raises :class:`TypeError` if `time` is not an instance of
        :class: `int` or :class:`~datetime.datetime`, or `inc` is not
        an instance of :class:`int`. Raises :class:`ValueError` if
        `time` or `inc` is not in [-2**31, 2**31).

        :Parameters:
          - `time`: time in seconds since epoch UTC, or a naive UTC
            :class:`~datetime.datetime`, or an aware
            :class:`~datetime.datetime`
          - `inc`: the incrementing counter

        .. versionchanged:: 1.7
           `time` can now be a :class:`~datetime.datetime` instance.
        """
        if isinstance(time, datetime.datetime):
            if time.utcoffset() is not None:
                time = time - time.utcoffset()
            time = int(calendar.timegm(time.timetuple()))
        if not isinstance(time, (int, long_type)):
            raise TypeError("time must be an instance of int")
        if not isinstance(inc, (int, long_type)):
            raise TypeError("inc must be an instance of int")
        if not LOWERBOUND <= time < UPPERBOUND:
            raise ValueError("time must be contained in [-2**31, 2**31): " + str(time))
        if not LOWERBOUND <= inc < UPPERBOUND:
            raise ValueError("inc must be contained in [-2**31, 2**31): " + str(inc))
        if not 0 <= inc < 1000000:
            secs = inc // 1000000
            inc = inc % 1000000
            time = time + secs
            if not LOWERBOUND <= time < UPPERBOUND:
                raise ValueError("time is overflowed by inc")

        self.__time = time
        self.__inc = inc

    @property
    def time(self):
        """Get the time portion of this :class:`Timestamp`.
        """
        return self.__time

    @property
    def inc(self):
        """Get the inc portion of this :class:`Timestamp`.
        """
        return self.__inc

    def __eq__(self, other):
        if isinstance(other, Timestamp):
            return (self.__time == other.time and self.__inc == other.inc)
        else:
            return NotImplemented

    def __ne__(self, other):
        return not self == other

    def __lt__(self, other):
        if isinstance(other, Timestamp):
            return (self.time, self.inc) < (other.time, other.inc)
        return NotImplemented

    def __le__(self, other):
        if isinstance(other, Timestamp):
            return (self.time, self.inc) <= (other.time, other.inc)
        return NotImplemented

    def __gt__(self, other):
        if isinstance(other, Timestamp):
            return (self.time, self.inc) > (other.time, other.inc)
        return NotImplemented

    def __ge__(self, other):
        if isinstance(other, Timestamp):
            return (self.time, self.inc) >= (other.time, other.inc)
        return NotImplemented

    def __repr__(self):
        return "Timestamp(%s, %s)" % (self.__time, self.__inc)

    def as_datetime(self):
        """Return a :class:`~datetime.datetime` instance corresponding
        to the time portion of this :class:`Timestamp`.

        .. versionchanged:: 1.8
           The returned datetime is now timezone aware.
        """
        return datetime.datetime.fromtimestamp(self.__time, utc)
