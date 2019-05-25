
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

package org.bson;

import static org.bson.BSON.ARRAY;
import static org.bson.BSON.BINARY;
import static org.bson.BSON.BOOLEAN;
import static org.bson.BSON.B_BINARY;
import static org.bson.BSON.B_GENERAL;
import static org.bson.BSON.B_UUID;
import static org.bson.BSON.CODE;
import static org.bson.BSON.CODE_W_SCOPE;
import static org.bson.BSON.DATE;
import static org.bson.BSON.EOO;
import static org.bson.BSON.MAXKEY;
import static org.bson.BSON.MINKEY;
import static org.bson.BSON.NULL;
import static org.bson.BSON.NUMBER;
import static org.bson.BSON.NUMBER_INT;
import static org.bson.BSON.NUMBER_LONG;
import static org.bson.BSON.OBJECT;
import static org.bson.BSON.OID;
import static org.bson.BSON.REGEX;
import static org.bson.BSON.STRING;
import static org.bson.BSON.SYMBOL;
import static org.bson.BSON.TIMESTAMP;
import static org.bson.BSON.UNDEFINED;
import static org.bson.BSON.NUMBER_DECIMAL;
import static org.bson.BSON.regexFlags;

import java.lang.reflect.Array;
import java.math.BigDecimal;
import java.nio.Buffer;
import java.sql.Timestamp;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;
import java.util.regex.Pattern;

import org.bson.io.BasicOutputBuffer;
import org.bson.io.OutputBuffer;
import org.bson.types.BSONDecimal;
import org.bson.types.BSONTimestamp;
import org.bson.types.Binary;
import org.bson.types.Code;
import org.bson.types.CodeWScope;
import org.bson.types.MaxKey;
import org.bson.types.MinKey;
import org.bson.types.ObjectId;
import org.bson.types.Symbol;

/**
 * this is meant to be pooled or cached there is some per instance memory for
 * string conversion, etc...
 */
@SuppressWarnings("unchecked")
public class BasicBSONEncoder implements BSONEncoder {

	static final boolean DEBUG = false;
	
	public BasicBSONEncoder() {

	}

	public byte[] encode(BSONObject o) {
		BasicOutputBuffer buf = new BasicOutputBuffer();
		set(buf);
		putObject(o);
		done();
		return buf.toByteArray();
	}

	public void set(OutputBuffer out) {
		if (_buf != null) throw new IllegalStateException("in the middle of something");

		_buf = out;
	}

	public void done() {
		_buf = null;
	}

	/**
	 * @return true if object was handled
	 */
	protected boolean handleSpecialObjects(String name, BSONObject o) {
		return false;
	}

	protected boolean putSpecial(String name, Object o) {
		return false;
	}

	/**
	 * Encodes a <code>BSONObject</code>. This is for the higher level api calls
	 * 
	 * @param o
	 *            the object to encode
	 * @return the number of characters in the encoding
	 */
	public int putObject(BSONObject o) {
		return putObject(null, o);
	}

	/**
	 * this is really for embedded objects
	 */
	protected int putObject(String name, BSONObject o) {

		if (o == null) throw new NullPointerException("can't save a null object");

		if (DEBUG) System.out.println("putObject : " + name + " [" + o.getClass() + "]" + " # keys " + o.keySet().size());

		final int start = _buf.getPosition();

		byte myType = OBJECT;
		if (o instanceof List) myType = ARRAY;

		if (handleSpecialObjects(name, o)) return _buf.getPosition() - start;

		if (name != null) {
			_put(myType, name);
		}

		final int sizePos = _buf.getPosition();
		_buf.writeInt(0); // leaving space for this. set it at the end

		List transientFields = null;
		boolean rewriteID = myType == OBJECT && name == null;

		if (myType == OBJECT) {
			if (rewriteID && o.containsField("_id")) _putObjectField("_id", o.get("_id"));

			{
				Object temp = o.get("_transientFields");
				if (temp instanceof List) transientFields = (List) temp;
			}
		}

		if (o instanceof Map) {
			for (Entry<String, Object> e : ((Map<String, Object>) o).entrySet()) {

				if (rewriteID && e.getKey().equals("_id")) continue;

				if (transientFields != null && transientFields.contains(e.getKey())) continue;

				_putObjectField(e.getKey(), e.getValue());

			}
		} else {
			for (String s : o.keySet()) {

				if (rewriteID && s.equals("_id")) continue;

				if (transientFields != null && transientFields.contains(s)) continue;

				Object val = o.get(s);

				_putObjectField(s, val);

			}
		}
		_buf.write(EOO);

		_buf.writeInt(sizePos, _buf.getPosition() - sizePos);
		return _buf.getPosition() - start;
	}

