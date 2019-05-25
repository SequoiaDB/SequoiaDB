
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

import java.beans.BeanInfo;
import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.math.BigDecimal;
import java.util.Collection;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.UUID;
import java.util.regex.Pattern;

import org.bson.types.BSONDecimal;
import org.bson.types.BSONTimestamp;
import org.bson.types.BasicBSONList;
import org.bson.types.Binary;
import org.bson.types.Code;
import org.bson.types.CodeWScope;
import org.bson.types.MaxKey;
import org.bson.types.MinKey;
import org.bson.types.ObjectId;
import org.bson.types.Symbol;
import org.bson.util.JSON;


/**
 * A simple implementation of <code>BSONObject</code>. A <code>BSONObject</code>
 * can be created as follows, using this class: <blockquote>
 * 
 * <pre>
 * BSONObject obj = new BasicBSONObject();
 * obj.put(&quot;foo&quot;, &quot;bar&quot;);
 * </pre>
 * 
 * </blockquote>
 */
public class BasicBSONObject implements Map<String, Object>, BSONObject {
	private static final long serialVersionUID = -4415279469780082174L;
	private Map<String, Object> _objectMap = null;

	/**
	 * Creates an empty object.
	 * 
	 * @param sort
	 *            true: key will be sorted false: key won't be sorted.
	 */
	public BasicBSONObject(boolean sort) {
		if (sort) {
			_objectMap = new TreeMap<String, Object>();
		} else {
			_objectMap = new LinkedHashMap<String, Object>();
		}
	}

	/**
	 * Creates an empty object. by default, key won't be sorted
	 */
	public BasicBSONObject() {
		this(false);
	}

	public BasicBSONObject(int size) {
		this(false);
	}

	/**
	 * The current bson object keeps any elements or not
	 * 
	 * @return true for empty, false for not
	 */
	public boolean isEmpty() {
		return _objectMap.size() == 0;
	}

	/**
	 * Convenience CTOR
	 * 
	 * @param key
	 *            key under which to store
	 * @param value
	 *            value to store
	 */
	public BasicBSONObject(String key, Object value) {
		this(false);
		put(key, value);
	}

	/**
	 * Creates a BSONObject from a map.
	 * 
	 * @param m
	 *            map to convert
	 */
	@SuppressWarnings({ "unchecked" })
	public BasicBSONObject(Map m) {
		_objectMap = new LinkedHashMap<String, Object>(m);
	}

	/**
	 * Converts a BSONObject to a map.
	 * 
	 * @return the BSONObject
	 */
	public Map toMap() {
		if (_objectMap instanceof LinkedHashMap) {
			return new LinkedHashMap<String, Object>(_objectMap);
		} else {
			return new TreeMap<String, Object>(_objectMap);
		}
	}

	/**
	 * Deletes a field from this object.
	 * 
	 * @param key
	 *            the field name to remove
	 * @return the object removed
	 */
	public Object removeField(String key) {
		return _objectMap.remove(key);
	}

	/**
	 * Checks if this object contains a given field
	 * 
	 * @param field
	 *            field name
	 * @return if the field exists
	 */
	public boolean containsField(String field) {
		return _objectMap.containsKey(field);
	}

	/**
	 * @deprecated
	 */
	@Deprecated
	public boolean containsKey(String key) {
		return containsField(key);
	}

	/**
	 * Gets a value from this object
	 * 
	 * @param key
	 *            field name
	 * @return the value
	 */
	public Object get(String key) {
		return _objectMap.get(key);
	}

	/**
	 * Returns the value of a field as an <code>int</code>.
	 * 
	 * @param key
	 *            the field to look for
	 * @return the field value (or default)
	 */
	public int getInt(String key) {
		Object o = get(key);
		if (o == null)
			throw new NullPointerException("no value for: " + key);

		return BSON.toInt(o);
	}

	/**
	 * Returns the value of a field as an <code>int</code>.
	 * 
	 * @param key
	 *            the field to look for
	 * @param def
	 *            the default to return
	 * @return the field value (or default)
	 */
	public int getInt(String key, int def) {
		Object foo = get(key);
		if (foo == null)
			return def;

		return BSON.toInt(foo);
	}

