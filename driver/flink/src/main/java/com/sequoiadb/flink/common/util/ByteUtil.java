/*
 * Copyright 2022 SequoiaDB Inc.
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

package com.sequoiadb.flink.common.util;

public class ByteUtil {

    public static byte[] toBytes(boolean data) {
        return data ? new byte[]{1} : new byte[]{0};
    }

    // 16-bit integer
    public static byte[] toBytes(short data) {
        byte b1 = (byte) (data & 0xff);
        byte b2 = (byte) ((data >> 8) & 0xff);
        return new byte[]{b1, b2};
    }

    // 32-bit integer
    public static byte[] toBytes(int data) {
        byte b1 = (byte) (data & 0xff);
        byte b2 = (byte) ((data >> 8) & 0xff);
        byte b3 = (byte) ((data >> 16) & 0xff);
        byte b4 = (byte) ((data >> 24) & 0xff);
        return new byte[]{b1, b2, b3, b4};
    }

    public static byte[] toBytes(long data) {
        byte b1 = (byte) (data & 0xff);
        byte b2 = (byte) ((data >> 8) & 0xff);
        byte b3 = (byte) ((data >> 16) & 0xff);
        byte b4 = (byte) ((data >> 24) & 0xff);
        byte b5 = (byte) ((data >> 32) & 0xff);
        byte b6 = (byte) ((data >> 40) & 0xff);
        byte b7 = (byte) ((data >> 48) & 0xff);
        byte b8 = (byte) ((data >> 56) & 0xff);
        return new byte[]{b1, b2, b3, b4, b5, b6, b7, b8};
    }

    public static byte[] toBytes(float data) {
        return toBytes(Float.floatToIntBits(data));
    }

    public static byte[] toBytes(double data) {
        return toBytes(Double.doubleToLongBits(data));
    }

}
