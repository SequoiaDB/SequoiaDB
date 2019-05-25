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

import static org.springframework.data.mongodb.core.aggregation.Fields.*;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.springframework.data.domain.Sort;
import org.springframework.data.domain.Sort.Direction;
import org.springframework.data.mongodb.core.aggregation.ExposedFields.ExposedField;
import org.springframework.data.mongodb.core.aggregation.ExposedFields.FieldReference;
import org.springframework.data.mongodb.core.aggregation.Fields.AggregationField;
import org.springframework.data.mongodb.core.query.Criteria;
import org.springframework.data.mongodb.core.query.SerializationUtils;
import org.springframework.util.Assert;

import org.springframework.data.mongodb.assist.BasicDBObject;
import org.springframework.data.mongodb.assist.DBObject;

/**
 * An {@code Aggregation} is a representation of a list of aggregation steps to be performed by the MongoDB Aggregation
 * Framework.
 * 
 * @author Tobias Trelle
 * @author Thomas Darimont
 * @author Oliver Gierke
 * @since 1.3
 */
public class Aggregation {

	/**
	 * References the root document, i.e. the top-level document, currently being processed in the aggregation pipeline
	 * stage.
	 */
	public static final String ROOT = SystemVariable.ROOT.toString();

	/**
	 * References the start of the field path being processed in the aggregation pipeline stage. Unless documented
	 * otherwise, all stages start with CURRENT the same as ROOT.
	 */
	public static final String CURRENT = SystemVariable.CURRENT.toString();

	public static final AggregationOperationContext DEFAULT_CONTEXT = new NoOpAggregationOperationContext();
	public static final AggregationOptions DEFAULT_OPTIONS = newAggregationOptions().build();

	protected final List<AggregationOperation> operations;
	private final AggregationOptions options;

	/**
	 * Creates a new {@link Aggregation} from the given {@link AggregationOperation}s.
	 * 
	 * @param operations must not be {@literal null} or empty.
	 */
	public static Aggregation newAggregation(List<? extends AggregationOperation> operations) {
		return newAggregation(operations.toArray(new AggregationOperation[operations.size()]));
	}

	/**
	 * Creates a new {@link Aggregation} from the given {@link AggregationOperation}s.
	 * 
	 * @param operations must not be {@literal null} or empty.
	 */
	public static Aggregation newAggregation(AggregationOperation... operations) {
		return new Aggregation(operations);
	}

	/**
	 * Returns a copy of this {@link Aggregation} with the given {@link AggregationOptions} set. Note that options are
	 * supported in MongoDB version 2.6+.
	 * 
	 * @param options must not be {@literal null}.
	 * @return
	 * @since 1.6
	 */
	public Aggregation withOptions(AggregationOptions options) {

		Assert.notNull(options, "AggregationOptions must not be null.");
		return new Aggregation(this.operations, options);
	}

	/**
	 * Creates a new {@link TypedAggregation} for the given type and {@link AggregationOperation}s.
	 * 
	 * @param type must not be {@literal null}.
	 * @param operations must not be {@literal null} or empty.
	 */
	public static <T> TypedAggregation<T> newAggregation(Class<T> type, List<? extends AggregationOperation> operations) {
		return newAggregation(type, operations.toArray(new AggregationOperation[operations.size()]));
	}

	/**
	 * Creates a new {@link TypedAggregation} for the given type and {@link AggregationOperation}s.
	 * 
	 * @param type must not be {@literal null}.
	 * @param operations must not be {@literal null} or empty.
	 */
	public static <T> TypedAggregation<T> newAggregation(Class<T> type, AggregationOperation... operations) {
		return new TypedAggregation<T>(type, operations);
	}

	/**
	 * Creates a new {@link Aggregation} from the given {@link AggregationOperation}s.
	 * 
	 * @param aggregationOperations must not be {@literal null} or empty.
	 */
	protected Aggregation(AggregationOperation... aggregationOperations) {
		this(asAggregationList(aggregationOperations));
	}

	/**
	 * @param aggregationOperations must not be {@literal null} or empty.
	 * @return
	 */
	protected static List<AggregationOperation> asAggregationList(AggregationOperation... aggregationOperations) {

		Assert.notEmpty(aggregationOperations, "AggregationOperations must not be null or empty!");

		return Arrays.asList(aggregationOperations);
	}