	protected void _putObjectField(String name, Object val) {

		if (name.equals("_transientFields")) return;

		if (DEBUG) System.out.println("\t put thing : " + name);

		if (name.equals("$where") && val instanceof String) {
			_put(CODE, name);
			_putValueString(val.toString());
			return;
		}

		val = BSON.applyEncodingHooks(val);

		if (val == null)
			putNull(name);
		else if (val instanceof Timestamp)
			putTimestamp(name, new BSONTimestamp((Timestamp) val));
		else if (val instanceof Date)
			putDate(name, (Date) val);
		else if (val instanceof Number)
			putNumber(name, (Number) val);
		else if (val instanceof Character)
			putString(name, val.toString());
		else if (val instanceof String)
			putString(name, val.toString());
		else if (val instanceof ObjectId)
			putObjectId(name, (ObjectId) val);
		else if (val instanceof BSONObject)
			putObject(name, (BSONObject) val);
		else if (val instanceof Boolean)
			putBoolean(name, (Boolean) val);
		else if (val instanceof Pattern)
			putPattern(name, (Pattern) val);
		else if (val instanceof Map)
			putMap(name, (Map) val);
		else if (val instanceof Iterable)
			putIterable(name, (Iterable) val);
		else if (val instanceof byte[])
			putBinary(name, (byte[]) val);
		else if (val instanceof Binary)
			putBinary(name, (Binary) val);
		else if (val instanceof UUID)
			putUUID(name, (UUID) val);
		else if (val.getClass().isArray())
			putArray(name, val);
		else if (val instanceof Symbol) {
			putSymbol(name, (Symbol) val);
		} else if (val instanceof BSONTimestamp) {
			putTimestamp(name, (BSONTimestamp) val);
		} else if (val instanceof BSONDecimal) {
			putDecimal(name, (BSONDecimal)val);
		} else if (val instanceof CodeWScope) {
			putCodeWScope(name, (CodeWScope) val);
		} else if (val instanceof Code) {
			putCode(name, (Code) val);
		} else if (val instanceof MinKey) {
			putMinKey(name);
		} else if (val instanceof MaxKey) {
			putMaxKey(name);
		} else if (putSpecial(name, val)) {
		} else {
			throw new IllegalArgumentException("can't serialize " + val.getClass());
		}

	}

	private void putArray(String name, Object array) {
		_put(ARRAY, name);
		final int sizePos = _buf.getPosition();
		_buf.writeInt(0);

		int size = Array.getLength(array);
		for (int i = 0; i < size; i++)
			_putObjectField(String.valueOf(i), Array.get(array, i));

		_buf.write(EOO);
		_buf.writeInt(sizePos, _buf.getPosition() - sizePos);
	}

	private void putIterable(String name, Iterable l) {
		_put(ARRAY, name);
		final int sizePos = _buf.getPosition();
		_buf.writeInt(0);

		int i = 0;
		for (Object obj : l) {
			_putObjectField(String.valueOf(i), obj);
			i++;
		}

		_buf.write(EOO);
		_buf.writeInt(sizePos, _buf.getPosition() - sizePos);
	}

	private void putMap(String name, Map m) {
		_put(OBJECT, name);
		final int sizePos = _buf.getPosition();
		_buf.writeInt(0);

		for (Map.Entry entry : (Set<Map.Entry>) m.entrySet())
			_putObjectField(entry.getKey().toString(), entry.getValue());

		_buf.write(EOO);
		_buf.writeInt(sizePos, _buf.getPosition() - sizePos);
	}

	protected void putNull(String name) {
		_put(NULL, name);
	}

	protected void putUndefined(String name) {
		_put(UNDEFINED, name);
	}

	protected void putTimestamp(String name, BSONTimestamp ts) {
		_put(TIMESTAMP, name);
		_buf.writeInt(ts.getInc());
		_buf.writeInt(ts.getTime());
	} 
	
	protected void putDecimal(String name, BSONDecimal decimal) {
		int size = decimal.getSize();
		int typemod = decimal.getTypemod();
		short signscale = decimal.getSignScale();
		short weight = decimal.getWeight();
		short[] digits = decimal.getDigits();

		
		_put(NUMBER_DECIMAL, name);
		
		_buf.writeInt(size);
		_buf.writeInt(typemod);
		_buf.writeShort(signscale);
		_buf.writeShort(weight);
		for (int i = 0; i < digits.length; i++) {
			_buf.writeShort(digits[i]);
		}
	}
	
	protected void putCodeWScope(String name, CodeWScope code) {
		_put(CODE_W_SCOPE, name);
		int temp = _buf.getPosition();
		_buf.writeInt(0);
		_putValueString(code.getCode());
		putObject(code.getScope());
		_buf.writeInt(temp, _buf.getPosition() - temp);
	}

