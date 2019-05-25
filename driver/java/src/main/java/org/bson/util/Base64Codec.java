
/**
 *      Copyright (C) 2012 10gen Inc.
 *  
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

package org.bson.util;

/*
 *      Copyright (C) 2012 10gen Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

/**
 * Provides Base64 encoding and decoding </a>.
 * 
 * <p>
 * This class implements Base64 encoding
 * 
 * Thanks to Apache Commons project. This class refactored from org.apache.commons.codec.binary 
 * 
 * Original Thanks to "commons" project in ws.apache.org for this code.
 * http://svn.apache.org/repos/asf/webservices/commons/trunk/modules/util/
 * </p>
 * 
 */
public class Base64Codec {
    
    private static int BYTES_PER_UNENCODED_BLOCK = 3;
    private static int BYTES_PER_ENCODED_BLOCK = 4;
    
    /** Mask used to extract 6 bits, used when encoding */
    private static final int SixBitMask = 0x3f; 
    
    /** padding char */
    private static final byte PAD = '=';
    
    /**
     * This array is a lookup table that translates 6-bit positive integer index values into their "Base64 Alphabet"
     * equivalents as specified in Table 1 of RFC 2045.
     *
     */
    private static final byte[] EncodeTable = { 'A', 'B', 'C', 'D', 'E', 'F',
            'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
            'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
            'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
            't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5',
            '6', '7', '8', '9', '+', '/' };
    
    
    public String encode(byte[] in) {
        
        int modulus = 0;
        int bitWorkArea = 0;
        int numEncodedBytes = (in.length/BYTES_PER_UNENCODED_BLOCK)*BYTES_PER_ENCODED_BLOCK 
                + ((in.length%BYTES_PER_UNENCODED_BLOCK == 0 )?0:4);

        byte[] buffer = new byte[numEncodedBytes]; 
        int pos = 0;
        
        for (int i = 0; i < in.length; i++) {
            modulus = (modulus+1) % BYTES_PER_UNENCODED_BLOCK;
            int b = in[i];
            
            if (b < 0) 
                b += 256;
            
            bitWorkArea = (bitWorkArea << 8) + b; //  BITS_PER_BYTE
            if (0 == modulus) { // 3 bytes = 24 bits = 4 * 6 bits to extract
                buffer[pos++] = EncodeTable[(bitWorkArea >> 18) & SixBitMask];
                buffer[pos++] = EncodeTable[(bitWorkArea >> 12) & SixBitMask];
                buffer[pos++] = EncodeTable[(bitWorkArea >> 6) & SixBitMask];
                buffer[pos++] = EncodeTable[bitWorkArea & SixBitMask];
            }
        }
        
        switch (modulus) { // 0-2
        case 1 : // 8 bits = 6 + 2
            buffer[pos++] = EncodeTable[(bitWorkArea >> 2) & SixBitMask]; // top 6 bits
            buffer[pos++] = EncodeTable[(bitWorkArea << 4) & SixBitMask]; // remaining 2 
            buffer[pos++] = PAD;
            buffer[pos++] = PAD;
            break;

        case 2 : // 16 bits = 6 + 6 + 4
            buffer[pos++] = EncodeTable[(bitWorkArea >> 10) & SixBitMask];
            buffer[pos++] = EncodeTable[(bitWorkArea >> 4) & SixBitMask];
            buffer[pos++] = EncodeTable[(bitWorkArea << 2) & SixBitMask];
            buffer[pos++] = PAD;
            break;
        }
        
        return new String(buffer);
    }
}
