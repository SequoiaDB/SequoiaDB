/*
 * Copyright 2011-2014 the original author or authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
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
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map.Entry;
import java.util.Set;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.springframework.core.convert.ConversionException;
import org.springframework.core.convert.ConversionService;
import org.springframework.core.convert.converter.Converter;
import org.springframework.data.mapping.Association;
import org.springframework.data.mapping.PersistentEntity;
import org.springframework.data.mapping.PropertyPath;
import org.springframework.data.mapping.PropertyReferenceException;
import org.springframework.data.mapping.context.MappingContext;
import org.springframework.data.mapping.context.PersistentPropertyPath;
import org.springframework.data.mapping.model.MappingException;



import org.springframework.data.sequoiadb.assist.DBRef;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty.PropertyToFieldNameConverter;
import org.springframework.data.sequoiadb.core.query.Query;
import org.springframework.util.Assert;

/**
 * A helper class to encapsulate any modifications of a Query object before it gets submitted to the database.
 * 





 */
public class QueryMapper {

	private static final List<String> DEFAULT_ID_NAMES = Arrays.asList("id", "_id");
	private static final BSONObject META_TEXT_SCORE = new BasicBSONObject("$meta", "textScore");

	private enum MetaMapping {
		FORCE, WHEN_PRESENT, IGNORE;
	}

	private final ConversionService conversionService;
	private final SequoiadbConverter converter;
	private final MappingContext<? extends SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> mappingContext;

	/**
	 * Creates a new {@link QueryMapper} with the given {@link SequoiadbConverter}.
	 * 
	 * @param converter must not be {@literal null}.
	 */
	public QueryMapper(SequoiadbConverter converter) {

		Assert.notNull(converter);

		this.conversionService = converter.getConversionService();
		this.converter = converter;
		this.mappingContext = converter.getMappingContext();
	}

	/**
	 * Replaces the property keys used in the given {@link BSONObject} with the appropriate keys by using the
	 * {@link PersistentEntity} metadata.
	 * 
	 * @param query must not be {@literal null}.
	 * @param entity can be {@literal null}.
	 * @return
	 */
	@SuppressWarnings("deprecation")
	public BSONObject getMappedObject(BSONObject query, SequoiadbPersistentEntity<?> entity) {

		if (isNestedKeyword(query)) {
			return getMappedKeyword(new Keyword(query), entity);
		}

		BSONObject result = new BasicBSONObject();

		for (String key : query.keySet()) {

			if (Query.isRestrictedTypeKey(key)) {

				@SuppressWarnings("unchecked")
				Set<Class<?>> restrictedTypes = (Set<Class<?>>) query.get(key);
				this.converter.getTypeMapper().writeTypeRestrictions(result, restrictedTypes);

				continue;
			}

			if (isKeyword(key)) {
				result.putAll(getMappedKeyword(new Keyword(query, key), entity));
				continue;
			}

			Field field = createPropertyField(entity, key, mappingContext);
			Entry<String, Object> entry = getMappedObjectForField(field, query.get(key));

			result.put(entry.getKey(), entry.getValue());
		}

		return result;
	}

	/**
	 * Maps fields used for sorting to the {@link SequoiadbPersistentEntity}s properties. <br />
	 * Also converts properties to their {@code $meta} representation if present.
	 * 
	 * @param sortObject
	 * @param entity
	 * @return
	 * @since 1.6
	 */
	public BSONObject getMappedSort(BSONObject sortObject, SequoiadbPersistentEntity<?> entity) {

		if (sortObject == null) {
			return null;
		}

		BSONObject mappedSort = getMappedObject(sortObject, entity);
		mapMetaAttributes(mappedSort, entity, MetaMapping.WHEN_PRESENT);
		return mappedSort;
	}

	/**
	 * Maps fields to retrieve to the {@link SequoiadbPersistentEntity}s properties. <br />
	 * Also onverts and potentially adds missing property {@code $meta} representation.
	 * 
	 * @param fieldsObject
	 * @param entity
	 * @return
	 * @since 1.6
	 */
	public BSONObject getMappedFields(BSONObject fieldsObject, SequoiadbPersistentEntity<?> entity) {

		BSONObject mappedFields = fieldsObject != null ? getMappedObject(fieldsObject, entity) : new BasicBSONObject();
		mapMetaAttributes(mappedFields, entity, MetaMapping.FORCE);
		return mappedFields.keySet().isEmpty() ? null : mappedFields;
	}

