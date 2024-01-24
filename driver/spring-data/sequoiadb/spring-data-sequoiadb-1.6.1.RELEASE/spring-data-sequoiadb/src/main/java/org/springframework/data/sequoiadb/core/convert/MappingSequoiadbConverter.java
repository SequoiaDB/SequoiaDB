/*
 * Copyright 2011-2014 by the original author(s).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.springframework.data.sequoiadb.core.convert;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.BeansException;
import org.springframework.context.ApplicationContext;
import org.springframework.context.ApplicationContextAware;
import org.springframework.core.convert.ConversionException;
import org.springframework.core.convert.ConversionService;
import org.springframework.core.convert.support.DefaultConversionService;
import org.springframework.data.convert.CollectionFactory;
import org.springframework.data.convert.EntityInstantiator;
import org.springframework.data.convert.TypeMapper;
import org.springframework.data.mapping.Association;
import org.springframework.data.mapping.AssociationHandler;
import org.springframework.data.mapping.PreferredConstructor.Parameter;
import org.springframework.data.mapping.PropertyHandler;
import org.springframework.data.mapping.context.MappingContext;
import org.springframework.data.mapping.model.BeanWrapper;
import org.springframework.data.mapping.model.DefaultSpELExpressionEvaluator;
import org.springframework.data.mapping.model.MappingException;
import org.springframework.data.mapping.model.ParameterValueProvider;
import org.springframework.data.mapping.model.PersistentEntityParameterValueProvider;
import org.springframework.data.mapping.model.PropertyValueProvider;
import org.springframework.data.mapping.model.SpELContext;
import org.springframework.data.mapping.model.SpELExpressionEvaluator;
import org.springframework.data.mapping.model.SpELExpressionParameterValueProvider;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.assist.*;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.data.util.ClassTypeInformation;
import org.springframework.data.util.TypeInformation;
import org.springframework.expression.spel.standard.SpelExpressionParser;
import org.springframework.util.Assert;
import org.springframework.util.ClassUtils;
import org.springframework.util.CollectionUtils;

/**
 * {@link SequoiadbConverter} that uses a {@link MappingContext} to do sophisticated mapping of domain objects to
 * {@link BSONObject}.
 */
public class MappingSequoiadbConverter extends AbstractSequoiadbConverter implements ApplicationContextAware, ValueResolver {

	private static final String INCOMPATIBLE_TYPES = "Cannot convert %1$s of type %2$s into an instance of %3$s! Implement a custom Converter<%2$s, %3$s> and register it with the CustomConversions. Parent object was: %4$s";

	protected static final Logger LOGGER = LoggerFactory.getLogger(MappingSequoiadbConverter.class);

	protected final MappingContext<? extends SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> mappingContext;
	protected final SpelExpressionParser spelExpressionParser = new SpelExpressionParser();
	protected final QueryMapper idMapper;
	protected final DbRefResolver dbRefResolver;

	protected ApplicationContext applicationContext;
	protected SequoiadbTypeMapper typeMapper;
	protected String mapKeyDotReplacement = null;

	private SpELContext spELContext;

	/**
	 * Creates a new {@link MappingSequoiadbConverter} given the new {@link DbRefResolver} and {@link MappingContext}.
	 * 
	 * @param sequoiadbDbFactory must not be {@literal null}.
	 * @param mappingContext must not be {@literal null}.
	 */
	public MappingSequoiadbConverter(DbRefResolver dbRefResolver,
                                     MappingContext<? extends SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> mappingContext) {

		super(new DefaultConversionService());

		Assert.notNull(dbRefResolver, "DbRefResolver must not be null!");
		Assert.notNull(mappingContext, "MappingContext must not be null!");

		this.dbRefResolver = dbRefResolver;
		this.mappingContext = mappingContext;
		this.typeMapper = new DefaultSequoiadbTypeMapper(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY, mappingContext);
		this.idMapper = new QueryMapper(this);

		this.spELContext = new SpELContext(DBObjectPropertyAccessor.INSTANCE);
	}

	/**
	 * Creates a new {@link MappingSequoiadbConverter} given the new {@link SequoiadbFactory} and {@link MappingContext}.
	 * 
	 * @deprecated use the constructor taking a {@link DbRefResolver} instead.
	 * @param sequoiadbFactory must not be {@literal null}.
	 * @param mappingContext must not be {@literal null}.
	 */
	@Deprecated
	public MappingSequoiadbConverter(SequoiadbFactory sequoiadbFactory,
                                     MappingContext<? extends SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> mappingContext) {
		this(new DefaultDbRefResolver(sequoiadbFactory), mappingContext);
	}

