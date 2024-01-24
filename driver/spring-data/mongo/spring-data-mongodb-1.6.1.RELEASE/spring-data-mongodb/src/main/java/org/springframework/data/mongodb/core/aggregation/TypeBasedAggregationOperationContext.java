/*
 * Copyright 2013 the original author or authors.
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
package org.springframework.data.mongodb.core.aggregation;

import static org.springframework.data.mongodb.core.aggregation.Fields.*;

import org.springframework.data.mapping.PropertyPath;
import org.springframework.data.mapping.context.MappingContext;
import org.springframework.data.mapping.context.PersistentPropertyPath;
import org.springframework.data.mongodb.core.aggregation.ExposedFields.ExposedField;
import org.springframework.data.mongodb.core.aggregation.ExposedFields.FieldReference;
import org.springframework.data.mongodb.core.convert.QueryMapper;
import org.springframework.data.mongodb.core.mapping.MongoPersistentEntity;
import org.springframework.data.mongodb.core.mapping.MongoPersistentProperty;
import org.springframework.util.Assert;

import org.springframework.data.mongodb.assist.DBObject;

/**
 * {@link AggregationOperationContext} aware of a particular type and a {@link MappingContext} to potentially translate
 * property references into document field names.
 * 
 * @author Oliver Gierke
 * @since 1.3
 */
public class TypeBasedAggregationOperationContext implements AggregationOperationContext {

	private final Class<?> type;
	private final MappingContext<? extends MongoPersistentEntity<?>, MongoPersistentProperty> mappingContext;
	private final QueryMapper mapper;

	/**
	 * Creates a new {@link TypeBasedAggregationOperationContext} for the given type, {@link MappingContext} and
	 * {@link QueryMapper}.
	 * 
	 * @param type must not be {@literal null}.
	 * @param mappingContext must not be {@literal null}.
	 * @param mapper must not be {@literal null}.
	 */
	public TypeBasedAggregationOperationContext(Class<?> type,
			MappingContext<? extends MongoPersistentEntity<?>, MongoPersistentProperty> mappingContext, QueryMapper mapper) {

		Assert.notNull(type, "Type must not be null!");
		Assert.notNull(mappingContext, "MappingContext must not be null!");
		Assert.notNull(mapper, "QueryMapper must not be null!");

		this.type = type;
		this.mappingContext = mappingContext;
		this.mapper = mapper;
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.aggregation.AggregationOperationContext#getMappedObject(org.springframework.data.mongodb.assist.DBObject)
	 */
	@Override
	public DBObject getMappedObject(DBObject dbObject) {
		return mapper.getMappedObject(dbObject, mappingContext.getPersistentEntity(type));
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.aggregation.AggregationOperationContext#getReference(org.springframework.data.mongodb.core.aggregation.Field)
	 */
	@Override
	public FieldReference getReference(Field field) {

		PropertyPath.from(field.getTarget(), type);
		return getReferenceFor(field);
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.aggregation.AggregationOperationContext#getReference(java.lang.String)
	 */
	@Override
	public FieldReference getReference(String name) {
		return getReferenceFor(field(name));
	}

	private FieldReference getReferenceFor(Field field) {

		PersistentPropertyPath<MongoPersistentProperty> propertyPath = mappingContext.getPersistentPropertyPath(
				field.getTarget(), type);
		Field mappedField = field(propertyPath.getLeafProperty().getName(),
				propertyPath.toDotPath(MongoPersistentProperty.PropertyToFieldNameConverter.INSTANCE));

		return new FieldReference(new ExposedField(mappedField, true));
	}
}