	private void mapMetaAttributes(BSONObject source, SequoiadbPersistentEntity<?> entity, MetaMapping metaMapping) {

		if (entity == null || source == null) {
			return;
		}

		if (entity.hasTextScoreProperty() && !MetaMapping.IGNORE.equals(metaMapping)) {
			SequoiadbPersistentProperty textScoreProperty = entity.getTextScoreProperty();
			if (MetaMapping.FORCE.equals(metaMapping)
					|| (MetaMapping.WHEN_PRESENT.equals(metaMapping) && source.containsField(textScoreProperty.getFieldName()))) {
				source.putAll(getMappedTextScoreField(textScoreProperty));
			}
		}
	}

	private BSONObject getMappedTextScoreField(SequoiadbPersistentProperty property) {
		return new BasicBSONObject(property.getFieldName(), META_TEXT_SCORE);
	}

	/**
	 * Extracts the mapped object value for given field out of rawValue taking nested {@link Keyword}s into account
	 * 
	 * @param field
	 * @param rawValue
	 * @return
	 */
	protected Entry<String, Object> getMappedObjectForField(Field field, Object rawValue) {

		String key = field.getMappedKey();
		Object value;

		if (isNestedKeyword(rawValue) && !field.isIdField()) {
			Keyword keyword = new Keyword((BSONObject) rawValue);
			value = getMappedKeyword(field, keyword);
		} else {
			value = getMappedValue(field, rawValue);
		}

		return createMapEntry(key, value);
	}

	/**
	 * @param entity
	 * @param key
	 * @param mappingContext
	 * @return
	 */
	protected Field createPropertyField(SequoiadbPersistentEntity<?> entity, String key,
                                        MappingContext<? extends SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> mappingContext) {
		return entity == null ? new Field(key) : new MetadataBackedField(key, entity, mappingContext);
	}

	/**
	 * Returns the given {@link BSONObject} representing a keyword by mapping the keyword's value.
	 * 
	 * @param keyword the {@link BSONObject} representing a keyword (e.g. {@code $ne : … } )
	 * @param entity
	 * @return
	 */
	protected BSONObject getMappedKeyword(Keyword keyword, SequoiadbPersistentEntity<?> entity) {

		if (keyword.isOrOrNor() || keyword.hasIterableValue()) {

			Iterable<?> conditions = keyword.getValue();
			BasicBSONList newConditions = new BasicBSONList();

			for (Object condition : conditions) {
				newConditions.add(isDBObject(condition) ? getMappedObject((BSONObject) condition, entity)
						: convertSimpleOrDBObject(condition, entity));
			}

			return new BasicBSONObject(keyword.getKey(), newConditions);
		}

		return new BasicBSONObject(keyword.getKey(), convertSimpleOrDBObject(keyword.getValue(), entity));
	}

	/**
	 * Returns the mapped keyword considered defining a criteria for the given property.
	 * 
	 * @param property
	 * @param keyword
	 * @return
	 */
	protected BSONObject getMappedKeyword(Field property, Keyword keyword) {

		boolean needsAssociationConversion = property.isAssociation() && !keyword.isExists();
		Object value = keyword.getValue();

		Object convertedValue = needsAssociationConversion ? convertAssociation(value, property) : getMappedValue(
				property.with(keyword.getKey()), value);

		return new BasicBSONObject(keyword.key, convertedValue);
	}