	/**
	 * Configures the {@link SequoiadbTypeMapper} to be used to add type information to {@link BSONObject}s created by the
	 * converter and how to lookup type information from {@link BSONObject}s when reading them. Uses a
	 * {@link DefaultSequoiadbTypeMapper} by default. Setting this to {@literal null} will reset the {@link TypeMapper} to the
	 * default one.
	 * 
	 * @param typeMapper the typeMapper to set
	 */
	public void setTypeMapper(SequoiadbTypeMapper typeMapper) {
		this.typeMapper = typeMapper == null ? new DefaultSequoiadbTypeMapper(DefaultSequoiadbTypeMapper.DEFAULT_TYPE_KEY,
				mappingContext) : typeMapper;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.convert.SequoiadbConverter#getTypeMapper()
	 */
	@Override
	public SequoiadbTypeMapper getTypeMapper() {
		return this.typeMapper;
	}

	/**
	 * Configure the characters dots potentially contained in a {@link Map} shall be replaced with. By default we don't do
	 * any translation but rather reject a {@link Map} with keys containing dots causing the conversion for the entire
	 * object to fail. If further customization of the translation is needed, have a look at
	 * {@link #potentiallyEscapeMapKey(String)} as well as {@link #potentiallyUnescapeMapKey(String)}.
	 * 
	 * @param mapKeyDotReplacement the mapKeyDotReplacement to set
	 */
	public void setMapKeyDotReplacement(String mapKeyDotReplacement) {
		this.mapKeyDotReplacement = mapKeyDotReplacement;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.convert.EntityConverter#getMappingContext()
	 */
	public MappingContext<? extends SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> getMappingContext() {
		return mappingContext;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.context.ApplicationContextAware#setApplicationContext(org.springframework.context.ApplicationContext)
	 */
	public void setApplicationContext(ApplicationContext applicationContext) throws BeansException {

		this.applicationContext = applicationContext;
		this.spELContext = new SpELContext(this.spELContext, applicationContext);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.core.SequoiadbReader#read(java.lang.Class, BSONObject)
	 */
	public <S extends Object> S read(Class<S> clazz, final BSONObject dbo) {
		return read(ClassTypeInformation.from(clazz), dbo);
	}

	protected <S extends Object> S read(TypeInformation<S> type, BSONObject dbo) {
		return read(type, dbo, ObjectPath.ROOT);
	}

	@SuppressWarnings("unchecked")
	private <S extends Object> S read(TypeInformation<S> type, BSONObject dbo, ObjectPath path) {

		if (null == dbo) {
			return null;
		}

		TypeInformation<? extends S> typeToUse = typeMapper.readType(dbo, type);
		Class<? extends S> rawType = typeToUse.getType();

		if (conversions.hasCustomReadTarget(dbo.getClass(), rawType)) {
			return conversionService.convert(dbo, rawType);
		}

		if (BSONObject.class.isAssignableFrom(rawType)) {
			return (S) dbo;
		}

		if (typeToUse.isCollectionLike() && dbo instanceof BasicBSONList) {
			return (S) readCollectionOrArray(typeToUse, (BasicBSONList) dbo, path);
		}

		if (typeToUse.isMap()) {
			return (S) readMap(typeToUse, dbo, path);
		}

		if (dbo instanceof BasicBSONList) {
			throw new MappingException(String.format(INCOMPATIBLE_TYPES, dbo, BasicBSONList.class, typeToUse.getType(), path));
		}

		// Retrieve persistent entity info
		SequoiadbPersistentEntity<S> persistentEntity = (SequoiadbPersistentEntity<S>) mappingContext
				.getPersistentEntity(typeToUse);
		if (persistentEntity == null) {
			throw new MappingException("No mapping metadata found for " + rawType.getName());
		}

		return read(persistentEntity, dbo, path);
	}

	private ParameterValueProvider<SequoiadbPersistentProperty> getParameterProvider(SequoiadbPersistentEntity<?> entity,
																					 BSONObject source, DefaultSpELExpressionEvaluator evaluator, ObjectPath path) {

		SequoiadbPropertyValueProvider provider = new SequoiadbPropertyValueProvider(source, evaluator, path);
		PersistentEntityParameterValueProvider<SequoiadbPersistentProperty> parameterProvider = new PersistentEntityParameterValueProvider<SequoiadbPersistentProperty>(
				entity, provider, path.getCurrentObject());

		return new ConverterAwareSpELExpressionParameterValueProvider(evaluator, conversionService, parameterProvider, path);
	}

	private <S extends Object> S read(final SequoiadbPersistentEntity<S> entity, final BSONObject dbo, final ObjectPath path) {

		final DefaultSpELExpressionEvaluator evaluator = new DefaultSpELExpressionEvaluator(dbo, spELContext);

		ParameterValueProvider<SequoiadbPersistentProperty> provider = getParameterProvider(entity, dbo, evaluator, path);
		EntityInstantiator instantiator = instantiators.getInstantiatorFor(entity);
		S instance = instantiator.createInstance(entity, provider);

		final BeanWrapper<S> wrapper = BeanWrapper.create(instance, conversionService);
		final SequoiadbPersistentProperty idProperty = entity.getIdProperty();
		final S result = wrapper.getBean();

		// make sure id property is set before all other properties
		Object idValue = null;

		if (idProperty != null) {
			idValue = getValueInternal(idProperty, dbo, evaluator, path);
			wrapper.setProperty(idProperty, idValue);
		}

		final ObjectPath currentPath = path.push(result, entity, idValue);

		// Set properties not already set in the constructor
		entity.doWithProperties(new PropertyHandler<SequoiadbPersistentProperty>() {
			public void doWithPersistentProperty(SequoiadbPersistentProperty prop) {

				// we skip the id property since it was already set
				if (idProperty != null && idProperty.equals(prop)) {
					return;
				}

				if (!dbo.containsField(prop.getFieldName()) || entity.isConstructorArgument(prop)) {
					return;
				}

				wrapper.setProperty(prop, getValueInternal(prop, dbo, evaluator, currentPath));
			}
		});

		// Handle associations
		entity.doWithAssociations(new AssociationHandler<SequoiadbPersistentProperty>() {
			public void doWithAssociation(Association<SequoiadbPersistentProperty> association) {

				final SequoiadbPersistentProperty property = association.getInverse();
				Object value = dbo.get(property.getFieldName());

				if (value == null) {
					return;
				}

				DBRef dbref = value instanceof DBRef ? (DBRef) value : null;

				DbRefProxyHandler handler = new DefaultDbRefProxyHandler(spELContext, mappingContext,
						MappingSequoiadbConverter.this);
				DbRefResolverCallback callback = new DefaultDbRefResolverCallback(dbo, currentPath, evaluator,
						MappingSequoiadbConverter.this);

				wrapper.setProperty(property, dbRefResolver.resolveDbRef(property, dbref, callback, handler));
			}
		});

		return result;
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.convert.SequoiadbWriter#toDBRef(java.lang.Object, org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty)
	 */
	public DBRef toDBRef(Object object, SequoiadbPersistentProperty referingProperty) {

		org.springframework.data.sequoiadb.core.mapping.DBRef annotation = null;

		if (referingProperty != null) {
			annotation = referingProperty.getDBRef();
			Assert.isTrue(annotation != null, "The referenced property has to be mapped with @DBRef!");
		}

		// @see DATA_JIRA-913
		if (object instanceof LazyLoadingProxy) {
			return ((LazyLoadingProxy) object).toDBRef();
		}

		return createDBRef(object, referingProperty);
	}

	/**
	 * Root entry method into write conversion. Adds a type discriminator to the {@link BSONObject}. Shouldn't be called for
	 * nested conversions.
	 * 
	 * @see org.springframework.data.sequoiadb.core.core.convert.SequoiadbWriter#write(java.lang.Object, BSONObject)
	 */
	public void write(final Object obj, final BSONObject dbo) {

		if (null == obj) {
			return;
		}

		Class<?> entityType = obj.getClass();
		boolean handledByCustomConverter = conversions.getCustomWriteTarget(entityType, BSONObject.class) != null;
		TypeInformation<? extends Object> type = ClassTypeInformation.from(entityType);

		if (!handledByCustomConverter && !(dbo instanceof BasicBSONList)) {
			typeMapper.writeType(type, dbo);
		}

		Object target = obj instanceof LazyLoadingProxy ? ((LazyLoadingProxy) obj).getTarget() : obj;

		writeInternal(target, dbo, type);
	}

	/**
	 * Internal write conversion method which should be used for nested invocations.
	 * 
	 * @param obj
	 * @param dbo
	 */
	@SuppressWarnings("unchecked")
	protected void writeInternal(final Object obj, final BSONObject dbo, final TypeInformation<?> typeHint) {

		if (null == obj) {
			return;
		}

		Class<?> entityType = obj.getClass();
		Class<?> customTarget = conversions.getCustomWriteTarget(entityType, BSONObject.class);

		if (customTarget != null) {
			BSONObject result = conversionService.convert(obj, BSONObject.class);
			dbo.putAll(result);
			return;
		}

		if (Map.class.isAssignableFrom(entityType)) {
			writeMapInternal((Map<Object, Object>) obj, dbo, ClassTypeInformation.MAP);
			return;
		}

		if (Collection.class.isAssignableFrom(entityType)) {
			writeCollectionInternal((Collection<?>) obj, ClassTypeInformation.LIST, (BasicBSONList) dbo);
			return;
		}

		SequoiadbPersistentEntity<?> entity = mappingContext.getPersistentEntity(entityType);
		writeInternal(obj, dbo, entity);
		addCustomTypeKeyIfNecessary(typeHint, obj, dbo);
	}

	protected void writeInternal(Object obj, final BSONObject dbo, SequoiadbPersistentEntity<?> entity) {

		if (obj == null) {
			return;
		}

		if (null == entity) {
			throw new MappingException("No mapping metadata found for entity of type " + obj.getClass().getName());
		}

		final BeanWrapper<Object> wrapper = BeanWrapper.create(obj, conversionService);
		final SequoiadbPersistentProperty idProperty = entity.getIdProperty();

		if (!dbo.containsField("_id") && null != idProperty) {

			try {
				Object id = wrapper.getProperty(idProperty, Object.class);
				dbo.put("_id", idMapper.convertId(id));
			} catch (ConversionException ignored) {}
		}

		// Write the properties
		entity.doWithProperties(new PropertyHandler<SequoiadbPersistentProperty>() {
			public void doWithPersistentProperty(SequoiadbPersistentProperty prop) {

				if (prop.equals(idProperty) || !prop.isWritable()) {
					return;
				}

				Object propertyObj = wrapper.getProperty(prop);

				if (null != propertyObj) {

					if (!conversions.isSimpleType(propertyObj.getClass())) {
						writePropertyInternal(propertyObj, dbo, prop);
					} else {
						writeSimpleInternal(propertyObj, dbo, prop);
					}
				}
			}
		});

		entity.doWithAssociations(new AssociationHandler<SequoiadbPersistentProperty>() {
			public void doWithAssociation(Association<SequoiadbPersistentProperty> association) {
				SequoiadbPersistentProperty inverseProp = association.getInverse();
				Class<?> type = inverseProp.getType();
				Object propertyObj = wrapper.getProperty(inverseProp, type);
				if (null != propertyObj) {
					writePropertyInternal(propertyObj, dbo, inverseProp);
				}
			}
		});
	}

	@SuppressWarnings({ "unchecked" })
	protected void writePropertyInternal(Object obj, BSONObject dbo, SequoiadbPersistentProperty prop) {

		if (obj == null) {
			return;
		}

		DBObjectAccessor accessor = new DBObjectAccessor(dbo);

		TypeInformation<?> valueType = ClassTypeInformation.from(obj.getClass());
		TypeInformation<?> type = prop.getTypeInformation();

		if (valueType.isCollectionLike()) {
			BSONObject collectionInternal = createCollection(asCollection(obj), prop);
			accessor.put(prop, collectionInternal);
			return;
		}

		if (valueType.isMap()) {
			BSONObject mapDbObj = createMap((Map<Object, Object>) obj, prop);
			accessor.put(prop, mapDbObj);
			return;
		}

		if (prop.isDbReference()) {

			DBRef dbRefObj = null;

			/*
			 * If we already have a LazyLoadingProxy, we use it's cached DBRef value instead of 
			 * unnecessarily initializing it only to convert it to a DBRef a few instructions later.
			 */
			if (obj instanceof LazyLoadingProxy) {
				dbRefObj = ((LazyLoadingProxy) obj).toDBRef();
			}

			dbRefObj = dbRefObj != null ? dbRefObj : createDBRef(obj, prop);

			if (null != dbRefObj) {
				accessor.put(prop, dbRefObj);
				return;
			}
		}

		/*
		 * If we have a LazyLoadingProxy we make sure it is initialized first.
		 */
		if (obj instanceof LazyLoadingProxy) {
			obj = ((LazyLoadingProxy) obj).getTarget();
		}

		// Lookup potential custom target type
		Class<?> basicTargetType = conversions.getCustomWriteTarget(obj.getClass(), null);

		if (basicTargetType != null) {
			accessor.put(prop, conversionService.convert(obj, basicTargetType));
			return;
		}

		Object existingValue = accessor.get(prop);
		BasicBSONObject propDbObj = existingValue instanceof BasicBSONObject ? (BasicBSONObject) existingValue
				: new BasicBSONObject();
		addCustomTypeKeyIfNecessary(type, obj, propDbObj);

		SequoiadbPersistentEntity<?> entity = isSubtype(prop.getType(), obj.getClass()) ? mappingContext
				.getPersistentEntity(obj.getClass()) : mappingContext.getPersistentEntity(type);

		writeInternal(obj, propDbObj, entity);
		accessor.put(prop, propDbObj);
	}

	private boolean isSubtype(Class<?> left, Class<?> right) {
		return left.isAssignableFrom(right) && !left.equals(right);
	}

	/**
	 * Returns given object as {@link Collection}. Will return the {@link Collection} as is if the source is a
	 * {@link Collection} already, will convert an array into a {@link Collection} or simply create a single element
	 * collection for everything else.
	 * 
	 * @param source
	 * @return
	 */
	private static Collection<?> asCollection(Object source) {

		if (source instanceof Collection) {
			return (Collection<?>) source;
		}

		return source.getClass().isArray() ? CollectionUtils.arrayToList(source) : Collections.singleton(source);
	}

	/**
	 * Writes the given {@link Collection} using the given {@link SequoiadbPersistentProperty} information.
	 * 
	 * @param collection must not be {@literal null}.
	 * @param property must not be {@literal null}.
	 * @return
	 */
	protected BSONObject createCollection(Collection<?> collection, SequoiadbPersistentProperty property) {

		if (!property.isDbReference()) {
			return writeCollectionInternal(collection, property.getTypeInformation(), new BasicBSONList());
		}

		BasicBSONList dbList = new BasicBSONList();

		for (Object element : collection) {

			if (element == null) {
				continue;
			}

			DBRef dbRef = createDBRef(element, property);
			dbList.add(dbRef);
		}

		return dbList;
	}

	/**
	 * Writes the given {@link Map} using the given {@link SequoiadbPersistentProperty} information.
	 * 
	 * @param map must not {@literal null}.
	 * @param property must not be {@literal null}.
	 * @return
	 */
	protected BSONObject createMap(Map<Object, Object> map, SequoiadbPersistentProperty property) {

		Assert.notNull(map, "Given map must not be null!");
		Assert.notNull(property, "PersistentProperty must not be null!");

		if (!property.isDbReference()) {
			return writeMapInternal(map, new BasicBSONObject(), property.getTypeInformation());
		}

		BasicBSONObject dbObject = new BasicBSONObject();

		for (Map.Entry<Object, Object> entry : map.entrySet()) {

			Object key = entry.getKey();
			Object value = entry.getValue();

			if (conversions.isSimpleType(key.getClass())) {

				String simpleKey = potentiallyEscapeMapKey(key.toString());
				dbObject.put(simpleKey, value != null ? createDBRef(value, property) : null);

			} else {
				throw new MappingException("Cannot use a complex object as a key value.");
			}
		}

		return dbObject;
	}

	/**
	 * Populates the given {@link BasicBSONList} with values from the given {@link Collection}.
	 * 
	 * @param source the collection to create a {@link BasicBSONList} for, must not be {@literal null}.
	 * @param type the {@link TypeInformation} to consider or {@literal null} if unknown.
	 * @param sink the {@link BasicBSONList} to write to.
	 * @return
	 */
	private BasicBSONList writeCollectionInternal(Collection<?> source, TypeInformation<?> type, BasicBSONList sink) {

		TypeInformation<?> componentType = type == null ? null : type.getComponentType();

		for (Object element : source) {

			Class<?> elementType = element == null ? null : element.getClass();

			if (elementType == null || conversions.isSimpleType(elementType)) {
				sink.add(getPotentiallyConvertedSimpleWrite(element));
			} else if (element instanceof Collection || elementType.isArray()) {
				sink.add(writeCollectionInternal(asCollection(element), componentType, new BasicBSONList()));
			} else {
				BasicBSONObject propDbObj = new BasicBSONObject();
				writeInternal(element, propDbObj, componentType);
				sink.add(propDbObj);
			}
		}

		return sink;
	}

	/**
	 * Writes the given {@link Map} to the given {@link BSONObject} considering the given {@link TypeInformation}.
	 * 
	 * @param obj must not be {@literal null}.
	 * @param dbo must not be {@literal null}.
	 * @param propertyType must not be {@literal null}.
	 * @return
	 */
	protected BSONObject writeMapInternal(Map<Object, Object> obj, BSONObject dbo, TypeInformation<?> propertyType) {

		for (Map.Entry<Object, Object> entry : obj.entrySet()) {
			Object key = entry.getKey();
			Object val = entry.getValue();
			if (conversions.isSimpleType(key.getClass())) {
				// Don't use conversion service here as removal of ObjectToString converter results in some primitive types not
				// being convertable
				String simpleKey = potentiallyEscapeMapKey(key.toString());
				if (val == null || conversions.isSimpleType(val.getClass())) {
					writeSimpleInternal(val, dbo, simpleKey);
				} else if (val instanceof Collection || val.getClass().isArray()) {
					dbo.put(simpleKey,
							writeCollectionInternal(asCollection(val), propertyType.getMapValueType(), new BasicBSONList()));
				} else {
					BSONObject newDbo = new BasicBSONObject();
					TypeInformation<?> valueTypeInfo = propertyType.isMap() ? propertyType.getMapValueType()
							: ClassTypeInformation.OBJECT;
					writeInternal(val, newDbo, valueTypeInfo);
					dbo.put(simpleKey, newDbo);
				}
			} else {
				throw new MappingException("Cannot use a complex object as a key value.");
			}
		}

		return dbo;
	}

	/**
	 * Potentially replaces dots in the given map key with the configured map key replacement if configured or aborts
	 * conversion if none is configured.
	 * 
	 * @see #setMapKeyDotReplacement(String)
	 * @param source
	 * @return
	 */
	protected String potentiallyEscapeMapKey(String source) {

		if (!source.contains(".")) {
			return source;
		}

		if (mapKeyDotReplacement == null) {
			throw new MappingException(String.format("Map key %s contains dots but no replacement was configured! Make "
					+ "sure map keys don't contain dots in the first place or configure an appropriate replacement!", source));
		}

		return source.replaceAll("\\.", mapKeyDotReplacement);
	}

	/**
	 * Translates the map key replacements in the given key just read with a dot in case a map key replacement has been
	 * configured.
	 * 
	 * @param source
	 * @return
	 */
	protected String potentiallyUnescapeMapKey(String source) {
		return mapKeyDotReplacement == null ? source : source.replaceAll(mapKeyDotReplacement, "\\.");
	}

	/**
	 * Adds custom type information to the given {@link BSONObject} if necessary. That is if the value is not the same as
	 * the one given. This is usually the case if you store a subtype of the actual declared type of the property.
	 * 
	 * @param type
	 * @param value must not be {@literal null}.
	 * @param dbObject must not be {@literal null}.
	 */
	protected void addCustomTypeKeyIfNecessary(TypeInformation<?> type, Object value, BSONObject dbObject) {

		TypeInformation<?> actualType = type != null ? type.getActualType() : null;
		Class<?> reference = actualType == null ? Object.class : actualType.getType();
		Class<?> valueType = ClassUtils.getUserClass(value.getClass());

		boolean notTheSameClass = !valueType.equals(reference);
		if (notTheSameClass) {
			typeMapper.writeType(valueType, dbObject);
		}
	}

	/**
	 * Writes the given simple value to the given {@link BSONObject}. Will store enum names for enum values.
	 * 
	 * @param value
	 * @param dbObject must not be {@literal null}.
	 * @param key must not be {@literal null}.
	 */
	private void writeSimpleInternal(Object value, BSONObject dbObject, String key) {
		dbObject.put(key, getPotentiallyConvertedSimpleWrite(value));
	}

	private void writeSimpleInternal(Object value, BSONObject dbObject, SequoiadbPersistentProperty property) {
		DBObjectAccessor accessor = new DBObjectAccessor(dbObject);
		accessor.put(property, getPotentiallyConvertedSimpleWrite(value));
	}

	/**
	 * Checks whether we have a custom conversion registered for the given value into an arbitrary simple Sdb type.
	 * Returns the converted value if so. If not, we perform special enum handling or simply return the value as is.
	 * 
	 * @param value
	 * @return
	 */
	private Object getPotentiallyConvertedSimpleWrite(Object value) {

		if (value == null) {
			return null;
		}

		Class<?> customTarget = conversions.getCustomWriteTarget(value.getClass(), null);

		if (customTarget != null) {
			return conversionService.convert(value, customTarget);
		} else {
			return Enum.class.isAssignableFrom(value.getClass()) ? ((Enum<?>) value).name() : value;
		}
	}

	/**
	 * Checks whether we have a custom conversion for the given simple object. Converts the given value if so, applies
	 * {@link Enum} handling or returns the value as is.
	 * 
	 * @param value
	 * @param target must not be {@literal null}.
	 * @return
	 */
	@SuppressWarnings({ "rawtypes", "unchecked" })
	private Object getPotentiallyConvertedSimpleRead(Object value, Class<?> target) {

		if (value == null || target == null) {
			return value;
		}

		if (conversions.hasCustomReadTarget(value.getClass(), target)) {
			return conversionService.convert(value, target);
		}

		if (Enum.class.isAssignableFrom(target)) {
			return Enum.valueOf((Class<Enum>) target, value.toString());
		}

		return target.isAssignableFrom(value.getClass()) ? value : conversionService.convert(value, target);
	}

	protected DBRef createDBRef(Object target, SequoiadbPersistentProperty property) {

		Assert.notNull(target);

		if (target instanceof DBRef) {
			return (DBRef) target;
		}

		SequoiadbPersistentEntity<?> targetEntity = mappingContext.getPersistentEntity(target.getClass());
		targetEntity = targetEntity == null ? targetEntity = mappingContext.getPersistentEntity(property) : targetEntity;

		if (null == targetEntity) {
			throw new MappingException("No mapping metadata found for " + target.getClass());
		}

		SequoiadbPersistentProperty idProperty = targetEntity.getIdProperty();

		if (idProperty == null) {
			throw new MappingException("No id property found on class " + targetEntity.getType());
		}

		Object id = null;

		if (target.getClass().equals(idProperty.getType())) {
			id = target;
		} else {
			BeanWrapper<Object> wrapper = BeanWrapper.create(target, conversionService);
			id = wrapper.getProperty(idProperty, Object.class);
		}

		if (null == id) {
			throw new MappingException("Cannot create a reference to an object with a NULL id.");
		}

		return dbRefResolver.createDbRef(property == null ? null : property.getDBRef(), targetEntity,
				idMapper.convertId(id));
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.convert.ValueResolver#getValueInternal(org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty, BSONObject, org.springframework.data.mapping.model.SpELExpressionEvaluator, java.lang.Object)
	 */
	@Override
	public Object getValueInternal(SequoiadbPersistentProperty prop, BSONObject dbo, SpELExpressionEvaluator evaluator,
								   ObjectPath path) {
		return new SequoiadbPropertyValueProvider(dbo, evaluator, path).getPropertyValue(prop);
	}

	/**
	 * Reads the given {@link BasicBSONList} into a collection of the given {@link TypeInformation}.
	 * 
	 * @param targetType must not be {@literal null}.
	 * @param sourceValue must not be {@literal null}.
	 * @param path must not be {@literal null}.
	 * @return the converted {@link Collection} or array, will never be {@literal null}.
	 */
	private Object readCollectionOrArray(TypeInformation<?> targetType, BasicBSONList sourceValue, ObjectPath path) {

		Assert.notNull(targetType, "Target type must not be null!");
		Assert.notNull(path, "Object path must not be null!");

		Class<?> collectionType = targetType.getType();

		if (sourceValue.isEmpty()) {
			return getPotentiallyConvertedSimpleRead(new HashSet<Object>(), collectionType);
		}

		TypeInformation<?> componentType = targetType.getComponentType();
		Class<?> rawComponentType = componentType == null ? null : componentType.getType();

		collectionType = Collection.class.isAssignableFrom(collectionType) ? collectionType : List.class;
		Collection<Object> items = targetType.getType().isArray() ? new ArrayList<Object>() : CollectionFactory
				.createCollection(collectionType, rawComponentType, sourceValue.size());

		for (int i = 0; i < sourceValue.size(); i++) {

			Object dbObjItem = sourceValue.get(i);

			if (dbObjItem instanceof DBRef) {
				items.add(DBRef.class.equals(rawComponentType) ? dbObjItem : read(componentType, readRef((DBRef) dbObjItem),
						path));
			} else if (dbObjItem instanceof BSONObject) {
				items.add(read(componentType, (BSONObject) dbObjItem, path));
			} else {
				items.add(getPotentiallyConvertedSimpleRead(dbObjItem, rawComponentType));
			}
		}

		return getPotentiallyConvertedSimpleRead(items, targetType.getType());
	}

	/**
	 * Reads the given {@link BSONObject} into a {@link Map}. will recursively resolve nested {@link Map}s as well.
	 * 
	 * @param type the {@link Map} {@link TypeInformation} to be used to unmarshall this {@link BSONObject}.
	 * @param dbObject must not be {@literal null}
	 * @param path must not be {@literal null}
	 * @return
	 */
	@SuppressWarnings("unchecked")
	protected Map<Object, Object> readMap(TypeInformation<?> type, BSONObject dbObject, ObjectPath path) {

		Assert.notNull(dbObject, "BSONObject must not be null!");
		Assert.notNull(path, "Object path must not be null!");

		Class<?> mapType = typeMapper.readType(dbObject, type).getType();

		TypeInformation<?> keyType = type.getComponentType();
		Class<?> rawKeyType = keyType == null ? null : keyType.getType();

		TypeInformation<?> valueType = type.getMapValueType();
		Class<?> rawValueType = valueType == null ? null : valueType.getType();

		Map<Object, Object> map = CollectionFactory.createMap(mapType, rawKeyType, dbObject.keySet().size());
		Map<String, Object> sourceMap = dbObject.toMap();

		for (Entry<String, Object> entry : sourceMap.entrySet()) {
			if (typeMapper.isTypeKey(entry.getKey())) {
				continue;
			}

			Object key = potentiallyUnescapeMapKey(entry.getKey());

			if (rawKeyType != null) {
				key = conversionService.convert(key, rawKeyType);
			}

			Object value = entry.getValue();

			if (value instanceof BSONObject) {
				map.put(key, read(valueType, (BSONObject) value, path));
			} else if (value instanceof DBRef) {
				map.put(key, DBRef.class.equals(rawValueType) ? value : read(valueType, readRef((DBRef) value)));
			} else {
				Class<?> valueClass = valueType == null ? null : valueType.getType();
				map.put(key, getPotentiallyConvertedSimpleRead(value, valueClass));
			}
		}

		return map;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.convert.SequoiadbWriter#convertToSequoiadbType(java.lang.Object, org.springframework.data.util.TypeInformation)
	 */
	@SuppressWarnings("unchecked")
	public Object convertToSequoiadbType(Object obj, TypeInformation<?> typeInformation) {

		if (obj == null) {
			return null;
		}

		Class<?> target = conversions.getCustomWriteTarget(obj.getClass());
		if (target != null) {
			return conversionService.convert(obj, target);
		}

		if (conversions.isSimpleType(obj.getClass())) {
			// Doesn't need conversion
			return getPotentiallyConvertedSimpleWrite(obj);
		}

		TypeInformation<?> typeHint = typeInformation == null ? ClassTypeInformation.OBJECT : typeInformation;

		if (obj instanceof BasicBSONList) {
			return maybeConvertList((BasicBSONList) obj, typeHint);
		}

		if (obj instanceof BSONObject) {
			BSONObject newValueDbo = new BasicBSONObject();
			for (String vk : ((BSONObject) obj).keySet()) {
				Object o = ((BSONObject) obj).get(vk);
				newValueDbo.put(vk, convertToSequoiadbType(o, typeHint));
			}
			return newValueDbo;
		}

		if (obj instanceof Map) {
			BSONObject result = new BasicBSONObject();
			for (Map.Entry<Object, Object> entry : ((Map<Object, Object>) obj).entrySet()) {
				result.put(entry.getKey().toString(), convertToSequoiadbType(entry.getValue(), typeHint));
			}
			return result;
		}

		if (obj.getClass().isArray()) {
			return maybeConvertList(Arrays.asList((Object[]) obj), typeHint);
		}

		if (obj instanceof Collection) {
			return maybeConvertList((Collection<?>) obj, typeHint);
		}

		BSONObject newDbo = new BasicBSONObject();
		this.write(obj, newDbo);

		if (typeInformation == null) {
			return removeTypeInfoRecursively(newDbo);
		}

		return !obj.getClass().equals(typeInformation.getType()) ? newDbo : removeTypeInfoRecursively(newDbo);
	}

	public BasicBSONList maybeConvertList(Iterable<?> source, TypeInformation<?> typeInformation) {

		BasicBSONList newDbl = new BasicBSONList();
		for (Object element : source) {
			newDbl.add(convertToSequoiadbType(element, typeInformation));
		}

		return newDbl;
	}

	/**
	 * Removes the type information from the conversion result.
	 * 
	 * @param object
	 * @return
	 */
	private Object removeTypeInfoRecursively(Object object) {

		if (!(object instanceof BSONObject)) {
			return object;
		}

		BSONObject dbObject = (BSONObject) object;
		String keyToRemove = null;
		for (String key : dbObject.keySet()) {

			if (typeMapper.isTypeKey(key)) {
				keyToRemove = key;
			}

			Object value = dbObject.get(key);
			if (value instanceof BasicBSONList) {
				for (Object element : (BasicBSONList) value) {
					removeTypeInfoRecursively(element);
				}
			} else {
				removeTypeInfoRecursively(value);
			}
		}

		if (keyToRemove != null) {
			dbObject.removeField(keyToRemove);
		}

		return dbObject;
	}

	/**
	 * {@link PropertyValueProvider} to evaluate a SpEL expression if present on the property or simply accesses the field
	 * of the configured source {@link BSONObject}.
	 *

	 */
	private class SequoiadbPropertyValueProvider implements PropertyValueProvider<SequoiadbPersistentProperty> {

		private final DBObjectAccessor source;
		private final SpELExpressionEvaluator evaluator;
		private final ObjectPath path;

		/**
		 * Creates a new {@link SequoiadbPropertyValueProvider} for the given source, {@link SpELExpressionEvaluator} and
		 * {@link ObjectPath}.
		 * 
		 * @param source must not be {@literal null}.
		 * @param evaluator must not be {@literal null}.
		 * @param path can be {@literal null}.
		 */
		public SequoiadbPropertyValueProvider(BSONObject source, SpELExpressionEvaluator evaluator, ObjectPath path) {

			Assert.notNull(source);
			Assert.notNull(evaluator);

			this.source = new DBObjectAccessor(source);
			this.evaluator = evaluator;
			this.path = path;
		}

		/* 
		 * (non-Javadoc)
		 * @see org.springframework.data.convert.PropertyValueProvider#getPropertyValue(org.springframework.data.mapping.PersistentProperty)
		 */
		public <T> T getPropertyValue(SequoiadbPersistentProperty property) {

			String expression = property.getSpelExpression();
			Object value = expression != null ? evaluator.evaluate(expression) : source.get(property);

			if (value == null) {
				return null;
			}

			return readValue(value, property.getTypeInformation(), path);
		}
	}

	/**
	 * Extension of {@link SpELExpressionParameterValueProvider} to recursively trigger value conversion on the raw
	 * resolved SpEL value.
	 * 

	 */
	private class ConverterAwareSpELExpressionParameterValueProvider extends
			SpELExpressionParameterValueProvider<SequoiadbPersistentProperty> {

		private final ObjectPath path;

		/**
		 * Creates a new {@link ConverterAwareSpELExpressionParameterValueProvider}.
		 * 
		 * @param evaluator must not be {@literal null}.
		 * @param conversionService must not be {@literal null}.
		 * @param delegate must not be {@literal null}.
		 */
		public ConverterAwareSpELExpressionParameterValueProvider(SpELExpressionEvaluator evaluator,
																  ConversionService conversionService, ParameterValueProvider<SequoiadbPersistentProperty> delegate, ObjectPath path) {

			super(evaluator, conversionService, delegate);
			this.path = path;
		}

		/* 
		 * (non-Javadoc)
		 * @see org.springframework.data.mapping.model.SpELExpressionParameterValueProvider#potentiallyConvertSpelValue(java.lang.Object, org.springframework.data.mapping.PreferredConstructor.Parameter)
		 */
		@Override
		protected <T> T potentiallyConvertSpelValue(Object object, Parameter<T, SequoiadbPersistentProperty> parameter) {
			return readValue(object, parameter.getType(), path);
		}
	}

	@SuppressWarnings("unchecked")
	private <T> T readValue(Object value, TypeInformation<?> type, ObjectPath path) {

		Class<?> rawType = type.getType();

		if (conversions.hasCustomReadTarget(value.getClass(), rawType)) {
			return (T) conversionService.convert(value, rawType);
		} else if (value instanceof DBRef) {
			return potentiallyReadOrResolveDbRef((DBRef) value, type, path, rawType);
		} else if (value instanceof BasicBSONList) {
			return (T) readCollectionOrArray(type, (BasicBSONList) value, path);
		} else if (value instanceof BSONObject) {
			return (T) read(type, (BSONObject) value, path);
		} else {
			return (T) getPotentiallyConvertedSimpleRead(value, rawType);
		}
	}

	@SuppressWarnings("unchecked")
	private <T> T potentiallyReadOrResolveDbRef(DBRef dbref, TypeInformation<?> type, ObjectPath path, Class<?> rawType) {

		if (rawType.equals(DBRef.class)) {
			return (T) dbref;
		}

		Object object = dbref == null ? null : path.getPathItem(dbref.getId(), dbref.getRef());

		if (object != null) {
			return (T) object;
		}

		return (T) (object != null ? object : read(type, readRef(dbref), path));
	}

	/**
	 * Performs the fetch operation for the given {@link DBRef}.
	 * 
	 * @param ref
	 * @return
	 */
	BSONObject readRef(DBRef ref) {
		return ref.fetch();
	}
}
