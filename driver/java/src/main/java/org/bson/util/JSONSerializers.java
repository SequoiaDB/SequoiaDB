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

import java.lang.reflect.Array;
import java.math.BigDecimal;
import java.sql.Timestamp;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.Map;
import java.util.Map.Entry;
import java.util.SimpleTimeZone;
import java.util.UUID;
import java.util.regex.Pattern;

import org.bson.BSON;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.*;

/**
 * Defines static methods for getting <code>ObjectSerializer</code> instances
 * that produce various flavors of JSON.
 */
public class JSONSerializers {

	private JSONSerializers() {
	}

	/**
	 * Returns an <code>ObjectSerializer</code> that mostly conforms to the
	 * strict JSON format defined in <a
	 * href="http://www.mongodb.org/display/DOCS/Mongo+Extended+JSON", but with
	 * a few differences to keep compatibility with previous versions of the
	 * driver. Clients should generally prefer <code>getStrict</code> in
	 * preference to this method.
	 * 
	 * @return object serializer
	 * @see #getStrict()
	 */
	public static ObjectSerializer getLegacy() {

		ClassMapBasedObjectSerializer serializer = addCommonSerializers();

		serializer.addObjectSerializer(Date.class, new LegacyDateSerializer(serializer));
		serializer.addObjectSerializer(Timestamp.class, new LegacyBSONTimestampSerializer(serializer));
		serializer.addObjectSerializer(BSONTimestamp.class, new LegacyBSONTimestampSerializer(serializer));
		serializer.addObjectSerializer(Binary.class, new BinarySerializer(serializer));
		serializer.addObjectSerializer(byte[].class, new ByteArraySerializer(serializer));
		return serializer;
	}

	/**
	 * Returns an <code>ObjectSerializer</code> that conforms to the strict JSON
	 * format defined in <a
	 * href="http://www.mongodb.org/display/DOCS/Mongo+Extended+JSON".
	 * 
	 * @return object serializer
	 */
	public static ObjectSerializer getStrict() {

		ClassMapBasedObjectSerializer serializer = addCommonSerializers();

		serializer.addObjectSerializer(Date.class, new DateSerializer(serializer));
		serializer.addObjectSerializer(Timestamp.class, new BSONTimestampSerializer(serializer));
		serializer.addObjectSerializer(BSONTimestamp.class, new BSONTimestampSerializer(serializer));
		serializer.addObjectSerializer(Binary.class, new BinarySerializer(serializer));
		serializer.addObjectSerializer(byte[].class, new ByteArraySerializer(serializer));

		return serializer;
	}

	static ClassMapBasedObjectSerializer addCommonSerializers() {
		ClassMapBasedObjectSerializer serializer = new ClassMapBasedObjectSerializer();

		serializer.addObjectSerializer(Object[].class, new ObjectArraySerializer(serializer));
		serializer.addObjectSerializer(Boolean.class, new ToStringSerializer());
		serializer.addObjectSerializer(Code.class, new CodeSerializer(serializer));
		serializer.addObjectSerializer(CodeWScope.class, new CodeWScopeSerializer(serializer));
		serializer.addObjectSerializer(Iterable.class, new IterableSerializer(serializer));
		serializer.addObjectSerializer(Map.class, new MapSerializer(serializer));
		serializer.addObjectSerializer(MaxKey.class, new MaxKeySerializer(serializer));
		serializer.addObjectSerializer(MinKey.class, new MinKeySerializer(serializer));
		serializer.addObjectSerializer(Number.class, new ToStringSerializer());
		serializer.addObjectSerializer(ObjectId.class, new ObjectIdSerializer(serializer));
		serializer.addObjectSerializer(Pattern.class, new PatternSerializer(serializer));
		serializer.addObjectSerializer(String.class, new StringSerializer());
		serializer.addObjectSerializer(UUID.class, new UUIDSerializer(serializer));
		serializer.addObjectSerializer(Long.class, new NumberLongSerializer(serializer));
		serializer.addObjectSerializer(BasicBSONObject.class, new BasicBSONObjectSerializer(serializer));
		serializer.addObjectSerializer(BSONDecimal.class, new BSONDecimalSerializer(serializer));
		serializer.addObjectSerializer(BigDecimal.class, new BSONDecimalSerializer(serializer));
		serializer.addObjectSerializer(Symbol.class, new SymbolSerializer(serializer));
		return serializer;
	}