	/**
	 * Returns the mapped value for the given source object assuming it's a value for the given
	 * {@link SequoiadbPersistentProperty}.
	 * 
	 * @param value the source object to be mapped
	 * @param property the property the value is a value for
	 * @param newKey the key the value will be bound to eventually
	 * @return
	 */
	protected Object getMappedValue(Field documentField, Object value) {

		if (documentField.isIdField()) {

			if (isDBObject(value)) {
				BSONObject valueDbo = (BSONObject) value;
				BSONObject resultDbo = new BasicBSONObject(valueDbo.toMap());

				if (valueDbo.containsField("$in") || valueDbo.containsField("$nin")) {
					String inKey = valueDbo.containsField("$in") ? "$in" : "$nin";
					List<Object> ids = new ArrayList<Object>();
					for (Object id : (Iterable<?>) valueDbo.get(inKey)) {
						ids.add(convertId(id));
					}
					resultDbo.put(inKey, ids.toArray(new Object[ids.size()]));
				} else if (valueDbo.containsField("$ne")) {
					resultDbo.put("$ne", convertId(valueDbo.get("$ne")));
				} else {
					return getMappedObject(resultDbo, null);
				}

				return resultDbo;

			} else {
				return convertId(value);
			}
		}

		if (isNestedKeyword(value)) {
			return getMappedKeyword(new Keyword((BSONObject) value), null);
		}

		if (isAssociationConversionNecessary(documentField, value)) {
			return convertAssociation(value, documentField);
		}

		return convertSimpleOrDBObject(value, documentField.getPropertyEntity());
	}

	/**
	 * Returns whether the given {@link Field} represents an association reference that together with the given value
	 * requires conversion to a {@link org.springframework.data.sequoiadb.core.mapping.DBRef} object. We check whether the
	 * type of the given value is compatible with the type of the given document field in order to deal with potential
	 * query field exclusions, since SequoiaDB uses the {@code int} {@literal 0} as an indicator for an excluded field.
	 * 
	 * @param documentField must not be {@literal null}.
	 * @param value
	 * @return
	 */
	protected boolean isAssociationConversionNecessary(Field documentField, Object value) {

		Assert.notNull(documentField, "Document field must not be null!");

		if (value == null) {
			return false;
		}

		if (!documentField.isAssociation()) {
			return false;
		}

		Class<? extends Object> type = value.getClass();
		SequoiadbPersistentProperty property = documentField.getProperty();

		if (property.getActualType().isAssignableFrom(type)) {
			return true;
		}

		SequoiadbPersistentEntity<?> entity = documentField.getPropertyEntity();
		return entity.hasIdProperty()
				&& (type.equals(DBRef.class) || entity.getIdProperty().getActualType().isAssignableFrom(type));
	}

	/**
	 * Retriggers mapping if the given source is a {@link BSONObject} or simply invokes the
	 * 
	 * @param source
	 * @param entity
	 * @return
	 */
	protected Object convertSimpleOrDBObject(Object source, SequoiadbPersistentEntity<?> entity) {

		if (source instanceof BasicBSONList) {
			return delegateConvertToSequoiadbType(source, entity);
		}

		if (isDBObject(source)) {
			return getMappedObject((BSONObject) source, entity);
		}

		return delegateConvertToSequoiadbType(source, entity);
	}

	/**
	 * Converts the given source Object to a sdb type with the type information of the original source type omitted.
	 * Subclasses may overwrite this method to retain the type information of the source type on the resulting sdb type.
	 * 
	 * @param source
	 * @param entity
	 * @return the converted sdb type or null if source is null
	 */
	protected Object delegateConvertToSequoiadbType(Object source, SequoiadbPersistentEntity<?> entity) {
		return converter.convertToSequoiadbType(source, entity == null ? null : entity.getTypeInformation());
	}

	protected Object convertAssociation(Object source, Field field) {
		return convertAssociation(source, field.getProperty());
	}

	/**
	 * Converts the given source assuming it's actually an association to another object.
	 * 
	 * @param source
	 * @param property
	 * @return
	 */
	protected Object convertAssociation(Object source, SequoiadbPersistentProperty property) {

		if (property == null || source == null || source instanceof BSONObject) {
			return source;
		}

		if (source instanceof DBRef) {

			DBRef ref = (DBRef) source;
			return new DBRef(ref.getDB(), ref.getRef(), convertId(ref.getId()));
		}

		if (source instanceof Iterable) {
			BasicBSONList result = new BasicBSONList();
			for (Object element : (Iterable<?>) source) {
				result.add(createDbRefFor(element, property));
			}
			return result;
		}

		if (property.isMap()) {
			BasicBSONObject result = new BasicBSONObject();
			BSONObject dbObject = (BSONObject) source;
			for (String key : dbObject.keySet()) {
				result.put(key, createDbRefFor(dbObject.get(key), property));
			}
			return result;
		}

		return createDbRefFor(source, property);
	}

