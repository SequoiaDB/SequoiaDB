// JSONCallback.java

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

import java.io.IOException;
import java.time.*;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeParseException;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.UUID;
import java.util.regex.Pattern;

import org.bson.BSON;
import org.bson.BSONObject;
import org.bson.BasicBSONCallback;
import org.bson.types.*;

import sun.misc.BASE64Decoder;

public class JSONCallback extends BasicBSONCallback {


	//@Override
	public void objectStart(boolean array) {
		_stackIsArrayType.addLast(array);
		super.objectStart(array);
	}
	
	@Override
	public void objectStart(boolean array, String name) {
		_stackIsArrayType.addLast(array);
		super.objectStart(array, name);
	}

	@Override
	public Object objectDone() {
		String name = curName();
		Object o = super.objectDone();
		BSONObject b = (BSONObject) o;
		
		if (_stackIsArrayType.size() <= 0) { 
			throw new IllegalStateException("something is wrong in the stack of nesting object type");
		}
		Boolean isArrayType = _stackIsArrayType.removeLast();

		// override the object if it's a special type
		if (!isArrayType) {
			if (b.containsField("$oid")) {
				o = new ObjectId((String) b.get("$oid"));
				if (!isStackEmpty()) {
					gotObjectId(name, (ObjectId) o);
				} else {
					setRoot(o);
				}
			} else if (b.containsField("$numberLong")) {
				o = Long.valueOf((String) b.get("$numberLong"));
				if (!isStackEmpty()) {
					gotLong(name, (Long) o);
				} else {
					setRoot(o);
				}
			} else if (b.containsField("$date")) {
				Object dateValue = b.get("$date");
				BSONDate date = null;
				if (dateValue instanceof Number) {
					date = new BSONDate( ((Number) dateValue).longValue() );
				} else {
					// case 1: string to UTC time, format:
					// yyyy-MM-dd'T'HH:mm:ss.SSS'Z'
					// yyyy-MM-dd'T'HH:mm:ss'Z'
					try {
						Instant instant = Instant.parse( dateValue.toString() );
						date = BSONDate.from( instant );
					} catch ( DateTimeParseException e ) {
						// ignore
					}

					// case 2: string to local time, format: yyyy-MM-dd
					try {
						if ( date == null ) {
							DateTimeFormatter formatter = DateTimeFormatter.ofPattern( "yyyy-MM-dd" );
							LocalDate localDate = LocalDate.parse( dateValue.toString(), formatter );
							date = BSONDate.valueOf( localDate );
						}
					} catch ( DateTimeParseException e ) {
						throw new JSONParseException( "Invalid date format: " + dateValue.toString(), 0, e );
					}
				}
				if (!isStackEmpty()) {
					cur().put(name, date);
				} else {
					setRoot(date);
				}
			} else if (b.containsField("$timestamp")) {
				Object timeStamp = b.get("$timestamp");
				if (timeStamp instanceof String) {
					String strTimeStamp = (String) timeStamp;
					String dateStr = strTimeStamp.substring(0,
							strTimeStamp.lastIndexOf('.'));
					String incStr = strTimeStamp.substring(strTimeStamp
							.lastIndexOf('.') + 1);

					Instant instant = null;
					try {
						DateTimeFormatter formatter = DateTimeFormatter.ofPattern( "yyyy-MM-dd-HH.mm.ss" );
						LocalDateTime localDateTime = LocalDateTime.parse( dateStr, formatter );
						instant = localDateTime.atZone( ZoneId.systemDefault() ).toInstant();
					} catch ( DateTimeParseException e ) {
						throw new JSONParseException(dateStr, dateStr.length() - 1, e);
					}
					o = new BSONTimestamp((int) (instant.getEpochSecond()),
							Integer.parseInt(incStr));

					if (!isStackEmpty()) {
						cur().put(name, o);
					} else {
						setRoot(o);
					}
				}
			} else if (b.containsField("$decimal")) {
				Object decimal = b.get("$decimal");
				if (decimal instanceof String) {
					String str = (String)decimal;
					int precision = -1;
					int scale = -1;
					Object arr = b.get("$precision");
					if (arr instanceof ArrayList) {
						// get precision
						//@SuppressWarnings("rawtypes")
						Object ret = ((ArrayList)arr).get(0);
						if (ret instanceof Integer) {
							precision = (Integer)ret;
						} else {
							precision = Integer.parseInt(((String)ret));
						}
						// get scale
						ret = ((ArrayList)arr).get(1);
						if (ret instanceof Integer) {
							scale = (Integer)ret;
						} else {
							scale = Integer.parseInt(((String)ret));
						}
					}
					o = new BSONDecimal(str, precision, scale);
					if (!isStackEmpty()) {
						cur().put(name, o);
					} else {
						setRoot(o);
					}
				}
			} else if (b.containsField("$regex")) {
				o = Pattern.compile((String) b.get("$regex"),
						BSON.regexFlags((String) b.get("$options")));
				if (!isStackEmpty()) {
					cur().put(name, o);
				} else {
					setRoot(o);
				}
			} else if (b.containsField("$ts")) {
				Long ts = ((Number) b.get("$ts")).longValue();
				Long inc = ((Number) b.get("$inc")).longValue();
				o = new BSONTimestamp(ts.intValue(), inc.intValue());
				if (!isStackEmpty()) {
					cur().put(name, o);
				} else {
					setRoot(o);
				}
			} else if (b.containsField("$minKey")) {
				o = new MinKey();
				if (!isStackEmpty()) {
					cur().put(name, o);
				} else {
					setRoot(o);
				}
			} else if (b.containsField("$maxKey")) {
				o = new MaxKey();
				if (!isStackEmpty()) {
					cur().put(name, o);
				} else {
					setRoot(o);
				}
			} else if (b.containsField("$uuid")) {
				o = UUID.fromString((String) b.get("$uuid"));
				if (!isStackEmpty()) {
					cur().put(name, o);
				} else {
					setRoot(o);
				}
			} else if (b.containsField("$binary")) {
				byte type = BSON.B_GENERAL;
				if (b.containsField("$type")) {
					Object oType = b.get("$type");
					if (oType instanceof Integer) {
						Integer iType = (Integer) b.get("$type");
						type = iType.byteValue();
					} else {
						String strType = (String) b.get("$type");
						type = (byte) Integer.parseInt(strType);
					}
				}
				String encodeString = (String) b.get("$binary");
				if (encodeString.length() % 4 != 0) {
					throw new JSONParseException(encodeString, encodeString.length() - 1,
							"invalid size of base64 encode");
				}
				try {
					BASE64Decoder decode = new BASE64Decoder();
					byte[] data = decode.decodeBuffer(encodeString);
					o = new Binary(type, data);
					if (!isStackEmpty()) {
						cur().put(name, o);
					} else {
						setRoot(o);
					}
				} catch (IOException e) {
					throw new JSONParseException(encodeString, encodeString.length() - 1, e);
				}
			}
		}
		return o;
	}

	// true for the nesting object is array, false for not 
	// we should note that, when we use the inherited "objectStart" and "objectDone", we
	// should handle this stack
	private final LinkedList<Boolean> _stackIsArrayType = new LinkedList<Boolean>();
}
