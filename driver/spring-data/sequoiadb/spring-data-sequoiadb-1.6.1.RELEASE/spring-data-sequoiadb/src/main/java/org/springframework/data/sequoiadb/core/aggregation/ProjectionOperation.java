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
package org.springframework.data.sequoiadb.core.aggregation;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.springframework.data.sequoiadb.core.aggregation.ExposedFields.ExposedField;
import org.springframework.data.sequoiadb.core.aggregation.ProjectionOperation.ProjectionOperationBuilder.FieldProjection;
import org.springframework.util.Assert;




/**
 * Encapsulates the aggregation framework {@code $project}-operation. Projection of field to be used in an
 * {@link Aggregation}. A projection is similar to a {@link Field} inclusion/exclusion but more powerful. It can
 * generate new fields, change values of given field etc.
 * <p>
 * 
 * @see http://docs.sequoiadb.org/manual/reference/aggregation/project/



 * @since 1.3
 */
public class ProjectionOperation implements FieldsExposingAggregationOperation {

	private static final List<Projection> NONE = Collections.emptyList();
	private static final String EXCLUSION_ERROR = "Exclusion of field %s not allowed. Projections by the sequoiadb "
			+ "aggregation framework only support the exclusion of the %s field!";

	private final List<Projection> projections;

	/**
	 * Creates a new empty {@link ProjectionOperation}.
	 */
	public ProjectionOperation() {
		this(NONE, NONE);
	}

	/**
	 * Creates a new {@link ProjectionOperation} including the given {@link Fields}.
	 * 
	 * @param fields must not be {@literal null}.
	 */
	public ProjectionOperation(Fields fields) {
		this(NONE, ProjectionOperationBuilder.FieldProjection.from(fields));
	}

	/**
	 * Copy constructor to allow building up {@link ProjectionOperation} instances from already existing
	 * {@link Projection}s.
	 * 
	 * @param current must not be {@literal null}.
	 * @param projections must not be {@literal null}.
	 */
	private ProjectionOperation(List<? extends Projection> current, List<? extends Projection> projections) {

		Assert.notNull(current, "Current projections must not be null!");
		Assert.notNull(projections, "Projections must not be null!");

		this.projections = new ArrayList<ProjectionOperation.Projection>(current.size() + projections.size());
		this.projections.addAll(current);
		this.projections.addAll(projections);
	}

	/**
	 * Creates a new {@link ProjectionOperation} with the current {@link Projection}s and the given one.
	 * 
	 * @param projection must not be {@literal null}.
	 * @return
	 */
	private ProjectionOperation and(Projection projection) {
		return new ProjectionOperation(this.projections, Arrays.asList(projection));
	}

	/**
	 * Creates a new {@link ProjectionOperation} with the current {@link Projection}s replacing the last current one with
	 * the given one.
	 * 
	 * @param projection must not be {@literal null}.
	 * @return
	 */
	private ProjectionOperation andReplaceLastOneWith(Projection projection) {

		List<Projection> projections = this.projections.isEmpty() ? Collections.<Projection> emptyList() : this.projections
				.subList(0, this.projections.size() - 1);
		return new ProjectionOperation(projections, Arrays.asList(projection));
	}

	/**
	 * Creates a new {@link ProjectionOperationBuilder} to define a projection for the field with the given name.
	 * 
	 * @param name must not be {@literal null} or empty.
	 * @return
	 */
	public ProjectionOperationBuilder and(String name) {
		return new ProjectionOperationBuilder(name, this, null);
	}

	public ExpressionProjectionOperationBuilder andExpression(String expression, Object... params) {
		return new ExpressionProjectionOperationBuilder(expression, this, params);
	}

	/**
	 * Excludes the given fields from the projection.
	 * 
	 * @param fieldNames must not be {@literal null}.
	 * @return
	 */
	public ProjectionOperation andExclude(String... fieldNames) {

		for (String fieldName : fieldNames) {
			Assert.isTrue(Fields.UNDERSCORE_ID.equals(fieldName),
					String.format(EXCLUSION_ERROR, fieldName, Fields.UNDERSCORE_ID));
		}

		List<FieldProjection> excludeProjections = FieldProjection.from(Fields.fields(fieldNames), false);
		return new ProjectionOperation(this.projections, excludeProjections);
	}