	/**
	 * Checks whether the given value is a {@link BSONObject}.
	 * 
	 * @param value can be {@literal null}.
	 * @return
	 */
	protected final boolean isDBObject(Object value) {
		return value instanceof BSONObject;
	}

	/**
	 * Creates a new {@link Entry} for the given {@link Field} with the given value.
	 * 
	 * @param field must not be {@literal null}.
	 * @param value can be {@literal null}.
	 * @return
	 */
	protected final Entry<String, Object> createMapEntry(Field field, Object value) {
		return createMapEntry(field.getMappedKey(), value);
	}

	/**
	 * Creates a new {@link Entry} with the given key and value.
	 * 
	 * @param key must not be {@literal null} or empty.
	 * @param value can be {@literal null}
	 * @return
	 */
	private Entry<String, Object> createMapEntry(String key, Object value) {

		Assert.hasText(key, "Key must not be null or empty!");
		return Collections.singletonMap(key, value).entrySet().iterator().next();
	}

	private DBRef createDbRefFor(Object source, SequoiadbPersistentProperty property) {

		if (source instanceof DBRef) {
			return (DBRef) source;
		}

		return converter.toDBRef(source, property);
	}

	/**
	 * Converts the given raw id value into either {@link ObjectId} or {@link String}.
	 * 
	 * @param id
	 * @return
	 */
	public Object convertId(Object id) {

		try {
			return conversionService.convert(id, ObjectId.class);
		} catch (ConversionException e) {
		}

		return delegateConvertToSequoiadbType(id, null);
	}

	/**
	 * Returns whether the given {@link Object} is a keyword, i.e. if it's a {@link BSONObject} with a keyword key.
	 * 
	 * @param candidate
	 * @return
	 */
	protected boolean isNestedKeyword(Object candidate) {

		if (!(candidate instanceof BasicBSONObject)) {
			return false;
		}

		BasicBSONObject dbObject = (BasicBSONObject) candidate;
		Set<String> keys = dbObject.keySet();

		if (keys.size() != 1) {
			return false;
		}

		return isKeyword(keys.iterator().next().toString());
	}

	/**
	 * Returns whether the given {@link String} is a SequoiaDB keyword. The default implementation will check against the
	 * set of registered keywords returned by {@link #getKeywords()}.
	 * 
	 * @param candidate
	 * @return
	 */
	protected boolean isKeyword(String candidate) {
		return candidate.startsWith("$");
	}

	/**
	 * Value object to capture a query keyword representation.
	 * 

	 */
	static class Keyword {

		private static final String N_OR_PATTERN = "\\$.*or";

		private final String key;
		private final Object value;

		public Keyword(BSONObject source, String key) {
			this.key = key;
			this.value = source.get(key);
		}

		public Keyword(BSONObject dbObject) {

			Set<String> keys = dbObject.keySet();
			Assert.isTrue(keys.size() == 1, "Can only use a single value BSONObject!");

			this.key = keys.iterator().next();
			this.value = dbObject.get(key);
		}

		/**
		 * Returns whether the current keyword is the {@code $exists} keyword.
		 * 
		 * @return
		 */
		public boolean isExists() {
			return "$exists".equalsIgnoreCase(key);
		}

		public boolean isOrOrNor() {
			return key.matches(N_OR_PATTERN);
		}

		public boolean hasIterableValue() {
			return value instanceof Iterable;
		}

		public String getKey() {
			return key;
		}

		@SuppressWarnings("unchecked")
		public <T> T getValue() {
			return (T) value;
		}
	}

	/**
	 * Value object to represent a field and its meta-information.
	 * 

	 */
	protected static class Field {

		private static final String ID_KEY = "_id";

		protected final String name;

		/**
		 * Creates a new {@link DocumentField} without meta-information but the given name.
		 * 
		 * @param name must not be {@literal null} or empty.
		 */
		public Field(String name) {

			Assert.hasText(name, "Name must not be null!");
			this.name = name;
		}

