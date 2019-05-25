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
package org.springframework.data.mongodb.core.aggregation;

import org.springframework.data.mongodb.core.aggregation.ExposedFields.ExposedField;
import org.springframework.data.mongodb.core.aggregation.ExposedFields.FieldReference;
import org.springframework.util.Assert;

import org.springframework.data.mongodb.assist.DBObject;

/**
 * {@link AggregationOperationContext} that combines the available field references from a given
 * {@code AggregationOperationContext} and an {@link FieldsExposingAggregationOperation}.
 * 
 * @author Thomas Darimont
 * @author Oliver Gierke
 * @since 1.4
 */
class ExposedFieldsAggregationOperationContext implements AggregationOperationContext {

	private final ExposedFields exposedFields;
	private final AggregationOperationContext rootContext;

	/**
	 * Creates a new {@link ExposedFieldsAggregationOperationContext} from the given {@link ExposedFields}. Uses the given
	 * {@link AggregationOperationContext} to perform a mapping to mongo types if necessary.
	 * 
	 * @param exposedFields must not be {@literal null}.
	 * @param rootContext must not be {@literal null}.
	 */
	public ExposedFieldsAggregationOperationContext(ExposedFields exposedFields, AggregationOperationContext rootContext) {

		Assert.notNull(exposedFields, "ExposedFields must not be null!");
		Assert.notNull(rootContext, "RootContext must not be null!");

		this.exposedFields = exposedFields;
		this.rootContext = rootContext;
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.aggregation.AggregationOperationContext#getMappedObject(org.springframework.data.mongodb.assist.DBObject)
	 */
	@Override
	public DBObject getMappedObject(DBObject dbObject) {
		return rootContext.getMappedObject(dbObject);
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.aggregation.AggregationOperationContext#getReference(org.springframework.data.mongodb.core.aggregation.ExposedFields.AvailableField)
	 */
	@Override
	public FieldReference getReference(Field field) {
		return getReference(field, field.getTarget());
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.aggregation.AggregationOperationContext#getReference(java.lang.String)
	 */
	@Override
	public FieldReference getReference(String name) {
		return getReference(null, name);
	}

	/**
	 * Returns a {@link FieldReference} to the given {@link Field} with the given {@code name}.
	 * 
	 * @param field may be {@literal null}
	 * @param name must not be {@literal null}
	 * @return
	 */
	private FieldReference getReference(Field field, String name) {

		Assert.notNull(name, "Name must not be null!");

		ExposedField exposedField = exposedFields.getField(name);

		if (exposedField != null) {

			if (field != null) {
				return new FieldReference(new ExposedField(field, exposedField.isSynthetic()));
			}

			return new FieldReference(exposedField);
		}

		if (name.contains(".")) {

			ExposedField rootField = exposedFields.getField(name.split("\\.")[0]);

			if (rootField != null) {

				return new FieldReference(new ExposedField(name, true));
			}
		}

		throw new IllegalArgumentException(String.format("Invalid reference '%s'!", name));
	}
}
