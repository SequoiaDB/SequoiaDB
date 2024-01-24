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
package org.springframework.data.sequoiadb.core.mapping;

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



/**
 * SequoiaDB specific {@link org.springframework.data.mapping.SequoiadbPersistentProperty} implementation.
 * 




 */
public class BasicSequoiadbPersistentProperty extends AnnotationBasedPersistentProperty<SequoiadbPersistentProperty> implements
        SequoiadbPersistentProperty {

	private static final Logger LOG = LoggerFactory.getLogger(BasicSequoiadbPersistentProperty.class);

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
	 * Creates a new {@link BasicSequoiadbPersistentProperty}.
	 * 
	 * @param field
	 * @param propertyDescriptor
	 * @param owner
	 * @param simpleTypeHolder
	 * @param fieldNamingStrategy
	 */
	public BasicSequoiadbPersistentProperty(Field field, PropertyDescriptor propertyDescriptor,
                                            SequoiadbPersistentEntity<?> owner, SimpleTypeHolder simpleTypeHolder, FieldNamingStrategy fieldNamingStrategy) {

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

		// We need to support a wider range of ID types than just the ones that can be converted to an ObjectId
		return SUPPORTED_ID_PROPERTY_NAMES.contains(getName());
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty#isExplicitIdProperty()
	 */
	@Override
	public boolean isExplicitIdProperty() {
		return isAnnotationPresent(Id.class);
	}

	/**
	 * Returns the key to be used to store the value of the property inside a Sdb {@link BSONObject}.
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

		org.springframework.data.sequoiadb.core.mapping.Field annotation = findAnnotation(org.springframework.data.sequoiadb.core.mapping.Field.class);

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
	 * @see org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty#getFieldOrder()
	 */
	public int getFieldOrder() {
		org.springframework.data.sequoiadb.core.mapping.Field annotation = findAnnotation(org.springframework.data.sequoiadb.core.mapping.Field.class);
		return annotation != null ? annotation.order() : Integer.MAX_VALUE;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mapping.model.AbstractPersistentProperty#createAssociation()
	 */
	@Override
	protected Association<SequoiadbPersistentProperty> createAssociation() {
		return new Association<SequoiadbPersistentProperty>(this, null);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty#isDbReference()
	 */
	public boolean isDbReference() {
		return isAnnotationPresent(DBRef.class);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty#getDBRef()
	 */
	public DBRef getDBRef() {
		return findAnnotation(DBRef.class);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty#isLanguageProperty()
	 */
	@Override
	public boolean isLanguageProperty() {
		return getFieldName().equals(LANGUAGE_FIELD_NAME) || isExplicitLanguageProperty();
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty#isExplicitLanguageProperty()
	 */
	@Override
	public boolean isExplicitLanguageProperty() {
		return isAnnotationPresent(Language.class);
	};

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty#isTextScoreProperty()
	 */
	@Override
	public boolean isTextScoreProperty() {
		return isAnnotationPresent(TextScore.class);
	}
}