	/**
	 * Returns the value of a field as a <code>long</code>.
	 * 
	 * @param key
	 *            the field to return
	 * @return the field value
	 */
	public long getLong(String key) {
		Object foo = get(key);
		return ((Number) foo).longValue();
	}

	/**
	 * Returns the value of a field as an <code>long</code>.
	 * 
	 * @param key
	 *            the field to look for
	 * @param def
	 *            the default to return
	 * @return the field value (or default)
	 */
	public long getLong(String key, long def) {
		Object foo = get(key);
		if (foo == null)
			return def;

		return ((Number) foo).longValue();
	}

	/**
	 * Returns the value of a field as a <code>double</code>.
	 * 
	 * @param key
	 *            the field to return
	 * @return the field value
	 */
	public double getDouble(String key) {
		Object foo = get(key);
		return ((Number) foo).doubleValue();
	}

	/**
	 * Returns the value of a field as an <code>double</code>.
	 * 
	 * @param key
	 *            the field to look for
	 * @param def
	 *            the default to return
	 * @return the field value (or default)
	 */
	public double getDouble(String key, double def) {
		Object foo = get(key);
		if (foo == null)
			return def;

		return ((Number) foo).doubleValue();
	}

	/**
	 * Returns the value of a field as a string
	 * 
	 * @param key
	 *            the field to look up
	 * @return the value of the field, converted to a string
	 */
	public String getString(String key) {
		Object foo = get(key);
		if (foo == null)
			return null;
		return foo.toString();
	}

	/**
	 * Returns the value of a field as a string
	 * 
	 * @param key
	 *            the field to look up
	 * @param def
	 *            the default to return
	 * @return the value of the field, converted to a string
	 */
	public String getString(String key, final String def) {
		Object foo = get(key);
		if (foo == null)
			return def;

		return foo.toString();
	}

	/**
	 * Returns the value of a field as a boolean.
	 * 
	 * @param key
	 *            the field to look up
	 * @return the value of the field, or false if field does not exist
	 */
	public boolean getBoolean(String key) {
		return getBoolean(key, false);
	}

	/**
	 * Returns the value of a field as a boolean
	 * 
	 * @param key
	 *            the field to look up
	 * @param def
	 *            the default value in case the field is not found
	 * @return the value of the field, converted to a string
	 */
	public boolean getBoolean(String key, boolean def) {
		Object foo = get(key);
		if (foo == null)
			return def;
		if (foo instanceof Number)
			return ((Number) foo).intValue() > 0;
		if (foo instanceof Boolean)
			return ((Boolean) foo).booleanValue();
		throw new IllegalArgumentException("can't coerce to bool:"
				+ foo.getClass());
	}

	/**
	 * Returns the object id or null if not set.
	 * 
	 * @param field
	 *            The field to return
	 * @return The field object value or null if not found (or if null :-^).
	 */
	public ObjectId getObjectId(final String field) {
		return (ObjectId) get(field);
	}

	/**
	 * Returns the object id or def if not set.
	 * 
	 * @param field
	 *            The field to return
	 * @param def
	 *            the default value in case the field is not found
	 * @return The field object value or def if not set.
	 */
	public ObjectId getObjectId(final String field, final ObjectId def) {
		final Object foo = get(field);
		return (foo != null) ? (ObjectId) foo : def;
	}

	/**
	 * Returns the date or null if not set.
	 * 
	 * @param field
	 *            The field to return
	 * @return The field object value or null if not found.
	 */
	public Date getDate(final String field) {
		return (Date) get(field);
	}

	/**
	 * Returns the date or def if not set.
	 * 
	 * @param field
	 *            The field to return
	 * @param def
	 *            the default value in case the field is not found
	 * @return The field object value or def if not set.
	 */
	public Date getDate(final String field, final Date def) {
		final Object foo = get(field);
		return (foo != null) ? (Date) foo : def;
	}

	/**
	 * Returns the BigDecimal object or null if not set.
	 * 
	 * @param field
	 *            The field to return
	 * @return The field object value or null if not found.
	 */
	public BigDecimal getBigDecimal(final String field) {
		Object obj = get(field);
		if (obj == null) {
			return null;
		}
		if (obj instanceof BigDecimal) {
			return (BigDecimal) obj;
		} else {
			return ((BSONDecimal) get(field)).toBigDecimal();
		}
	}