	/**
	 * Includes the given fields into the projection.
	 * 
	 * @param fieldNames must not be {@literal null}.
	 * @return
	 */
	public ProjectionOperation andInclude(String... fieldNames) {

		List<FieldProjection> projections = FieldProjection.from(Fields.fields(fieldNames), true);
		return new ProjectionOperation(this.projections, projections);
	}

	/**
	 * Includes the given fields into the projection.
	 * 
	 * @param fields must not be {@literal null}.
	 * @return
	 */
	public ProjectionOperation andInclude(Fields fields) {
		return new ProjectionOperation(this.projections, FieldProjection.from(fields, true));
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.aggregation.FieldsExposingAggregationOperation#getFields()
	 */
	@Override
	public ExposedFields getFields() {

		ExposedFields fields = null;

		for (Projection projection : projections) {
			ExposedField field = projection.getExposedField();
			fields = fields == null ? ExposedFields.from(field) : fields.and(field);
		}

		return fields;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.aggregation.AggregationOperation#toDBObject(org.springframework.data.sequoiadb.core.aggregation.AggregationOperationContext)
	 */
	@Override
	public BSONObject toDBObject(AggregationOperationContext context) {

		BasicBSONObject fieldObject = new BasicBSONObject();

		for (Projection projection : projections) {
			fieldObject.putAll(projection.toDBObject(context));
		}

		return new BasicBSONObject("$project", fieldObject);
	}

	/**
	 * Base class for {@link ProjectionOperationBuilder}s.
	 * 

	 */
	private static abstract class AbstractProjectionOperationBuilder implements AggregationOperation {

		protected final Object value;
		protected final ProjectionOperation operation;

		/**
		 * Creates a new {@link AbstractProjectionOperationBuilder} fot the given value and {@link ProjectionOperation}.
		 * 
		 * @param value must not be {@literal null}.
		 * @param operation must not be {@literal null}.
		 */
		public AbstractProjectionOperationBuilder(Object value, ProjectionOperation operation) {

			Assert.notNull(value, "value must not be null or empty!");
			Assert.notNull(operation, "ProjectionOperation must not be null!");

			this.value = value;
			this.operation = operation;
		}

		/* 
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.core.aggregation.AggregationOperation#toDBObject(org.springframework.data.sequoiadb.core.aggregation.AggregationOperationContext)
		 */
		@Override
		public BSONObject toDBObject(AggregationOperationContext context) {
			return this.operation.toDBObject(context);
		}

		/**
		 * Returns the finally to be applied {@link ProjectionOperation} with the given alias.
		 * 
		 * @param alias will never be {@literal null} or empty.
		 * @return
		 */
		public abstract ProjectionOperation as(String alias);
	}

	/**
	 * An {@link ProjectionOperationBuilder} that is used for SpEL expression based projections.
	 * 

	 */
	public static class ExpressionProjectionOperationBuilder extends ProjectionOperationBuilder {

		private final Object[] params;
		private final String expression;

		/**
		 * Creates a new {@link ExpressionProjectionOperationBuilder} for the given value, {@link ProjectionOperation} and
		 * parameters.
		 * 
		 * @param expression must not be {@literal null}.
		 * @param operation must not be {@literal null}.
		 * @param parameters
		 */
		public ExpressionProjectionOperationBuilder(String expression, ProjectionOperation operation, Object[] parameters) {

			super(expression, operation, null);
			this.expression = expression;
			this.params = parameters.clone();
		}

		/* (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.core.aggregation.ProjectionOperation.ProjectionOperationBuilder#project(java.lang.String, java.lang.Object[])
		 */
		@Override
		public ProjectionOperationBuilder project(String operation, final Object... values) {

			OperationProjection operationProjection = new OperationProjection(Fields.field(value.toString()), operation,
					values) {
				@Override
				protected List<Object> getOperationArguments(AggregationOperationContext context) {

					List<Object> result = new ArrayList<Object>(values.length + 1);
					result.add(ExpressionProjection.toSequoiadbExpression(context,
							ExpressionProjectionOperationBuilder.this.expression, ExpressionProjectionOperationBuilder.this.params));
					result.addAll(Arrays.asList(values));

					return result;
				}
			};

			return new ProjectionOperationBuilder(value, this.operation.and(operationProjection), operationProjection);
		}

		/*
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.core.aggregation.ProjectionOperation.AbstractProjectionOperationBuilder#as(java.lang.String)
		 */
		@Override
		public ProjectionOperation as(String alias) {

			Field expressionField = Fields.field(alias, alias);
			return this.operation.and(new ExpressionProjection(expressionField, this.value.toString(), params));
		}

		/**
		 * A {@link Projection} based on a SpEL expression.
		 * 


		 */
		static class ExpressionProjection extends Projection {

			private static final SpelExpressionTransformer TRANSFORMER = new SpelExpressionTransformer();

			private final String expression;
			private final Object[] params;

			/**
			 * Creates a new {@link ExpressionProjection} for the given field, SpEL expression and parameters.
			 * 
			 * @param field must not be {@literal null}.
			 * @param expression must not be {@literal null} or empty.
			 * @param parameters must not be {@literal null}.
			 */
			public ExpressionProjection(Field field, String expression, Object[] parameters) {

				super(field);

				Assert.hasText(expression, "Expression must not be null!");
				Assert.notNull(parameters, "Parameters must not be null!");

				this.expression = expression;
				this.params = parameters.clone();
			}

			/* 
			 * (non-Javadoc)
			 * @see org.springframework.data.sequoiadb.core.aggregation.ProjectionOperation.Projection#toDBObject(org.springframework.data.sequoiadb.core.aggregation.AggregationOperationContext)
			 */
			@Override
			public BSONObject toDBObject(AggregationOperationContext context) {
				return new BasicBSONObject(getExposedField().getName(), toSequoiadbExpression(context, expression, params));
			}

			protected static Object toSequoiadbExpression(AggregationOperationContext context, String expression, Object[] params) {
				return TRANSFORMER.transform(expression, context, params);
			}
		}
	}

	/**
	 * Builder for {@link ProjectionOperation}s on a field.
	 * 


	 */
	public static class ProjectionOperationBuilder extends AbstractProjectionOperationBuilder {

		private static final String NUMBER_NOT_NULL = "Number must not be null!";
		private static final String FIELD_REFERENCE_NOT_NULL = "Field reference must not be null!";

		private final String name;
		private final OperationProjection previousProjection;

		/**
		 * Creates a new {@link ProjectionOperationBuilder} for the field with the given name on top of the given
		 * {@link ProjectionOperation}.
		 * 
		 * @param name must not be {@literal null} or empty.
		 * @param operation must not be {@literal null}.
		 * @param previousProjection the previous operation projection, may be {@literal null}.
		 */
		public ProjectionOperationBuilder(String name, ProjectionOperation operation, OperationProjection previousProjection) {
			super(name, operation);

			this.name = name;
			this.previousProjection = previousProjection;
		}

		/**
		 * Creates a new {@link ProjectionOperationBuilder} for the field with the given value on top of the given
		 * {@link ProjectionOperation}.
		 * 
		 * @param value
		 * @param operation
		 * @param previousProjection
		 */
		protected ProjectionOperationBuilder(Object value, ProjectionOperation operation,
				OperationProjection previousProjection) {

			super(value, operation);

			this.name = null;
			this.previousProjection = previousProjection;
		}

		/**
		 * Projects the result of the previous operation onto the current field. Will automatically add an exclusion for
		 * {@code _id} as what would be held in it by default will now go into the field just projected into.
		 * 
		 * @return
		 */
		public ProjectionOperation previousOperation() {

			return this.operation.andExclude(Fields.UNDERSCORE_ID) //
					.and(new PreviousOperationProjection(name));
		}

		/**
		 * Defines a nested field binding for the current field.
		 * 
		 * @param fields must not be {@literal null}.
		 * @return
		 */
		public ProjectionOperation nested(Fields fields) {
			return this.operation.and(new NestedFieldProjection(name, fields));
		}

		/**
		 * Allows to specify an alias for the previous projection operation.
		 * 
		 * @param string
		 * @return
		 */
		@Override
		public ProjectionOperation as(String alias) {

			if (this.previousProjection != null) {
				return this.operation.andReplaceLastOneWith(this.previousProjection.withAlias(alias));
			} else {
				return this.operation.and(new FieldProjection(Fields.field(alias, name), null));
			}
		}

		/**
		 * Generates an {@code $add} expression that adds the given number to the previously mentioned field.
		 * 
		 * @param number
		 * @return
		 */
		public ProjectionOperationBuilder plus(Number number) {

			Assert.notNull(number, NUMBER_NOT_NULL);
			return project("add", number);
		}

		/**
		 * Generates an {@code $add} expression that adds the value of the given field to the previously mentioned field.
		 * 
		 * @param fieldReference
		 * @return
		 */
		public ProjectionOperationBuilder plus(String fieldReference) {

			Assert.notNull(fieldReference, "Field reference must not be null!");
			return project("add", Fields.field(fieldReference));
		}

		/**
		 * Generates an {@code $subtract} expression that subtracts the given number to the previously mentioned field.
		 * 
		 * @param number
		 * @return
		 */
		public ProjectionOperationBuilder minus(Number number) {

			Assert.notNull(number, "Number must not be null!");
			return project("subtract", number);
		}

		/**
		 * Generates an {@code $subtract} expression that subtracts the value of the given field to the previously mentioned
		 * field.
		 * 
		 * @param fieldReference
		 * @return
		 */
		public ProjectionOperationBuilder minus(String fieldReference) {

			Assert.notNull(fieldReference, FIELD_REFERENCE_NOT_NULL);
			return project("subtract", Fields.field(fieldReference));
		}

		/**
		 * Generates an {@code $multiply} expression that multiplies the given number with the previously mentioned field.
		 * 
		 * @param number
		 * @return
		 */
		public ProjectionOperationBuilder multiply(Number number) {

			Assert.notNull(number, NUMBER_NOT_NULL);
			return project("multiply", number);
		}

		/**
		 * Generates an {@code $multiply} expression that multiplies the value of the given field with the previously
		 * mentioned field.
		 * 
		 * @param fieldReference
		 * @return
		 */
		public ProjectionOperationBuilder multiply(String fieldReference) {

			Assert.notNull(fieldReference, FIELD_REFERENCE_NOT_NULL);
			return project("multiply", Fields.field(fieldReference));
		}

		/**
		 * Generates an {@code $divide} expression that divides the previously mentioned field by the given number.
		 * 
		 * @param number
		 * @return
		 */
		public ProjectionOperationBuilder divide(Number number) {

			Assert.notNull(number, FIELD_REFERENCE_NOT_NULL);
			Assert.isTrue(Math.abs(number.intValue()) != 0, "Number must not be zero!");
			return project("divide", number);
		}

		/**
		 * Generates an {@code $divide} expression that divides the value of the given field by the previously mentioned
		 * field.
		 * 
		 * @param fieldReference
		 * @return
		 */
		public ProjectionOperationBuilder divide(String fieldReference) {

			Assert.notNull(fieldReference, FIELD_REFERENCE_NOT_NULL);
			return project("divide", Fields.field(fieldReference));
		}

		/**
		 * Generates an {@code $mod} expression that divides the previously mentioned field by the given number and returns
		 * the remainder.
		 * 
		 * @param number
		 * @return
		 */
		public ProjectionOperationBuilder mod(Number number) {

			Assert.notNull(number, NUMBER_NOT_NULL);
			Assert.isTrue(Math.abs(number.intValue()) != 0, "Number must not be zero!");
			return project("mod", number);
		}

		/**
		 * Generates an {@code $mod} expression that divides the value of the given field by the previously mentioned field
		 * and returns the remainder.
		 * 
		 * @param fieldReference
		 * @return
		 */
		public ProjectionOperationBuilder mod(String fieldReference) {

			Assert.notNull(fieldReference, FIELD_REFERENCE_NOT_NULL);
			return project("mod", Fields.field(fieldReference));
		}

		/* 
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.core.aggregation.AggregationOperation#toDBObject(org.springframework.data.sequoiadb.core.aggregation.AggregationOperationContext)
		 */
		@Override
		public BSONObject toDBObject(AggregationOperationContext context) {
			return this.operation.toDBObject(context);
		}

		/**
		 * Adds a generic projection for the current field.
		 * 
		 * @param operation the operation key, e.g. {@code $add}.
		 * @param values the values to be set for the projection operation.
		 * @return
		 */
		public ProjectionOperationBuilder project(String operation, Object... values) {
			OperationProjection operationProjection = new OperationProjection(Fields.field(value.toString()), operation,
					values);
			return new ProjectionOperationBuilder(value, this.operation.and(operationProjection), operationProjection);
		}

		/**
		 * A {@link Projection} to pull in the result of the previous operation.
		 * 

		 */
		static class PreviousOperationProjection extends Projection {

			private final String name;

			/**
			 * Creates a new {@link PreviousOperationProjection} for the field with the given name.
			 * 
			 * @param name must not be {@literal null} or empty.
			 */
			public PreviousOperationProjection(String name) {
				super(Fields.field(name));
				this.name = name;
			}

			/* 
			 * (non-Javadoc)
			 * @see org.springframework.data.sequoiadb.core.aggregation.ProjectionOperation.Projection#toDBObject(org.springframework.data.sequoiadb.core.aggregation.AggregationOperationContext)
			 */
			@Override
			public BSONObject toDBObject(AggregationOperationContext context) {
				return new BasicBSONObject(name, Fields.UNDERSCORE_ID_REF);
			}
		}

		/**
		 * A {@link FieldProjection} to map a result of a previous {@link AggregationOperation} to a new field.
		 * 


		 */
		static class FieldProjection extends Projection {

			private final Field field;
			private final Object value;

			/**
			 * Creates a new {@link FieldProjection} for the field of the given name, assigning the given value.
			 * 
			 * @param name must not be {@literal null} or empty.
			 * @param value
			 */
			public FieldProjection(String name, Object value) {
				this(Fields.field(name), value);
			}

			private FieldProjection(Field field, Object value) {

				super(field);

				this.field = field;
				this.value = value;
			}

			/**
			 * Factory method to easily create {@link FieldProjection}s for the given {@link Fields}. Fields are projected as
			 * references with their given name. A field {@code foo} will be projected as: {@code foo : 1 } .
			 * 
			 * @param fields the {@link Fields} to in- or exclude, must not be {@literal null}.
			 * @return
			 */
			public static List<? extends Projection> from(Fields fields) {
				return from(fields, null);
			}

			/**
			 * Factory method to easily create {@link FieldProjection}s for the given {@link Fields}.
			 * 
			 * @param fields the {@link Fields} to in- or exclude, must not be {@literal null}.
			 * @param value to use for the given field.
			 * @return
			 */
			public static List<FieldProjection> from(Fields fields, Object value) {

				Assert.notNull(fields, "Fields must not be null!");
				List<FieldProjection> projections = new ArrayList<FieldProjection>();

				for (Field field : fields) {
					projections.add(new FieldProjection(field, value));
				}

				return projections;
			}

			/* 
			 * (non-Javadoc)
			 * @see org.springframework.data.sequoiadb.core.aggregation.ProjectionOperation.Projection#toDBObject(org.springframework.data.sequoiadb.core.aggregation.AggregationOperationContext)
			 */
			@Override
			public BSONObject toDBObject(AggregationOperationContext context) {
				return new BasicBSONObject(field.getName(), renderFieldValue(context));
			}

			private Object renderFieldValue(AggregationOperationContext context) {

				if (value == null || Boolean.TRUE.equals(value)) {

					if (Aggregation.SystemVariable.isReferingToSystemVariable(field.getTarget())) {
						return field.getTarget();
					}

					return context.getReference(field).getReferenceValue();

				} else if (Boolean.FALSE.equals(value)) {

					return 0;
				}

				return value;
			}
		}

		static class OperationProjection extends Projection {

			private final Field field;
			private final String operation;
			private final List<Object> values;

			/**
			 * Creates a new {@link OperationProjection} for the given field.
			 * 
			 * @param field the name of the field to add the operation projection for, must not be {@literal null} or empty.
			 * @param operation the actual operation key, must not be {@literal null} or empty.
			 * @param values the values to pass into the operation, must not be {@literal null}.
			 */
			public OperationProjection(Field field, String operation, Object[] values) {

				super(field);

				Assert.hasText(operation, "Operation must not be null or empty!");
				Assert.notNull(values, "Values must not be null!");

				this.field = field;
				this.operation = operation;
				this.values = Arrays.asList(values);
			}

			/* 
			 * (non-Javadoc)
			 * @see org.springframework.data.sequoiadb.core.aggregation.ProjectionOperation.Projection#toDBObject(org.springframework.data.sequoiadb.core.aggregation.AggregationOperationContext)
			 */
			@Override
			public BSONObject toDBObject(AggregationOperationContext context) {

				BSONObject inner = new BasicBSONObject("$" + operation, getOperationArguments(context));

				return new BasicBSONObject(getField().getName(), inner);
			}

			protected List<Object> getOperationArguments(AggregationOperationContext context) {

				List<Object> result = new ArrayList<Object>(values.size());
				result.add(context.getReference(getField().getName()).toString());

				for (Object element : values) {
					result.add(element instanceof Field ? context.getReference((Field) element).toString() : element);
				}

				return result;
			}

			/**
			 * Returns the field that holds the {@link OperationProjection}.
			 * 
			 * @return
			 */
			protected Field getField() {
				return field;
			}

			/**
			 * Creates a new instance of this {@link OperationProjection} with the given alias.
			 * 
			 * @param alias the alias to set
			 * @return
			 */
			public OperationProjection withAlias(String alias) {

				final Field aliasedField = Fields.field(alias, this.field.getName());
				return new OperationProjection(aliasedField, operation, values.toArray()) {

					/* (non-Javadoc)
					 * @see org.springframework.data.sequoiadb.core.aggregation.ProjectionOperation.ProjectionOperationBuilder.OperationProjection#getField()
					 */
					@Override
					protected Field getField() {
						return aliasedField;
					}

					@Override
					protected List<Object> getOperationArguments(AggregationOperationContext context) {


						return OperationProjection.this.getOperationArguments(context);
					}
				};
			}
		}

		static class NestedFieldProjection extends Projection {

			private final String name;
			private final Fields fields;

			public NestedFieldProjection(String name, Fields fields) {

				super(Fields.field(name));
				this.name = name;
				this.fields = fields;
			}

			/* 
			 * (non-Javadoc)
			 * @see org.springframework.data.sequoiadb.core.aggregation.ProjectionOperation.Projection#toDBObject(org.springframework.data.sequoiadb.core.aggregation.AggregationOperationContext)
			 */
			@Override
			public BSONObject toDBObject(AggregationOperationContext context) {

				BSONObject nestedObject = new BasicBSONObject();

				for (Field field : fields) {
					nestedObject.put(field.getName(), context.getReference(field.getTarget()).toString());
				}

				return new BasicBSONObject(name, nestedObject);
			}
		}

		/**
		 * Extracts the minute from a date expression.
		 * 
		 * @return
		 */
		public ProjectionOperationBuilder extractMinute() {
			return project("minute");
		}

		/**
		 * Extracts the hour from a date expression.
		 * 
		 * @return
		 */
		public ProjectionOperationBuilder extractHour() {
			return project("hour");
		}

		/**
		 * Extracts the second from a date expression.
		 * 
		 * @return
		 */
		public ProjectionOperationBuilder extractSecond() {
			return project("second");
		}

		/**
		 * Extracts the millisecond from a date expression.
		 * 
		 * @return
		 */
		public ProjectionOperationBuilder extractMillisecond() {
			return project("millisecond");
		}

		/**
		 * Extracts the year from a date expression.
		 * 
		 * @return
		 */
		public ProjectionOperationBuilder extractYear() {
			return project("year");
		}

		/**
		 * Extracts the month from a date expression.
		 * 
		 * @return
		 */
		public ProjectionOperationBuilder extractMonth() {
			return project("month");
		}

		/**
		 * Extracts the week from a date expression.
		 * 
		 * @return
		 */
		public ProjectionOperationBuilder extractWeek() {
			return project("week");
		}

		/**
		 * Extracts the dayOfYear from a date expression.
		 * 
		 * @return
		 */
		public ProjectionOperationBuilder extractDayOfYear() {
			return project("dayOfYear");
		}

		/**
		 * Extracts the dayOfMonth from a date expression.
		 * 
		 * @return
		 */
		public ProjectionOperationBuilder extractDayOfMonth() {
			return project("dayOfMonth");
		}

		/**
		 * Extracts the dayOfWeek from a date expression.
		 * 
		 * @return
		 */
		public ProjectionOperationBuilder extractDayOfWeek() {
			return project("dayOfWeek");
		}
	}

	/**
	 * Base class for {@link Projection} implementations.
	 * 

	 */
	private static abstract class Projection {

		private final ExposedField field;

		/**
		 * Creates new {@link Projection} for the given {@link Field}.
		 * 
		 * @param field must not be {@literal null}.
		 */
		public Projection(Field field) {

			Assert.notNull(field, "Field must not be null!");
			this.field = new ExposedField(field, true);
		}

		/**
		 * Returns the field exposed by the {@link Projection}.
		 * 
		 * @return will never be {@literal null}.
		 */
		public ExposedField getExposedField() {
			return field;
		}

		/**
		 * Renders the current {@link Projection} into a {@link BSONObject} based on the given
		 * {@link AggregationOperationContext}.
		 * 
		 * @param context will never be {@literal null}.
		 * @return
		 */
		public abstract BSONObject toDBObject(AggregationOperationContext context);
	}
}
