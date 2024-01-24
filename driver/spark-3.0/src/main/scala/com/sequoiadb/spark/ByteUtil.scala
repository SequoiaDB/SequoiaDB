/*
 * Copyright 2017 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

package com.sequoiadb.spark

import java.nio.charset.Charset

private[spark] object ByteUtil {

    def getBytes(data: Boolean): Array[Byte] = {
        if (data) {
            Array[Byte](1)
        } else {
            Array[Byte](0)
        }
    }

    def getBytes(data: Short): Array[Byte] = {
        Array[Byte](
            (data & 0xff).toByte,
            ((data & 0xff00) >> 8).toByte)
    }

    def getBytes(data: Char): Array[Byte] = {
        Array[Byte]((data & 0xff).toByte)
    }

    def getBytes(data: Int): Array[Byte] = {
        Array[Byte](
            (data & 0xff).toByte,
            ((data & 0xff00) >> 8).toByte,
            ((data & 0xff0000) >> 16).toByte,
            ((data & 0xff000000) >> 24).toByte)
    }

    def getBytes(data: Long): Array[Byte] = {
        Array[Byte](
            (data & 0xff).toByte,
            ((data >> 8) & 0xff).toByte,
            ((data >> 16) & 0xff).toByte,
            ((data >> 24) & 0xff).toByte,
            ((data >> 32) & 0xff).toByte,
            ((data >> 40) & 0xff).toByte,
            ((data >> 48) & 0xff).toByte,
            ((data >> 56) & 0xff).toByte)
    }

    def getBytes(data: Float): Array[Byte] = {
        getBytes(java.lang.Float.floatToIntBits(data))
    }

    def getBytes(data: Double): Array[Byte] = {
        getBytes(java.lang.Double.doubleToLongBits(data))
    }

    def getBytes(data: String, charsetName: String = "UTF-8"): Array[Byte] = {
        data.getBytes(Charset.forName(charsetName))
    }
}