	/**
	 * Returns the BigDecimal object or def if not set.
	 * 
	 * @param field
	 *            The field to return
	 * @param def
	 *            the default value in case the field is not found
	 * @return The field object value or def if not set.
	 */
	public BigDecimal getBigDecimal(final String field, final BigDecimal def) {
		final Object foo = get(field);
		return (foo != null) ? (BigDecimal) foo : def;
	}

	/**
	 * Add a key/value pair to this object
	 * 
	 * @param key
	 *            the field name
	 * @param val
	 *            the field value
	 * @return the <code>val</code> parameter
	 */
	public Object put(String key, Object val) {
		return _objectMap.put(key, val);
	}

	/**
	 * Sets all key/value pairs from a map into this object
	 * 
	 * @param m
	 *            the map
	 */
	@SuppressWarnings({ "unchecked", "rawtypes" })
	public void putAll(Map m) {
		for (Map.Entry entry : (Set<Map.Entry>) m.entrySet()) {
			put(entry.getKey().toString(), entry.getValue());
		}
	}

	/**
	 * Sets all key/value pairs from an object into this object
	 * 
	 * @param o
	 *            the object
	 */
	public void putAll(BSONObject o) {
		for (String k : o.keySet()) {
			put(k, o.get(k));
		}
	}

	/**
	 * Add a key/value pair to this object
	 * 
	 * @param key
	 *            the field name
	 * @param val
	 *            the field value
	 * @return <code>this</code>
	 */
	public BasicBSONObject append(String key, Object val) {
		put(key, val);

		return this;
	}

	/**
	 * Returns a JSON serialization of this object
	 * 
	 * @return JSON serialization
	 */
	@Override
	public String toString() {
		return JSON.serialize(this);
	}

	@Override
    public int hashCode() {
	    return _objectMap.hashCode();
    }

	/**
	 * Current bson object is equal with the other or not
	 * 
	 * @return true or false
	 */
	@Override
	public boolean equals(Object o) {
		if (!(o instanceof BSONObject))
			return false;

		BSONObject other = (BSONObject) o;
		if (!keySet().equals(other.keySet()))
			return false;

		for (String key : keySet()) {
			Object a = get(key);
			Object b = other.get(key);

			if (a == null) {
				if (b != null)
					return false;
			}
			if (b == null) {
				if (a != null)
					return false;
			} else if (a instanceof Pattern && b instanceof Pattern) {
				Pattern p1 = (Pattern) a;
				Pattern p2 = (Pattern) b;
				if (!p1.pattern().equals(p2.pattern())
						|| p1.flags() != p2.flags())
					return false;
			} else {
				if (!a.equals(b))
					return false;
			}
		}
		return true;
	}