	/**
	 * Creates a new {@link Aggregation} from the given {@link AggregationOperation}s.
	 * 
	 * @param aggregationOperations must not be {@literal null} or empty.
	 */
	protected Aggregation(List<AggregationOperation> aggregationOperations) {
		this(aggregationOperations, DEFAULT_OPTIONS);
	}

	/**
	 * Creates a new {@link Aggregation} from the given {@link AggregationOperation}s.
	 * 
	 * @param aggregationOperations must not be {@literal null} or empty.
	 * @param options must not be {@literal null} or empty.
	 */
	protected Aggregation(List<AggregationOperation> aggregationOperations, AggregationOptions options) {

		Assert.notNull(aggregationOperations, "AggregationOperations must not be null!");
		Assert.isTrue(aggregationOperations.size() > 0, "At least one AggregationOperation has to be provided");
		Assert.notNull(options, "AggregationOptions must not be null!");

		this.operations = aggregationOperations;
		this.options = options;
	}

	/**
	 * A pointer to the previous {@link AggregationOperation}.
	 * 
	 * @return
	 */
	public static String previousOperation() {
		return "_id";
	}

	/**
	 * Creates a new {@link ProjectionOperation} including the given fields.
	 * 
	 * @param fields must not be {@literal null}.
	 * @return
	 */
	public static ProjectionOperation project(String... fields) {
		return project(fields(fields));
	}

	/**
	 * Creates a new {@link ProjectionOperation} includeing the given {@link Fields}.
	 * 
	 * @param fields must not be {@literal null}.
	 * @return
	 */
	public static ProjectionOperation project(Fields fields) {
		return new ProjectionOperation(fields);
	}

	/**
	 * Factory method to create a new {@link UnwindOperation} for the field with the given name.
	 * 
	 * @param fieldName must not be {@literal null} or empty.
	 * @return
	 */
	public static UnwindOperation unwind(String field) {
		return new UnwindOperation(field(field));
	}

	/**
	 * Creates a new {@link GroupOperation} for the given fields.
	 * 
	 * @param fields must not be {@literal null}.
	 * @return
	 */
	public static GroupOperation group(String... fields) {
		return group(fields(fields));
	}

	/**
	 * Creates a new {@link GroupOperation} for the given {@link Fields}.
	 * 
	 * @param fields must not be {@literal null}.
	 * @return
	 */
	public static GroupOperation group(Fields fields) {
		return new GroupOperation(fields);
	}

	/**
	 * Factory method to create a new {@link SortOperation} for the given {@link Sort}.
	 * 
	 * @param sort must not be {@literal null}.
	 * @return
	 */
	public static SortOperation sort(Sort sort) {
		return new SortOperation(sort);
	}

	/**
	 * Factory method to create a new {@link SortOperation} for the given sort {@link Direction} and {@code fields}.
	 * 
	 * @param direction must not be {@literal null}.
	 * @param fields must not be {@literal null}.
	 * @return
	 */
	public static SortOperation sort(Direction direction, String... fields) {
		return new SortOperation(new Sort(direction, fields));
	}

	/**
	 * Creates a new {@link SkipOperation} skipping the given number of elements.
	 * 
	 * @param elementsToSkip must not be less than zero.
	 * @return
	 */
	public static SkipOperation skip(int elementsToSkip) {
		return new SkipOperation(elementsToSkip);
	}

	/**
	 * Creates a new {@link LimitOperation} limiting the result to the given number of elements.
	 * 
	 * @param maxElements must not be less than zero.
	 * @return
	 */
	public static LimitOperation limit(long maxElements) {
		return new LimitOperation(maxElements);
	}

	/**
	 * Creates a new {@link MatchOperation} using the given {@link Criteria}.
	 * 
	 * @param criteria must not be {@literal null}.
	 * @return
	 */
	public static MatchOperation match(Criteria criteria) {
		return new MatchOperation(criteria);
	}

	/**
	 * Creates a new {@link Fields} instance for the given field names.
	 * 
	 * @see Fields#fields(String...)
	 * @param fields must not be {@literal null}.
	 * @return
	 */
	public static Fields fields(String... fields) {
		return Fields.fields(fields);
	}