	protected void putCode(String name, Code code) {
		_put(CODE, name);
		int temp = _buf.getPosition();
		_putValueString(code.getCode());
	}

	protected void putBoolean(String name, Boolean b) {
		_put(BOOLEAN, name);
		_buf.write(b ? (byte) 0x1 : (byte) 0x0);
	}

	protected void putDate(String name, Date d) {
		_put(DATE, name);
		_buf.writeLong(d.getTime());
	}

	protected void putNumber(String name, Number n) {
		if (n instanceof Integer || n instanceof Short || n instanceof Byte || n instanceof AtomicInteger) {
			_put(NUMBER_INT, name);
			_buf.writeInt(n.intValue());
		} else if (n instanceof Long || n instanceof AtomicLong) {
			_put(NUMBER_LONG, name);
			_buf.writeLong(n.longValue());
		} else if (n instanceof Float || n instanceof Double) {
			_put(NUMBER, name);
			_buf.writeDouble(n.doubleValue());
		} else if (n instanceof BigDecimal) {
			putDecimal(name, new BSONDecimal((BigDecimal)n));
		} else {
			throw new IllegalArgumentException("can't serialize " + n.getClass());
		}
	}

	protected void putBinary(String name, byte[] data) {
		putBinary(name, B_GENERAL, data);
	}

	protected void putBinary(String name, Binary val) {
		putBinary(name, val.getType(), val.getData());
	}

	private void putBinary(String name, int type, byte[] data) {
		_put(BINARY, name);
		int totalLen = data.length;

		if (type == B_BINARY) totalLen += 4;

		_buf.writeInt(totalLen);
		_buf.write(type);
		if (type == B_BINARY) _buf.writeInt(totalLen - 4);
		int before = _buf.getPosition();
		_buf.write(data);
		int after = _buf.getPosition();
	}

	protected void putUUID(String name, UUID val) {
		_put(BINARY, name);
		_buf.writeInt(16);
		_buf.write(B_UUID);
		_buf.writeLong(val.getMostSignificantBits());
		_buf.writeLong(val.getLeastSignificantBits());
	}

	protected void putSymbol(String name, Symbol s) {
		_putString(name, s.getSymbol(), SYMBOL);
	}

	protected void putString(String name, String s) {
		_putString(name, s, STRING);
	}

	private void _putString(String name, String s, byte type) {
		_put(type, name);
		_putValueString(s);
	}

	protected void putObjectId(String name, ObjectId oid) {
		_put(OID, name);
		_buf.writeIntBE(oid._time());
		_buf.writeIntBE(oid._machine());
		_buf.writeIntBE(oid._inc());
	}

	private void putPattern(String name, Pattern p) {
		_put(REGEX, name);
		_put(p.pattern());
		_put(regexFlags(p.flags()));
	}

	private void putMinKey(String name) {
		_put(MINKEY, name);
	}

	private void putMaxKey(String name) {
		_put(MAXKEY, name);
	}


	/**
	 * Encodes the type and key.
	 * 
	 */
	protected void _put(byte type, String name) {
		_buf.write(type);
		_put(name);
	}

	protected void _putValueString(String s) {
		int lenPos = _buf.getPosition();
		_buf.writeInt(0); // making space for size
		int strLen = _put(s);
		_buf.writeInt(lenPos, strLen);
	}

	void _reset(Buffer b) {
		b.position(0);
		b.limit(b.capacity());
	}

	/**
	 * puts as utf-8 string
	 */
	protected int _put(String str) {

		final int len = str.length();
		int total = 0;

		for (int i = 0; i < len;) {
			int c = Character.codePointAt(str, i);

			if (c < 0x80) {
				_buf.write((byte) c);
				total += 1;
			} else if (c < 0x800) {
				_buf.write((byte) (0xc0 + (c >> 6)));
				_buf.write((byte) (0x80 + (c & 0x3f)));
				total += 2;
			} else if (c < 0x10000) {
				_buf.write((byte) (0xe0 + (c >> 12)));
				_buf.write((byte) (0x80 + ((c >> 6) & 0x3f)));
				_buf.write((byte) (0x80 + (c & 0x3f)));
				total += 3;
			} else {
				_buf.write((byte) (0xf0 + (c >> 18)));
				_buf.write((byte) (0x80 + ((c >> 12) & 0x3f)));
				_buf.write((byte) (0x80 + ((c >> 6) & 0x3f)));
				_buf.write((byte) (0x80 + (c & 0x3f)));
				total += 4;
			}

			i += Character.charCount(c);
		}

		_buf.write((byte) 0);
		total++;
		return total;
	}

	public void writeInt(int x) {
		_buf.writeInt(x);
	}

	public void writeLong(long x) {
		_buf.writeLong(x);
	}

	public void writeCString(String s) {
		_put(s);
	}

	protected OutputBuffer _buf;

}