	@SuppressWarnings({ "rawtypes" })
	public boolean BasicTypeWrite(Object object, Object value, Method method)
			throws IllegalArgumentException, IllegalAccessException,
			InvocationTargetException {
		Class<?> paramType = method.getParameterTypes()[0];
		boolean result = true;
		boolean numberCompare = false;
		if (paramType.isPrimitive()) {

			if (paramType.getName().equals("int")) {
				method.invoke(object, ((Number) value).intValue());
			} else if (paramType.getName().equals("long")) {
				method.invoke(object, ((Number) value).longValue());
			} else if (paramType.getName().equals("byte")) {
				method.invoke(object, ((Number) value).byteValue());
			} else if (paramType.getName().equals("double")) {
				method.invoke(object, ((Number) value).doubleValue());
			} else if (paramType.getName().equals("float")) {
				method.invoke(object, ((Number) value).floatValue());
			} else if (paramType.getName().equals("short")) {
				method.invoke(object, ((Number) value).shortValue());
			} else if (paramType.getName().equals("char")) {
				method.invoke(object, ((Character) value).charValue());
			} else if (paramType.getName().equals("boolean")) {
				method.invoke(object, ((Boolean) value).booleanValue());// TODO
			} else {
				result = false;
			}

			return result;
		}

		if ((paramType.getName().equals("java.lang.Integer")
				|| paramType.getName().equals("java.lang.Long")
				|| paramType.getName().equals("java.lang.Float") || paramType
				.getName().equals("java.lang.Double"))
				&& (value.getClass().getName().equals("java.lang.Integer")
						|| value.getClass().getName().equals("java.lang.Long")
						|| value.getClass().getName().equals("java.lang.Float") || value
						.getClass().getName().equals("java.lang.Double"))) {
			numberCompare = true;
		}
		if (!numberCompare) {
			if (!paramType.isInstance(value)
					&& (!value.getClass().getName()
							.equals("java.math.BigDecimal") && !value
							.getClass().getName()
							.equals("org.bson.types.BSONDecimal"))) {
				throw new IllegalArgumentException("The method: "
						+ method.getName() + " Expected parameter type:"
						+ paramType.getName()
						+ " does not match with the actual type:"
						+ value.getClass().getName());
			}
		}

		result = true;
		if (String.class.isAssignableFrom(paramType)) {
			method.invoke(object, (String) value);
		} else if (Date.class.isAssignableFrom(paramType)) {
			method.invoke(object, (Date) value);
		} else if (Integer.class.isAssignableFrom(paramType)) {
			method.invoke(object, new Integer(((Number) value).intValue()));
		} else if (Long.class.isAssignableFrom(paramType)) {
			method.invoke(object, new Long(((Number) value).longValue()));
		} else if (Double.class.isAssignableFrom(paramType)) {
			method.invoke(object, new Double(((Number) value).doubleValue()));
		} else if (Float.class.isAssignableFrom(paramType)) {
			method.invoke(object, new Float(((Number) value).floatValue()));
		} else if (Character.class.isAssignableFrom(paramType)) {
			method.invoke(object, (Character) value);
		} else if (ObjectId.class.isAssignableFrom(paramType)) {
			method.invoke(object, (ObjectId) value);
		} else if (Boolean.class.isAssignableFrom(paramType)) {
			method.invoke(object, (Boolean) value);
		} else if (Pattern.class.isAssignableFrom(paramType)) {
			method.invoke(object, (Pattern) value);
		} else if (byte[].class.isAssignableFrom(paramType)) {
			method.invoke(object, (byte[]) value);
		} else if (Binary.class.isAssignableFrom(paramType)) {
			method.invoke(object, (Binary) value);
		} else if (UUID.class.isAssignableFrom(paramType)) {
			method.invoke(object, (UUID) value);
		} else if (Symbol.class.isAssignableFrom(paramType)) {
			method.invoke(object, (Symbol) value);
		} else if (BSONTimestamp.class.isAssignableFrom(paramType)) {
			method.invoke(object, (BSONTimestamp) value);
		} else if (BSONDecimal.class.isAssignableFrom(paramType)) {
			String className = value.getClass().getName();
			if (className.equals("java.math.BigDecimal")) {
				method.invoke(object, new BSONDecimal((BigDecimal) value));
			} else if (className.equals("org.bson.types.BSONDecimal")) {
				method.invoke(object, (BSONDecimal) value);
			} else {
				throw new IllegalArgumentException("The method: "
						+ method.getName() + " Expected parameter type:"
						+ paramType.getName()
						+ " does not match with the actual type:" + className);
			}
		} else if (BigDecimal.class.isAssignableFrom(paramType)) {
			String className = value.getClass().getName();
			if (className.equals("java.math.BigDecimal")) {
				method.invoke(object, (BigDecimal) value);
			} else if (className.equals("org.bson.types.BSONDecimal")) {
				method.invoke(object, ((BSONDecimal) value).toBigDecimal());
			} else {
				throw new IllegalArgumentException("The method: "
						+ method.getName() + " Expected parameter type:"
						+ paramType.getName()
						+ " does not match with the actual type:" + className);
			}
		} else if (CodeWScope.class.isAssignableFrom(paramType)) {
			method.invoke(object, (CodeWScope) value);
		} else if (Code.class.isAssignableFrom(paramType)) {
			method.invoke(object, (Code) value);
		} else if (MinKey.class.isAssignableFrom(paramType))
			method.invoke(object, (MinKey) value);
		else if (MaxKey.class.isAssignableFrom(paramType)) {
			method.invoke(object, (MaxKey) value);
		} else if (List.class.isAssignableFrom(paramType)) {
			method.invoke(object, (List) value);
		} else {
			result = false;
		}
		return result;
	}


