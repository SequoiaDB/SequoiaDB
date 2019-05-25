/*
 * Copyright 2013-2014 the original author or authors.
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

import java.util.Arrays;
import java.util.Iterator;
import java.util.Map.Entry;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.springframework.core.convert.converter.Converter;
import org.springframework.data.mapping.Association;
import org.springframework.data.mapping.context.MappingContext;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty.PropertyToFieldNameConverter;
import org.springframework.data.sequoiadb.core.query.Query;
import org.springframework.data.sequoiadb.core.query.Update.Modifier;
import org.springframework.data.sequoiadb.core.query.Update.Modifiers;
import org.springframework.data.util.ClassTypeInformation;
import org.springframework.util.Assert;




/**
 * A subclass of {@link QueryMapper} that retains type information on the sdb types.
 * 



 */
public class UpdateMapper extends QueryMapper {

	private final SequoiadbConverter converter;

	/**
	 * Creates a new {@link UpdateMapper} using the given {@link SequoiadbConverter}.
	 * 
	 * @param converter must not be {@literal null}.
	 */
	public UpdateMapper(SequoiadbConverter converter) {

		super(converter);
		this.converter = converter;
	}

	/**
	 * Converts the given source object to a sdb type retaining the original type information of the source type on the
	 * sdb type.
	 * 
	 * @see org.springframework.data.sequoiadb.core.convert.QueryMapper#delegateConvertToSequoiadbType(java.lang.Object,
	 *      SequoiadbPersistentEntity)
	 */
	@Override
	protected Object delegateConvertToSequoiadbType(Object source, SequoiadbPersistentEntity<?> entity) {
		return entity == null ? super.delegateConvertToSequoiadbType(source, null) : converter.convertToSequoiadbType(source,
				entity.getTypeInformation());
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.convert.QueryMapper#getMappedObjectForField(org.springframework.data.sequoiadb.core.convert.QueryMapper.Field, java.lang.Object)
	 */
	@Override
	protected Entry<String, Object> getMappedObjectForField(Field field, Object rawValue) {

		if (isDBObject(rawValue)) {
			return createMapEntry(field, convertSimpleOrDBObject(rawValue, field.getPropertyEntity()));
		}

		if (isQuery(rawValue)) {
			return createMapEntry(field,
					super.getMappedObject(((Query) rawValue).getQueryObject(), field.getPropertyEntity()));
		}

		if (isUpdateModifier(rawValue)) {
			return getMappedUpdateModifier(field, rawValue);
		}

		return super.getMappedObjectForField(field, getMappedValue(field, rawValue));
	}

	private Entry<String, Object> getMappedUpdateModifier(Field field, Object rawValue) {
		Object value = null;

		if (rawValue instanceof Modifier) {

			value = getMappedValue((Modifier) rawValue);

		} else if (rawValue instanceof Modifiers) {

			BSONObject modificationOperations = new BasicBSONObject();

			for (Modifier modifier : ((Modifiers) rawValue).getModifiers()) {
				modificationOperations.putAll(getMappedValue(modifier).toMap());
			}

			value = modificationOperations;
		} else {
			throw new IllegalArgumentException(String.format("Unable to map value of type '%s'!", rawValue.getClass()));
		}

		return createMapEntry(field, value);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.convert.QueryMapper#isAssociationConversionNecessary(org.springframework.data.sequoiadb.core.convert.QueryMapper.Field, java.lang.Object)
	 */
	@Override
	protected boolean isAssociationConversionNecessary(Field documentField, Object value) {
		return super.isAssociationConversionNecessary(documentField, value) || documentField.containsAssociation();
	}

	private boolean isUpdateModifier(Object value) {
		return value instanceof Modifier || value instanceof Modifiers;
	}

	private boolean isQuery(Object value) {
		return value instanceof Query;
	}

	private BSONObject getMappedValue(Modifier modifier) {

		Object value = converter.convertToSequoiadbType(modifier.getValue(), ClassTypeInformation.OBJECT);
		return new BasicBSONObject(modifier.getKey(), value);
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.convert.QueryMapper#createPropertyField(org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity, java.lang.String, org.springframework.data.mapping.context.MappingContext)
	 */
	@Override
	protected Field createPropertyField(SequoiadbPersistentEntity<?> entity, String key,
                                        MappingContext<? extends SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> mappingContext) {

		return entity == null ? super.createPropertyField(entity, key, mappingContext) : //
				new MetadataBackedUpdateField(entity, key, mappingContext);
	}

	/**
	 * {@link MetadataBackedField} that handles {@literal $} paths inside a field key. We clean up an update key
	 * containing a {@literal $} before handing it to the super class to make sure property lookups and transformations
	 * continue to work as expected. We provide a custom property converter to re-applied the cleaned up {@literal $}s
	 * when constructing the mapped key.
	 * 


	 */
	private static class MetadataBackedUpdateField extends MetadataBackedField {

		private final String key;

		/**
		 * Creates a new {@link MetadataBackedField} with the given {@link SequoiadbPersistentEntity}, key and
		 * {@link MappingContext}. We clean up the key before handing it up to the super class to make sure it continues to
		 * work as expected.
		 * 
		 * @param entity must not be {@literal null}.
		 * @param key must not be {@literal null} or empty.
		 * @param mappingContext must not be {@literal null}.
		 */
		public MetadataBackedUpdateField(SequoiadbPersistentEntity<?> entity, String key,
                                         MappingContext<? extends SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> mappingContext) {

			super(key.replaceAll("\\.\\$", ""), entity, mappingContext);
			this.key = key;
		}

		/*
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.core.convert.QueryMapper.MetadataBackedField#getMappedKey()
		 */
		@Override
		public String getMappedKey() {
			return this.getPath() == null ? key : super.getMappedKey();
		}

		/* 
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.core.convert.QueryMapper.MetadataBackedField#getPropertyConverter()
		 */
		@Override
		protected Converter<SequoiadbPersistentProperty, String> getPropertyConverter() {
			return new UpdatePropertyConverter(key);
		}

		/*
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.core.convert.QueryMapper.MetadataBackedField#getAssociationConverter()
		 */
		@Override
		protected Converter<SequoiadbPersistentProperty, String> getAssociationConverter() {
			return new UpdateAssociationConverter(getAssociation(), key);
		}

		/**
		 * Special mapper handling positional parameter {@literal $} within property names.
		 * 

		 * @since 1.7
		 */
		private static class UpdateKeyMapper {

			private final Iterator<String> iterator;

			protected UpdateKeyMapper(String rawKey) {

				Assert.hasText(rawKey, "Key must not be null or empty!");

				this.iterator = Arrays.asList(rawKey.split("\\.")).iterator();
				this.iterator.next();
			}

			/**
			 * Maps the property name while retaining potential positional operator {@literal $}.
			 * 
			 * @param property
			 * @return
			 */
			protected String mapPropertyName(SequoiadbPersistentProperty property) {

				String mappedName = PropertyToFieldNameConverter.INSTANCE.convert(property);
				return iterator.hasNext() && iterator.next().equals("$") ? String.format("%s.0", mappedName) : mappedName;
			}

		}

		/**
		 * Special {@link Converter} for {@link SequoiadbPersistentProperty} instances that will concatenate the {@literal $}
		 * contained in the source update key.
		 * 


		 */
		private static class UpdatePropertyConverter implements Converter<SequoiadbPersistentProperty, String> {

			private final UpdateKeyMapper mapper;

			/**
			 * Creates a new {@link UpdatePropertyConverter} with the given update key.
			 * 
			 * @param updateKey must not be {@literal null} or empty.
			 */
			public UpdatePropertyConverter(String updateKey) {

				Assert.hasText(updateKey, "Update key must not be null or empty!");

				this.mapper = new UpdateKeyMapper(updateKey);
			}

			/* 
			 * (non-Javadoc)
			 * @see org.springframework.core.convert.converter.Converter#convert(java.lang.Object)
			 */
			@Override
			public String convert(SequoiadbPersistentProperty property) {
				return mapper.mapPropertyName(property);
			}
		}

		/**
		 * {@link Converter} retaining positional parameter {@literal $} for {@link Association}s.
		 * 

		 */
		protected static class UpdateAssociationConverter extends AssociationConverter {

			private final UpdateKeyMapper mapper;

			/**
			 * Creates a new {@link AssociationConverter} for the given {@link Association}.
			 * 
			 * @param association must not be {@literal null}.
			 */
			public UpdateAssociationConverter(Association<SequoiadbPersistentProperty> association, String key) {

				super(association);
				this.mapper = new UpdateKeyMapper(key);
			}

			/* 
			 * (non-Javadoc)
			 * @see org.springframework.core.convert.converter.Converter#convert(java.lang.Object)
			 */
			@Override
			public String convert(SequoiadbPersistentProperty source) {
				return super.convert(source) == null ? null : mapper.mapPropertyName(source);
			}
		}
	}
}
