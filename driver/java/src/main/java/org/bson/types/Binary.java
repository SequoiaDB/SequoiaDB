// Binary.java

/**
 *  See the NOTICE.txt file distributed with this work for
 *  information regarding copyright ownership.
 *
 *  The authors license this file to you under the
 *  Apache License, Version 2.0 (the "License"); you may not use
 *  this file except in compliance with the License.  You may
 *  obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

package org.bson.types;

import org.bson.BSON;
import org.bson.util.JSON;

import java.io.Serializable;
import java.util.Arrays;

/**
 generic binary holder
 */
public class Binary implements Serializable {
    private static final long serialVersionUID = 7902997490338209467L;

    private final byte type;
    private final byte[] data;

    /**
     * Creates a Binary object with the default binary type of 0
     * @param data raw data
     */
    public Binary(byte[] data) {
        this(BSON.B_GENERAL, data);
    }

    /**
     * Creates a Binary object
     * @param type type of the field as encoded in BSON
     * @param data raw data
     */
    public Binary(byte type, byte[] data) {
        this.type = type;
        this.data = data;
    }

    /**
     * @return the type of binary data.
     */
    public byte getType() {
        return type;
    }

    /**
     * @return the binary data.
     */
    public byte[] getData() {
        return data;
    }

    /**
     * @return the length of binary data.
     */
    public int length() {
        return data.length;
    }

    @Override
    public boolean equals(final Object obj) {
        if (this == obj) {
            return true;
        }

        if (obj == null || !(obj instanceof Binary)) {
            return false;
        }

        Binary other = (Binary) obj;
        return type == other.type && Arrays.equals(data, other.data);
    }

    @Override
    public int hashCode() {
        int result = (int) type;
        result = 31 * result + Arrays.hashCode(data);
        return result;
    }

    @Override
    public String toString() {
        return JSON.serialize(this);
    }
}
