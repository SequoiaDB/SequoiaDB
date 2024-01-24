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
package org.springframework.data.mongodb.repository.query;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.bson.util.JSON;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.data.mongodb.core.MongoOperations;
import org.springframework.data.mongodb.core.query.BasicQuery;
import org.springframework.data.mongodb.core.query.Query;
import org.springframework.util.StringUtils;

import org.springframework.data.mongodb.assist.DBObject;
import org.springframework.data.mongodb.assist.DBRef;

/**
 * Query to use a plain JSON String to create the {@link Query} to actually execute.
 * 
 * @author Oliver Gierke
 * @author Christoph Strobl
 * @author Thomas Darimont
 */
public class StringBasedMongoQuery extends AbstractMongoQuery {

	private static final String COUND_AND_DELETE = "Manually defined query for %s cannot be both a count and delete query at the same time!";
	private static final Logger LOG = LoggerFactory.getLogger(StringBasedMongoQuery.class);
	private static final ParameterBindingParser PARSER = ParameterBindingParser.INSTANCE;

	private final String query;
	private final String fieldSpec;
	private final boolean isCountQuery;
	private final boolean isDeleteQuery;
	private final List<ParameterBinding> queryParameterBindings;
	private final List<ParameterBinding> fieldSpecParameterBindings;

	/**
	 * Creates a new {@link StringBasedMongoQuery} for the given {@link MongoQueryMethod} and {@link MongoOperations}.
	 * 
	 * @param method must not be {@literal null}.
	 * @param mongoOperations must not be {@literal null}.
	 */
	public StringBasedMongoQuery(MongoQueryMethod method, MongoOperations mongoOperations) {
		this(method.getAnnotatedQuery(), method, mongoOperations);
	}