	/**
	 * Return an instance of the class "cls", only for BasicBSONObject
	 * @param cls target class object
	 * @return the instance of the class
	 * @throws Exception If error happens.
	 */
	public /*! @cond x*/ <T> /*! @endcond */ T as(Class<T> cls) throws Exception {
		return as(cls, null);
	}

	@SuppressWarnings({ "unchecked" })
	public /*! @cond x*/ <T> /*! @endcond */ T as(Class<T> cls, Type eleType) throws Exception {
		boolean hasConsturctor = false;
		T result = null;
		for (Constructor<?> con : cls.getConstructors()) {
			if (con.getParameterTypes().length == 0) {
				result = (T) con.newInstance();
				hasConsturctor = true;
				break;
			}
		}
		if (hasConsturctor == false) {
			throw new Exception("Class " + cls.getName()
					+ " does not exist an default constructor method");
		}

		if (BSON.IsBasicType(result)) {
			throw new IllegalArgumentException(
					"Not support as to basic type. type=" + cls.getName());
		} else if (Collection.class.isAssignableFrom(cls)
				|| Map.class.isAssignableFrom(cls) || cls.isArray()) {
			throw new IllegalArgumentException(
					"Not support as to Collection/Map/Array type. type="
							+ cls.getName());
		} else {
			BeanInfo bi = Introspector.getBeanInfo(cls);
			PropertyDescriptor[] props = bi.getPropertyDescriptors();

			Object value = null;
			for (PropertyDescriptor p : props) {
				if (this.containsField(p.getName())) {
					Method writeMethod = p.getWriteMethod();

					if (writeMethod == null) {
						throw new IllegalArgumentException("The property:"
								+ cls.getName() + "." + p.getName()
								+ " have no set method.");
					}

					value = this.get(p.getName());

					if (value == null) {
						continue;
					} else if (p.getPropertyType().equals(java.util.Map.class)) { // TODO
						Field mapField = cls.getDeclaredField(p.getName());
						Type generictype = mapField.getGenericType();
						Type valueType = null;
						if (generictype instanceof ParameterizedType) {
							Type[] types = ((ParameterizedType) generictype)
									.getActualTypeArguments();
							valueType = types[1];
						}
						Map map = ((BSONObject) value).toMap();
						Map realMap = new HashMap();
						Set<Map.Entry<?, ?>> set = map.entrySet();
						Iterator<Map.Entry<?, ?>> iterator = set.iterator();
						while (iterator.hasNext()) {
							Map.Entry<?, ?> entry = iterator.next();
							String key = entry.getKey().toString();
							if (((Class) valueType)
									.equals(java.lang.Object.class)) {
								Object v = entry.getValue();
								if (BSON.IsBasicType(v)) {
									realMap.put(key, v);
								} else if (v instanceof BasicBSONList) {
									realMap.put(key,
											((BasicBSONList) v).asList());
								} else if (v instanceof BasicBSONObject) {
									realMap.put(key,
											((BasicBSONObject) v).asMap());
								} else {
									throw new IllegalArgumentException(
											"can't support in map. value_type="
													+ v.getClass());
								}
							} else {
								if (((Class) valueType).isPrimitive()
										|| ((Class) valueType)
												.equals(java.lang.String.class)) {
									realMap.put(key,
											((BSONObject) value).get(key));
								} else {
									Object tmpObj = ((BSONObject) value)
											.get(key);
									if (BSON.IsBasicType(tmpObj)) {
										realMap.put(key, tmpObj);
									} else {
										realMap.put(key, ((BSONObject) tmpObj)
												.as((Class) valueType));
									}
								}
							}
						}
						writeMethod.invoke(result, realMap);
					} else if (value instanceof BasicBSONObject) { // bson <=>
						writeMethod.invoke(result,
								((BSONObject) value).as(p.getPropertyType()));
					} else if (value instanceof BasicBSONList) { // bsonlist <=>

						Field f = cls.getDeclaredField(p.getName());
						if (f == null)
							continue;
						Type _type = f.getGenericType();

						Type fileType = null;
						if (_type != null && _type instanceof ParameterizedType) {
							fileType = ((ParameterizedType) _type)
									.getActualTypeArguments()[0];
						} else {
							throw new IllegalArgumentException(
									"Current version only support parameterized type Collection(List/Set/Queue) field. unknow type="
											+ _type.toString());
						}

						writeMethod.invoke(result, ((BSONObject) value).as(
								p.getPropertyType(), fileType));
					} else if (BasicTypeWrite(result, value, writeMethod)) {
						continue;
					} else {
						continue;
					}
				}
			}
		}
		return result;
	}

