/**
 *      Copyright (C) 2008 10gen Inc.
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

/**
 * Exception throw when invalid JSON is passed to JSONParser.
 * 
 * This exception creates a message that points to the first 
 * offending character in the JSON string:
 * <pre>
 * { "x" : 3, "y" : 4, some invalid json.... }
 *                     ^
 * </pre>
 */
public class JSONParseException extends RuntimeException { 

    private static final long serialVersionUID = -4415279469780082174L;

    String s;
    int pos;
    String detail;

    public String getMessage() {
        int count = pos;
        StringBuilder sb = new StringBuilder();
        sb.append("\n");
        if (detail != null && !detail.isEmpty()) {
            sb.append(detail);
            sb.append(", ");
            count += detail.length() +  2;
        }
        sb.append(s);
        sb.append("\n");
        for(int i=0; i<count; i++) {
            sb.append(" ");
        }
        sb.append("^");
        return sb.toString();
    }

    public JSONParseException(String s, int pos) {
        this.s = s;
        this.pos = pos;
    }
    
    public JSONParseException(String s, int pos, Throwable cause) {
    	super(cause);
        this.s = s;
        this.pos = pos;
    }

    public JSONParseException(String s, int pos, String detail) {
        this.s = s;
        this.pos = pos;
        this.detail = detail;
    }
}