	private abstract static class CompoundObjectSerializer extends AbstractObjectSerializer {
		protected final ObjectSerializer serializer;

		CompoundObjectSerializer(ObjectSerializer serializer) {
			this.serializer = serializer;
		}
	}

	private static class LegacyBinarySerializer extends AbstractObjectSerializer {

		public void serialize(Object obj, StringBuilder buf) {
			buf.append("<Binary Data>");
		}

	}

	private static class ObjectArraySerializer extends CompoundObjectSerializer {

		ObjectArraySerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			buf.append("[ ");
			for (int i = 0; i < Array.getLength(obj); i++) {
				if (i > 0) buf.append(" , ");
				serializer.serialize(Array.get(obj, i), buf);
			}

			buf.append(" ]");
		}

	}

	private static class ToStringSerializer extends AbstractObjectSerializer {

		public void serialize(Object obj, StringBuilder buf) {
			buf.append(obj.toString());
		}

	}

	private static class LegacyBSONTimestampSerializer extends CompoundObjectSerializer {

		LegacyBSONTimestampSerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			BSONTimestamp t;
			if (obj instanceof Timestamp) {
				t = new BSONTimestamp((Timestamp) obj);
			} else {
				t = (BSONTimestamp) obj;
			}
			BasicBSONObject temp = new BasicBSONObject();
			temp.put("$ts", Integer.valueOf(t.getTime()));
			temp.put("$inc", Integer.valueOf(t.getInc()));
			serializer.serialize(temp, buf);
		}

	}

	private static class CodeSerializer extends CompoundObjectSerializer {

		CodeSerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			Code c = (Code) obj;
			BasicBSONObject temp = new BasicBSONObject();
			temp.put("$code", c.getCode());
			serializer.serialize(temp, buf);
		}

	}

	private static class CodeWScopeSerializer extends CompoundObjectSerializer {

		CodeWScopeSerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			CodeWScope c = (CodeWScope) obj;
			BasicBSONObject temp = new BasicBSONObject();
			temp.put("$code", c.getCode());
			temp.put("$scope", c.getScope());
			serializer.serialize(temp, buf);
		}

	}

	private static class LegacyDateSerializer extends CompoundObjectSerializer {

		LegacyDateSerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			Date d = (Date) obj;
			SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd");
			serializer.serialize(new BasicBSONObject("$date", format.format(d)), buf);
		}

	}

	private static class BasicBSONObjectSerializer extends CompoundObjectSerializer {

		BasicBSONObjectSerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			boolean first = true;
			buf.append("{ ");
			BSONObject dbo = (BSONObject) obj;
			String name;

			for (final String s : dbo.keySet()) {
				name = s;

				if (first)
					first = false;
				else
					buf.append(" , ");

				JSON.string(buf, name);
				buf.append(" : ");
				serializer.serialize(dbo.get(name), buf);
			}

			buf.append(" }");
		}

	}

	private static class IterableSerializer extends CompoundObjectSerializer {

		IterableSerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			boolean first = true;
			buf.append("[ ");

			for (final Object o : ((Iterable) obj)) {
				if (first)
					first = false;
				else
					buf.append(" , ");

				serializer.serialize(o, buf);
			}
			buf.append(" ]");
		}
	}

	private static class MapSerializer extends CompoundObjectSerializer {

		MapSerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			boolean first = true;
			buf.append("{ ");
			Map m = (Map) obj;
			Entry entry;

			for (final Object o : m.entrySet()) {
				entry = (Entry) o;
				if (first)
					first = false;
				else
					buf.append(" , ");
				JSON.string(buf, entry.getKey().toString());
				buf.append(" : ");
				serializer.serialize(entry.getValue(), buf);
			}

			buf.append(" }");
		}

	}

	private static class MaxKeySerializer extends CompoundObjectSerializer {

		MaxKeySerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			serializer.serialize(new BasicBSONObject("$maxKey", 1), buf);
		}

	}

	private static class MinKeySerializer extends CompoundObjectSerializer {

		MinKeySerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			serializer.serialize(new BasicBSONObject("$minKey", 1), buf);
		}

	}

	private static class ObjectIdSerializer extends CompoundObjectSerializer {

		ObjectIdSerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			serializer.serialize(new BasicBSONObject("$oid", obj.toString()), buf);
		}
	}

	private static class NumberLongSerializer extends CompoundObjectSerializer {

		NumberLongSerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) { 
			if (!BSON.getJSCompatibility()) {
				buf.append(obj.toString());
			} else {
				Long number = (Long)obj;
				if (number >= -9007199254740991L && number <= 9007199254740991L) {
					buf.append(obj.toString());
				} else {
					serializer.serialize(new BasicBSONObject("$numberLong", obj.toString()), buf);
				}
			}
		}
	}
	
	private static class PatternSerializer extends CompoundObjectSerializer {

		PatternSerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			BSONObject externalForm = new BasicBSONObject();
			externalForm.put("$regex", obj.toString());
			if (((Pattern) obj).flags() != 0) externalForm.put("$options", BSON.regexFlags(((Pattern) obj).flags()));
			serializer.serialize(externalForm, buf);
		}
	}

	private static class StringSerializer extends AbstractObjectSerializer {

		public void serialize(Object obj, StringBuilder buf) {
			JSON.string(buf, (String) obj);
		}
	}

	private static class UUIDSerializer extends CompoundObjectSerializer {

		UUIDSerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			UUID uuid = (UUID) obj;
			BasicBSONObject temp = new BasicBSONObject();
			temp.put("$uuid", uuid.toString());
			serializer.serialize(temp, buf);
		}
	}

	private static class BSONTimestampSerializer extends CompoundObjectSerializer {

		BSONTimestampSerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			BSONTimestamp t;
			if (obj instanceof Timestamp) {
				t = new BSONTimestamp((Timestamp) obj);
			} else {
				t = (BSONTimestamp) obj;
			}
			BasicBSONObject temp = new BasicBSONObject();
			temp.put("$t", Integer.valueOf(t.getTime()));
			temp.put("$i", Integer.valueOf(t.getInc()));
			BasicBSONObject timestampObj = new BasicBSONObject();
			timestampObj.put("$timestamp", temp);
			serializer.serialize(timestampObj, buf);
		}

	}

	private static class BSONDecimalSerializer extends CompoundObjectSerializer {
		
		BSONDecimalSerializer(ObjectSerializer serializer) {
			super(serializer);
		}
		
		public void serialize(Object obj, StringBuilder buf) {
			BasicBSONObject temp = new BasicBSONObject();
			BSONDecimal t = null;
			if (obj instanceof BigDecimal) {
				t = new BSONDecimal((BigDecimal)obj);
			} else {
				t = (BSONDecimal)obj;
			}
			String data = t.getValue();
			int precision = t.getPrecision();
			int scale = t.getScale();
			temp.put("$decimal", data);
			if (precision != -1 || scale != -1) {
				BSONObject arr = new BasicBSONList();
				arr.put("0", precision);
				arr.put("1", scale);
				temp.put("$precision", arr);
			}
			serializer.serialize(temp, buf);
		}

	}
	
	private static class DateSerializer extends CompoundObjectSerializer {

		DateSerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			Date d = (Date) obj;
			serializer.serialize(new BasicBSONObject("$date", d.getTime()), buf);
		}

	}

	private abstract static class BinarySerializerBase extends CompoundObjectSerializer {
		BinarySerializerBase(ObjectSerializer serializer) {
			super(serializer);
		}

		protected void serialize(byte[] bytes, byte type, StringBuilder buf) {
			BSONObject temp = new BasicBSONObject();
			temp.put("$binary", (new Base64Codec()).encode(bytes));
			temp.put("$type", String.valueOf(type));
			serializer.serialize(temp, buf);
		}
	}

	private static class BinarySerializer extends BinarySerializerBase {
		BinarySerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			Binary bin = (Binary) obj;
			serialize(bin.getData(), bin.getType(), buf);
		}

	}

	private static class ByteArraySerializer extends BinarySerializerBase {
		ByteArraySerializer(ObjectSerializer serializer) {
			super(serializer);
		}

		public void serialize(Object obj, StringBuilder buf) {
			serialize((byte[]) obj, (byte) 0, buf);
		}

	}

    private static class SymbolSerializer extends CompoundObjectSerializer {

        SymbolSerializer(ObjectSerializer serializer) {
            super(serializer);
        }

        @Override
        public void serialize(Object obj, StringBuilder buf) {
            Symbol s = (Symbol) obj;
            BasicBSONObject temp = new BasicBSONObject();
            temp.put("$symbol", s.getSymbol());
            serializer.serialize(temp, buf);
        }

    }
}