	public Object asMap() {
		Map<String, Object> realMap = new HashMap<String, Object>();
		for (String key : this.keySet()) {
			Object v = this.get(key);
			if (v == null) {
				continue;
			} else if (BSON.IsBasicType(v)) {
				realMap.put(key, v);
			} else if (v instanceof BasicBSONList) {
				realMap.put(key, ((BasicBSONList) v).asList());
			} else if (v instanceof BasicBSONObject) {
				realMap.put(key, ((BasicBSONObject) v).asMap());
			} else {
				throw new IllegalArgumentException(
						"can't support in map. value_type=" + v.getClass());
			}
		}

		return realMap;
	}

	public static BSONObject typeToBson(Object object, Boolean ignoreNullValue)
			throws IntrospectionException, IllegalArgumentException,
			IllegalAccessException, InvocationTargetException {
		BSONObject result = null;
		if (object == null) {
			result = null;
		} else if (BSON.IsBasicType(object)) {
			throw new IllegalArgumentException(
					"Current version is not support basice type to bson in the top level.");
		} else if (object instanceof List) {
			BSONObject listObj = new BasicBSONList();
			List list = (List) object;
			int index = 0;
			for (Object obj : list) {
				if (BSON.IsBasicType(obj)) {
					if (!ignoreNullValue || null != obj) {
						listObj.put(Integer.toString(index), obj);
					}
				} else {
					BSONObject tmpObj = typeToBson(obj, ignoreNullValue);
					if (!ignoreNullValue || null != tmpObj) {
						listObj.put(Integer.toString(index), tmpObj);
					}
				}
				++index;
			}
			result = listObj;
		} else if (object instanceof Map) {
			BSONObject mapObj = new BasicBSONObject();
			Map map = (Map) object;
			Set<Map.Entry<?, ?>> set = map.entrySet();
			Iterator<Map.Entry<?, ?>> iterator = set.iterator();
			while (iterator.hasNext()) {
				Map.Entry<?, ?> entry = iterator.next();
				String key = entry.getKey().toString();
				Object value = entry.getValue();
				if (BSON.IsBasicType(value)) {
					if (!ignoreNullValue || null != value) {
						mapObj.put(key, value);
					}
				} else {
					BSONObject tmpObj = typeToBson(value, ignoreNullValue);
					if (!ignoreNullValue || null != value) {
						mapObj.put(key, tmpObj);
					}
				}
			}
			result = mapObj;
		} else if (object.getClass().isArray()) {
			throw new IllegalArgumentException(
					"Current version is not support Array type field.");
		} else if (object instanceof BSONObject) {
			result = (BSONObject) object;
		} else if (object.getClass().getName() == "java.lang.Class") {
			throw new IllegalArgumentException(
					"Current version is not support java.lang.Class type field.");
		} else { // User define type.
			result = new BasicBSONObject();
			Class<?> cl = object.getClass();

			BeanInfo bi = Introspector.getBeanInfo(cl);
			PropertyDescriptor[] props = bi.getPropertyDescriptors();
			for (PropertyDescriptor p : props) {
				Class<?> type = p.getPropertyType();
				Method readMethod = p.getReadMethod();
				if (readMethod == null) {
					throw new IllegalArgumentException("The property:"
							+ cl.getName() + "." + p.getName()
							+ " have no get method.");
				}
				Object propObj = readMethod.invoke(object);
				if (BSON.IsBasicType(propObj)) {
					if (!ignoreNullValue || null != propObj) {
						result.put(p.getName(), propObj);
					}
				} else if (type.getName() == "java.lang.Class") {
					continue;
				} else {
					BSONObject tmpObj = typeToBson(propObj, ignoreNullValue);
					if (!ignoreNullValue || null != tmpObj) {
						result.put(p.getName(), tmpObj);
					}
				}
			}
		}

		return result;
	}