	/**
	 * Creates a new {@link Fields} instance from the given field name and target reference.
	 * 
	 * @param name must not be {@literal null} or empty.
	 * @param target must not be {@literal null} or empty.
	 * @return
	 */
	public static Fields bind(String name, String target) {
		return Fields.from(field(name, target));
	}

	/**
	 * Returns a new {@link AggregationOptions.Builder}.
	 * 
	 * @return
	 * @since 1.6
	 */
	public static AggregationOptions.Builder newAggregationOptions() {
		return new AggregationOptions.Builder();
	}

	/**
	 * Converts this {@link Aggregation} specification to a {@link DBObject}.
	 * 
	 * @param inputCollectionName the name of the input collection
	 * @return the {@code DBObject} representing this aggregation
	 */
	public DBObject toDbObject(String inputCollectionName, AggregationOperationContext rootContext) {

		AggregationOperationContext context = rootContext;
		List<DBObject> operationDocuments = new ArrayList<DBObject>(operations.size());

		for (AggregationOperation operation : operations) {

			operationDocuments.add(operation.toDBObject(context));

			if (operation instanceof FieldsExposingAggregationOperation) {
				FieldsExposingAggregationOperation exposedFieldsOperation = (FieldsExposingAggregationOperation) operation;
				context = new ExposedFieldsAggregationOperationContext(exposedFieldsOperation.getFields(), rootContext);
			}
		}

		DBObject command = new BasicDBObject("aggregate", inputCollectionName);
		command.put("pipeline", operationDocuments);

		command = options.applyAndReturnPotentiallyChangedCommand(command);

		return command;
	}

	/* 
	 * (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString() {
		return SerializationUtils
				.serializeToJsonSafely(toDbObject("__collection__", new NoOpAggregationOperationContext()));
	}

	/**
	 * Simple {@link AggregationOperationContext} that just returns {@link FieldReference}s as is.
	 * 
	 * @author Oliver Gierke
	 */
	private static class NoOpAggregationOperationContext implements AggregationOperationContext {

		/* 
		 * (non-Javadoc)
		 * @see org.springframework.data.mongodb.core.aggregation.AggregationOperationContext#getMappedObject(org.springframework.data.mongodb.assist.DBObject)
		 */
		@Override
		public DBObject getMappedObject(DBObject dbObject) {
			return dbObject;
		}

		/* 
		 * (non-Javadoc)
		 * @see org.springframework.data.mongodb.core.aggregation.AggregationOperationContext#getReference(org.springframework.data.mongodb.core.aggregation.ExposedFields.AvailableField)
		 */
		@Override
		public FieldReference getReference(Field field) {
			return new FieldReference(new ExposedField(field, true));
		}

		/* 
		 * (non-Javadoc)
		 * @see org.springframework.data.mongodb.core.aggregation.AggregationOperationContext#getReference(java.lang.String)
		 */
		@Override
		public FieldReference getReference(String name) {
			return new FieldReference(new ExposedField(new AggregationField(name), true));
		}
	}

	/**
	 * Describes the system variables available in MongoDB aggregation framework pipeline expressions.
	 * 
	 * @author Thomas Darimont
	 * @see http://docs.mongodb.org/manual/reference/aggregation-variables
	 */
	enum SystemVariable {

		ROOT, CURRENT;

		private static final String PREFIX = "$$";

		/**
		 * Return {@literal true} if the given {@code fieldRef} denotes a well-known system variable, {@literal false}
		 * otherwise.
		 * 
		 * @param fieldRef may be {@literal null}.
		 * @return
		 */
		public static boolean isReferingToSystemVariable(String fieldRef) {

			if (fieldRef == null || !fieldRef.startsWith(PREFIX) || fieldRef.length() <= 2) {
				return false;
			}

			int indexOfFirstDot = fieldRef.indexOf('.');
			String candidate = fieldRef.substring(2, indexOfFirstDot == -1 ? fieldRef.length() : indexOfFirstDot);

			for (SystemVariable value : values()) {
				if (value.name().equals(candidate)) {
					return true;
				}
			}

			return false;
		}

		/* 
		 * (non-Javadoc)
		 * @see java.lang.Enum#toString()
		 */
		@Override
		public String toString() {
			return PREFIX.concat(name());
		}
	}
}