	/**
	 * Creates a new {@link StringBasedMongoQuery} for the given {@link String}, {@link MongoQueryMethod} and
	 * {@link MongoOperations}.
	 * 
	 * @param method must not be {@literal null}.
	 * @param template must not be {@literal null}.
	 */
	public StringBasedMongoQuery(String query, MongoQueryMethod method, MongoOperations mongoOperations) {

		super(method, mongoOperations);

		this.query = query;
		this.queryParameterBindings = PARSER.parseParameterBindingsFrom(query);

		this.fieldSpec = method.getFieldSpecification();
		this.fieldSpecParameterBindings = PARSER.parseParameterBindingsFrom(method.getFieldSpecification());

		this.isCountQuery = method.hasAnnotatedQuery() ? method.getQueryAnnotation().count() : false;
		this.isDeleteQuery = method.hasAnnotatedQuery() ? method.getQueryAnnotation().delete() : false;

		if (isCountQuery && isDeleteQuery) {
			throw new IllegalArgumentException(String.format(COUND_AND_DELETE, method));
		}
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.repository.query.AbstractMongoQuery#createQuery(org.springframework.data.mongodb.repository.query.ConvertingParameterAccessor)
	 */
	@Override
	protected Query createQuery(ConvertingParameterAccessor accessor) {

		String queryString = replacePlaceholders(query, accessor, queryParameterBindings);

		Query query = null;

		if (fieldSpec != null) {
			String fieldString = replacePlaceholders(fieldSpec, accessor, fieldSpecParameterBindings);
			query = new BasicQuery(queryString, fieldString);
		} else {
			query = new BasicQuery(queryString);
		}

		query.with(accessor.getSort());

		if (LOG.isDebugEnabled()) {
			LOG.debug(String.format("Created query %s", query.getQueryObject()));
		}

		return query;
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.repository.query.AbstractMongoQuery#isCountQuery()
	 */
	@Override
	protected boolean isCountQuery() {
		return isCountQuery;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.repository.query.AbstractMongoQuery#isDeleteQuery()
	 */
	@Override
	protected boolean isDeleteQuery() {
		return this.isDeleteQuery;
	}

	/**
	 * Replaced the parameter place-holders with the actual parameter values from the given {@link ParameterBinding}s.
	 * 
	 * @param input
	 * @param accessor
	 * @param bindings
	 * @return
	 */
	private String replacePlaceholders(String input, ConvertingParameterAccessor accessor, List<ParameterBinding> bindings) {

		if (bindings.isEmpty()) {
			return input;
		}

		StringBuilder result = new StringBuilder(input);

		for (ParameterBinding binding : bindings) {

			String parameter = binding.getParameter();
			int idx = result.indexOf(parameter);

			if (idx != -1) {
				result.replace(idx, idx + parameter.length(), getParameterValueForBinding(accessor, binding));
			}
		}

		return result.toString();
	}

	/**
	 * Returns the serialized value to be used for the given {@link ParameterBinding}.
	 * 
	 * @param accessor
	 * @param binding
	 * @return
	 */
	private String getParameterValueForBinding(ConvertingParameterAccessor accessor, ParameterBinding binding) {

		Object value = accessor.getBindableValue(binding.getParameterIndex());

		if (value instanceof String && binding.isQuoted()) {
			return (String) value;
		}

		return JSON.serialize(value);
	}

	/**
	 * A parser that extracts the parameter bindings from a given query string.
	 * 
	 * @author Thomas Darimont
	 */
	private static enum ParameterBindingParser {

		INSTANCE;

		private static final String PARAMETER_PREFIX = "_param_";
		private static final String PARSEABLE_PARAMETER = "\"" + PARAMETER_PREFIX + "$1\"";
		private static final Pattern PARAMETER_BINDING_PATTERN = Pattern.compile("\\?(\\d+)");
		private static final Pattern PARSEABLE_BINDING_PATTERN = Pattern.compile("\"?" + PARAMETER_PREFIX + "(\\d+)\"?");

		private final static int PARAMETER_INDEX_GROUP = 1;

		/**
		 * Returns a list of {@link ParameterBinding}s found in the given {@code input} or an
		 * {@link Collections#emptyList()}.
		 * 
		 * @param input
		 * @param conversionService must not be {@literal null}.
		 * @return
		 */
		public List<ParameterBinding> parseParameterBindingsFrom(String input) {

			if (!StringUtils.hasText(input)) {
				return Collections.emptyList();
			}

			List<ParameterBinding> bindings = new ArrayList<ParameterBinding>();

			String parseableInput = makeParameterReferencesParseable(input);

			collectParameterReferencesIntoBindings(bindings, JSON.parse(parseableInput));

			return bindings;
		}

		private String makeParameterReferencesParseable(String input) {

			Matcher matcher = PARAMETER_BINDING_PATTERN.matcher(input);
			String parseableInput = matcher.replaceAll(PARSEABLE_PARAMETER);

			return parseableInput;
		}

		private void collectParameterReferencesIntoBindings(List<ParameterBinding> bindings, Object value) {

			if (value instanceof String) {

				String string = ((String) value).trim();
				potentiallyAddBinding(string, bindings);

			} else if (value instanceof Pattern) {

				String string = ((Pattern) value).toString().trim();

				Matcher valueMatcher = PARSEABLE_BINDING_PATTERN.matcher(string);
				while (valueMatcher.find()) {
					int paramIndex = Integer.parseInt(valueMatcher.group(PARAMETER_INDEX_GROUP));

					/*
					 * The pattern is used as a direct parameter replacement, e.g. 'field': ?1, 
					 * therefore we treat it as not quoted to remain backwards compatible.
					 */
					boolean quoted = !string.equals(PARAMETER_PREFIX + paramIndex);

					bindings.add(new ParameterBinding(paramIndex, quoted));
				}

			} else if (value instanceof DBRef) {

				DBRef dbref = (DBRef) value;

				potentiallyAddBinding(dbref.getRef(), bindings);
				potentiallyAddBinding(dbref.getId().toString(), bindings);

			} else if (value instanceof DBObject) {

				DBObject dbo = (DBObject) value;

				for (String field : dbo.keySet()) {
					collectParameterReferencesIntoBindings(bindings, field);
					collectParameterReferencesIntoBindings(bindings, dbo.get(field));
				}
			}
		}

		private void potentiallyAddBinding(String source, List<ParameterBinding> bindings) {

			Matcher valueMatcher = PARSEABLE_BINDING_PATTERN.matcher(source);

			while (valueMatcher.find()) {

				int paramIndex = Integer.parseInt(valueMatcher.group(PARAMETER_INDEX_GROUP));
				boolean quoted = (source.startsWith("'") && source.endsWith("'"))
						|| (source.startsWith("\"") && source.endsWith("\""));

				bindings.add(new ParameterBinding(paramIndex, quoted));
			}
		}
	}

	/**
	 * A generic parameter binding with name or position information.
	 * 
	 * @author Thomas Darimont
	 */
	private static class ParameterBinding {

		private final int parameterIndex;
		private final boolean quoted;

		/**
		 * Creates a new {@link ParameterBinding} with the given {@code parameterIndex} and {@code quoted} information.
		 * 
		 * @param parameterIndex
		 * @param quoted whether or not the parameter is already quoted.
		 */
		public ParameterBinding(int parameterIndex, boolean quoted) {

			this.parameterIndex = parameterIndex;
			this.quoted = quoted;
		}

		public boolean isQuoted() {
			return quoted;
		}

		public int getParameterIndex() {
			return parameterIndex;
		}

		public String getParameter() {
			return "?" + parameterIndex;
		}
	}
}