	@SuppressWarnings({ "rawtypes" })
	public static BSONObject typeToBson(Object object)
			throws IntrospectionException, IllegalArgumentException,
			IllegalAccessException, InvocationTargetException {

		return typeToBson(object, false);
	}

	/**
	 * Returns this object's fields' names
	 * 
	 * @return The names of the fields in this object
	 */
	@Override
	public Set<String> keySet() {
		return _objectMap.keySet();
	}

	/**
	 * Returns a Set view of the mappings contained in this map
	 * 
	 * @return Returns a Set view of the mappings contained in this map.
	 */
	public Set<Entry<String, Object>> entrySet() {
		return _objectMap.entrySet();
	}

	/**
	 * Returns the number of key-value mappings in this map. If the map contains
	 * more than Integer.MAX_VALUE elements, returns Integer.MAX_VALUE.
	 * 
	 * @return the number of key-value mappings in this map
	 */
	@Override
	public int size() {
		return _objectMap.size();
	}

	/**
	 * Returns true if this map contains a mapping for the specified key. More
	 * formally, returns true if and only if this map contains a mapping for a
	 * key k such that (key==null ? k==null : key.equals(k)). (There can be at
	 * most one such mapping.)
	 * 
	 * @param key
	 *            whose presence in this map is to be tested
	 * @return true if this map contains a mapping for the specified key
	 */
	@Override
	public boolean containsKey(Object key) {
		return _objectMap.containsKey(key);
	}

	/**
	 * Returns true if this map maps one or more keys to the specified value.
	 * More formally, returns true if and only if this map contains at least one
	 * mapping to a value v such that (value==null ? v==null : value.equals(v)).
	 * This operation will probably require time linear in the map size for most
	 * implementations of the Map interface.
	 * 
	 * @param value
	 *            whose presence in this map is to be tested
	 * @return true if this map maps one or more keys to the specified value
	 */
	@Override
	public boolean containsValue(Object value) {
		return _objectMap.containsValue(value);
	}

	/**
	 * Removes the mapping for a key from this map if it is present (optional
	 * operation). More formally, if this map contains a mapping from key k to
	 * value v such that (key==null ? k==null : key.equals(k)), that mapping is
	 * removed. (The map can contain at most one such mapping.) Returns the
	 * value to which this map previously associated the key, or null if the map
	 * contained no mapping for the key. If this map permits null values, then a
	 * return value of null does not necessarily indicate that the map contained
	 * no mapping for the key; it's also possible that the map explicitly mapped
	 * the key to null. The map will not contain a mapping for the specified key
	 * once the call returns.
	 * 
	 * @param key
	 *            whose mapping is to be removed from the map
	 * @return the previous value associated with key, or null if there was no
	 *         mapping for key.
	 * 
	 */
	@Override
	public Object remove(Object key) {
		return _objectMap.remove(key);
	}

	/**
	 * Removes all of the mappings from this map (optional operation). The map
	 * will be empty after this call returns.
	 */
	@Override
	public void clear() {
		_objectMap.clear();
	}

	/**
	 * Returns a Collection view of the values contained in this map. The
	 * collection is backed by the map, so changes to the map are reflected in
	 * the collection, and vice-versa. If the map is modified while an iteration
	 * over the collection is in progress (except through the iterator's own
	 * remove operation), the results of the iteration are undefined. The
	 * collection supports element removal, which removes the corresponding
	 * mapping from the map, via the Iterator.remove, Collection.remove,
	 * removeAll, retainAll and clear operations. It does not support the add or
	 * addAll operations.
	 * 
	 * @return a collection view of the values contained in this map
	 */
	@Override
	public Collection<Object> values() {
		return _objectMap.values();
	}

	/**
	 * Returns the value to which the specified key is mapped, or null if the
	 * map contains no mapping for the key.
	 * 
	 * @param key
	 *            the key whose associated value is to be returned.
	 * @return the value or null
	 */
	@Override
	public Object get(Object key) {
		return _objectMap.get(key);
	}

}
