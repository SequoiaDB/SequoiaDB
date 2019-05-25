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
package org.springframework.data.mongodb.core.mapping;

import java.beans.PropertyDescriptor;
import java.lang.reflect.Field;
import java.math.BigInteger;
import java.util.HashSet;
import java.util.Set;

import org.bson.types.ObjectId;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.data.annotation.Id;
import org.springframework.data.mapping.Association;
import org.springframework.data.mapping.model.AnnotationBasedPersistentProperty;
import org.springframework.data.mapping.model.FieldNamingStrategy;
import org.springframework.data.mapping.model.MappingException;
import org.springframework.data.mapping.model.PropertyNameFieldNamingStrategy;
import org.springframework.data.mapping.model.SimpleTypeHolder;
import org.springframework.util.StringUtils;

import org.springframework.data.mongodb.assist.DBObject;

/**
 * MongoDB specific {@link org.springframework.data.mapping.MongoPersistentProperty} implementation.
 * 
 * @author Oliver Gierke
 * @author Patryk Wasik
 * @author Thomas Darimont
 * @author Christoph Strobl
 */
public class BasicMongoPersistentProperty extends AnnotationBasedPersistentProperty<MongoPersistentProperty> implements
		MongoPersistentProperty {

	private static final Logger LOG = LoggerFactory.getLogger(BasicMongoPersistentProperty.class);

	private static final String ID_FIELD_NAME = "_id";
	private static final String LANGUAGE_FIELD_NAME = "language";
	private static final Set<Class<?>> SUPPORTED_ID_TYPES = new HashSet<Class<?>>();
	private static final Set<String> SUPPORTED_ID_PROPERTY_NAMES = new HashSet<String>();

	static {

		SUPPORTED_ID_TYPES.add(ObjectId.class);
		SUPPORTED_ID_TYPES.add(String.class);
		SUPPORTED_ID_TYPES.add(BigInteger.class);

		SUPPORTED_ID_PROPERTY_NAMES.add("id");
		SUPPORTED_ID_PROPERTY_NAMES.add("_id");
	}

	private final FieldNamingStrategy fieldNamingStrategy;

	/**
	 * Creates a new {@link BasicMongoPersistentProperty}.
	 * 
	 * @param field
	 * @param propertyDescriptor
	 * @param owner
	 * @param simpleTypeHolder
	 * @param fieldNamingStrategy
	 */
	public BasicMongoPersistentProperty(Field field, PropertyDescriptor propertyDescriptor,
			MongoPersistentEntity<?> owner, SimpleTypeHolder simpleTypeHolder, FieldNamingStrategy fieldNamingStrategy) {

		super(field, propertyDescriptor, owner, simpleTypeHolder);
		this.fieldNamingStrategy = fieldNamingStrategy == null ? PropertyNameFieldNamingStrategy.INSTANCE
				: fieldNamingStrategy;

		if (isIdProperty() && getFieldName() != ID_FIELD_NAME) {
			LOG.warn("Customizing field name for id property not allowed! Custom name will not be considered!");
		}
	}

	/**
	 * Also considers fields as id that are of supported id type and name.
	 * 
	 * @see #SUPPORTED_ID_PROPERTY_NAMES
	 * @see #SUPPORTED_ID_TYPES
	 */
	@Override
	public boolean isIdProperty() {

		if (super.isIdProperty()) {
			return true;
		}

		return SUPPORTED_ID_PROPERTY_NAMES.contains(getName());
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.mapping.MongoPersistentProperty#isExplicitIdProperty()
	 */
	@Override
	public boolean isExplicitIdProperty() {
		return isAnnotationPresent(Id.class);
	}

	/**
	 * Returns the key to be used to store the value of the property inside a Mongo {@link DBObject}.
	 * 
	 * @return
	 */
	public String getFieldName() {

		if (isIdProperty()) {

			if (owner == null) {
				return ID_FIELD_NAME;
			}

			if (owner.getIdProperty() == null) {
				return ID_FIELD_NAME;
			}

			if (owner.isIdProperty(this)) {
				return ID_FIELD_NAME;
			}
		}

		org.springframework.data.mongodb.core.mapping.Field annotation = findAnnotation(org.springframework.data.mongodb.core.mapping.Field.class);

		if (annotation != null && StringUtils.hasText(annotation.value())) {
			return annotation.value();
		}

		String fieldName = fieldNamingStrategy.getFieldName(this);

		if (!StringUtils.hasText(fieldName)) {
			throw new MappingException(String.format("Invalid (null or empty) field name returned for property %s by %s!",
					this, fieldNamingStrategy.getClass()));
		}

		return fieldName;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.mapping.MongoPersistentProperty#getFieldOrder()
	 */
	public int getFieldOrder() {
		org.springframework.data.mongodb.core.mapping.Field annotation = findAnnotation(org.springframework.data.mongodb.core.mapping.Field.class);
		return annotation != null ? annotation.order() : Integer.MAX_VALUE;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mapping.model.AbstractPersistentProperty#createAssociation()
	 */
	@Override
	protected Association<MongoPersistentProperty> createAssociation() {
		return new Association<MongoPersistentProperty>(this, null);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.mapping.MongoPersistentProperty#isDbReference()
	 */
	public boolean isDbReference() {
		return isAnnotationPresent(DBRef.class);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.mapping.MongoPersistentProperty#getDBRef()
	 */
	public DBRef getDBRef() {
		return findAnnotation(DBRef.class);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.mapping.MongoPersistentProperty#isLanguageProperty()
	 */
	@Override
	public boolean isLanguageProperty() {
		return getFieldName().equals(LANGUAGE_FIELD_NAME) || isExplicitLanguageProperty();
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.mapping.MongoPersistentProperty#isExplicitLanguageProperty()
	 */
	@Override
	public boolean isExplicitLanguageProperty() {
		return isAnnotationPresent(Language.class);
	};

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.mapping.MongoPersistentProperty#isTextScoreProperty()
	 */
	@Override
	public boolean isTextScoreProperty() {
		return isAnnotationPresent(TextScore.class);
	}
}