		/**
		 * Returns a new {@link DocumentField} with the given name.
		 * 
		 * @param name must not be {@literal null} or empty.
		 * @return
		 */
		public Field with(String name) {
			return new Field(name);
		}

		/**
		 * Returns whether the current field is the id field.
		 * 
		 * @return
		 */
		public boolean isIdField() {
			return ID_KEY.equals(name);
		}

		/**
		 * Returns the underlying {@link SequoiadbPersistentProperty} backing the field. For path traversals this will be the
		 * property that represents the value to handle. This means it'll be the leaf property for plain paths or the
		 * association property in case we refer to an association somewhere in the path.
		 * 
		 * @return
		 */
		public SequoiadbPersistentProperty getProperty() {
			return null;
		}

		/**
		 * Returns the {@link SequoiadbPersistentEntity} that field is conatined in.
		 * 
		 * @return
		 */
		public SequoiadbPersistentEntity<?> getPropertyEntity() {
			return null;
		}

		/**
		 * Returns whether the field represents an association.
		 * 
		 * @return
		 */
		public boolean isAssociation() {
			return false;
		}

		/**
		 * Returns the key to be used in the mapped document eventually.
		 * 
		 * @return
		 */
		public String getMappedKey() {
			return isIdField() ? ID_KEY : name;
		}

		/**
		 * Returns whether the field references an association in case it refers to a nested field.
		 * 
		 * @return
		 */
		public boolean containsAssociation() {
			return false;
		}

		public Association<SequoiadbPersistentProperty> getAssociation() {
			return null;
		}
	}

	/**
	 * Extension of {@link DocumentField} to be backed with mapping metadata.
	 * 


	 */
	protected static class MetadataBackedField extends Field {

		private static final String INVALID_ASSOCIATION_REFERENCE = "Invalid path reference %s! Associations can only be pointed to directly or via their id property!";

		private final SequoiadbPersistentEntity<?> entity;
		private final MappingContext<? extends SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> mappingContext;
		private final SequoiadbPersistentProperty property;
		private final PersistentPropertyPath<SequoiadbPersistentProperty> path;
		private final Association<SequoiadbPersistentProperty> association;

		/**
		 * Creates a new {@link MetadataBackedField} with the given name, {@link SequoiadbPersistentEntity} and
		 * {@link MappingContext}.
		 * 
		 * @param name must not be {@literal null} or empty.
		 * @param entity must not be {@literal null}.
		 * @param context must not be {@literal null}.
		 */
		public MetadataBackedField(String name, SequoiadbPersistentEntity<?> entity,
				MappingContext<? extends SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> context) {
			this(name, entity, context, null);
		}

		/**
		 * Creates a new {@link MetadataBackedField} with the given name, {@link SequoiadbPersistentEntity} and
		 * {@link MappingContext} with the given {@link SequoiadbPersistentProperty}.
		 * 
		 * @param name must not be {@literal null} or empty.
		 * @param entity must not be {@literal null}.
		 * @param context must not be {@literal null}.
		 * @param property may be {@literal null}.
		 */
		public MetadataBackedField(String name, SequoiadbPersistentEntity<?> entity,
				MappingContext<? extends SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> context,
				SequoiadbPersistentProperty property) {

			super(name);

			Assert.notNull(entity, "SequoiadbPersistentEntity must not be null!");

			this.entity = entity;
			this.mappingContext = context;

			this.path = getPath(name);
			this.property = path == null ? property : path.getLeafProperty();
			this.association = findAssociation();
		}

		/*
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.core.convert.QueryMapper.Field#with(java.lang.String)
		 */
		@Override
		public MetadataBackedField with(String name) {
			return new MetadataBackedField(name, entity, mappingContext, property);
		}

		/*
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.core.convert.QueryMapper.Field#isIdKey()
		 */
		@Override
		public boolean isIdField() {

			SequoiadbPersistentProperty idProperty = entity.getIdProperty();

			if (idProperty != null) {
				return idProperty.getName().equals(name) || idProperty.getFieldName().equals(name);
			}

			return DEFAULT_ID_NAMES.contains(name);
		}

		/* 
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.core.convert.QueryMapper.Field#getProperty()
		 */
		@Override
		public SequoiadbPersistentProperty getProperty() {
			return association == null ? property : association.getInverse();
		}

		/* 
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.core.convert.QueryMapper.Field#getEntity()
		 */
		@Override
		public SequoiadbPersistentEntity<?> getPropertyEntity() {
			SequoiadbPersistentProperty property = getProperty();
			return property == null ? null : mappingContext.getPersistentEntity(property);
		}

		/* 
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.core.convert.QueryMapper.Field#isAssociation()
		 */
		@Override
		public boolean isAssociation() {
			return association != null;
		}

		/* 
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.core.convert.QueryMapper.Field#getAssociation()
		 */
		@Override
		public Association<SequoiadbPersistentProperty> getAssociation() {
			return association;
		}

		/**
		 * Finds the association property in the {@link PersistentPropertyPath}.
		 * 
		 * @return
		 */
		private final Association<SequoiadbPersistentProperty> findAssociation() {

			if (this.path != null) {
				for (SequoiadbPersistentProperty p : this.path) {
					if (p.isAssociation()) {
						return p.getAssociation();
					}
				}
			}

			return null;
		}

		/*
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.core.convert.QueryMapper.Field#getTargetKey()
		 */
		@Override
		public String getMappedKey() {
			return path == null ? name : path.toDotPath(isAssociation() ? getAssociationConverter() : getPropertyConverter());
		}

		protected PersistentPropertyPath<SequoiadbPersistentProperty> getPath() {
			return path;
		}

		/**
		 * Returns the {@link PersistentPropertyPath} for the given <code>pathExpression</code>.
		 * 
		 * @param pathExpression
		 * @return
		 */
		private PersistentPropertyPath<SequoiadbPersistentProperty> getPath(String pathExpression) {

			try {

				PropertyPath path = PropertyPath.from(pathExpression, entity.getTypeInformation());
				PersistentPropertyPath<SequoiadbPersistentProperty> propertyPath = mappingContext.getPersistentPropertyPath(path);

				Iterator<SequoiadbPersistentProperty> iterator = propertyPath.iterator();
				boolean associationDetected = false;

				while (iterator.hasNext()) {

					SequoiadbPersistentProperty property = iterator.next();

					if (property.isAssociation()) {
						associationDetected = true;
						continue;
					}

					if (associationDetected && !property.isIdProperty()) {
						throw new MappingException(String.format(INVALID_ASSOCIATION_REFERENCE, pathExpression));
					}
				}

				return propertyPath;
			} catch (PropertyReferenceException e) {
				return null;
			}
		}

		/**
		 * Return the {@link Converter} to be used to created the mapped key. Default implementation will use
		 * {@link PropertyToFieldNameConverter}.
		 * 
		 * @return
		 */
		protected Converter<SequoiadbPersistentProperty, String> getPropertyConverter() {
			return PropertyToFieldNameConverter.INSTANCE;
		}

		/**
		 * Return the {@link Converter} to use for creating the mapped key of an association. Default implementation is
		 * {@link AssociationConverter}.
		 * 
		 * @return
		 * @since 1.7
		 */
		protected Converter<SequoiadbPersistentProperty, String> getAssociationConverter() {
			return new AssociationConverter(getAssociation());
		}
	}

	/**
	 * Converter to skip all properties after an association property was rendered.
	 * 

	 */
	protected static class AssociationConverter implements Converter<SequoiadbPersistentProperty, String> {

		private final SequoiadbPersistentProperty property;
		private boolean associationFound;

		/**
		 * Creates a new {@link AssociationConverter} for the given {@link Association}.
		 * 
		 * @param association must not be {@literal null}.
		 */
		public AssociationConverter(Association<SequoiadbPersistentProperty> association) {

			Assert.notNull(association, "Association must not be null!");
			this.property = association.getInverse();
		}

		/* 
		 * (non-Javadoc)
		 * @see org.springframework.core.convert.converter.Converter#convert(java.lang.Object)
		 */
		@Override
		public String convert(SequoiadbPersistentProperty source) {

			if (associationFound) {
				return null;
			}

			if (property.equals(source)) {
				associationFound = true;
			}

			return source.getFieldName();
		}
	}
}
