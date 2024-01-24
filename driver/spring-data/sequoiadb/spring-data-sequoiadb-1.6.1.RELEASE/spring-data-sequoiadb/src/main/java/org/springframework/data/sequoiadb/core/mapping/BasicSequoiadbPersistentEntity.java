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

import java.lang.reflect.Field;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Map;

import org.springframework.beans.BeansException;
import org.springframework.context.ApplicationContext;
import org.springframework.context.ApplicationContextAware;
import org.springframework.context.expression.BeanFactoryAccessor;
import org.springframework.context.expression.BeanFactoryResolver;
import org.springframework.data.annotation.Id;
import org.springframework.data.mapping.Association;
import org.springframework.data.mapping.AssociationHandler;
import org.springframework.data.mapping.PropertyHandler;
import org.springframework.data.mapping.model.BasicPersistentEntity;
import org.springframework.data.mapping.model.MappingException;
import org.springframework.data.sequoiadb.SequoiadbCollectionUtils;
import org.springframework.data.util.TypeInformation;
import org.springframework.expression.Expression;
import org.springframework.expression.ParserContext;
import org.springframework.expression.spel.standard.SpelExpressionParser;
import org.springframework.expression.spel.support.StandardEvaluationContext;
import org.springframework.util.Assert;
import org.springframework.util.ClassUtils;
import org.springframework.util.StringUtils;

/**
 * SequoiaDB specific {@link SequoiadbPersistentEntity} implementation that adds Sdb specific meta-data such as the
 * collection name and the like.
 *
 */
public class BasicSequoiadbPersistentEntity<T> extends BasicPersistentEntity<T, SequoiadbPersistentProperty> implements
		SequoiadbPersistentEntity<T>, ApplicationContextAware {

	private static final String AMBIGUOUS_FIELD_MAPPING = "Ambiguous field mapping detected! Both %s and %s map to the same field name %s! Disambiguate using @DocumentField annotation!";
	private final String collection;
	private final String language;
	private final SpelExpressionParser parser;
	private final StandardEvaluationContext context;

	/**
	 * Creates a new {@link BasicSequoiadbPersistentEntity} with the given {@link TypeInformation}. Will default the
	 * collection name to the entities simple type name.
	 * 
	 * @param typeInformation
	 */
	public BasicSequoiadbPersistentEntity(TypeInformation<T> typeInformation) {

		super(typeInformation, SequoiadbPersistentPropertyComparator.INSTANCE);

		this.parser = new SpelExpressionParser();
		this.context = new StandardEvaluationContext();

		Class<?> rawType = typeInformation.getType();
		String fallback = SequoiadbCollectionUtils.getPreferredCollectionName(rawType);

		if (rawType.isAnnotationPresent(Document.class)) {
			Document d = rawType.getAnnotation(Document.class);
			this.collection = StringUtils.hasText(d.collection()) ? d.collection() : fallback;
			this.language = StringUtils.hasText(d.language()) ? d.language() : "";
		} else {
			this.collection = fallback;
			this.language = "";
		}
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.context.ApplicationContextAware#setApplicationContext(org.springframework.context.ApplicationContext)
	 */
	public void setApplicationContext(ApplicationContext applicationContext) throws BeansException {

		context.addPropertyAccessor(new BeanFactoryAccessor());
		context.setBeanResolver(new BeanFactoryResolver(applicationContext));
		context.setRootObject(applicationContext);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity#getCollection()
	 */
	public String getCollection() {
		Expression expression = parser.parseExpression(collection, ParserContext.TEMPLATE_EXPRESSION);
		return expression.getValue(context, String.class);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity#getLanguage()
	 */
	@Override
	public String getLanguage() {
		return this.language;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity#getTextScoreProperty()
	 */
	@Override
	public SequoiadbPersistentProperty getTextScoreProperty() {
		return getPersistentProperty(TextScore.class);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity#hasTextScoreProperty()
	 */
	@Override
	public boolean hasTextScoreProperty() {
		return getTextScoreProperty() != null;
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.mapping.model.BasicPersistentEntity#verify()
	 */
	@Override
	public void verify() {

		verifyFieldUniqueness();
		verifyFieldTypes();
	}

	private void verifyFieldUniqueness() {

		AssertFieldNameUniquenessHandler handler = new AssertFieldNameUniquenessHandler();

		doWithProperties(handler);
		doWithAssociations(handler);
	}

	private void verifyFieldTypes() {
		doWithProperties(new PropertyTypeAssertionHandler());
	}

	/**
	 * {@link Comparator} implementation inspecting the {@link SequoiadbPersistentProperty}'s order.
	 * 

	 */
	static enum SequoiadbPersistentPropertyComparator implements Comparator<SequoiadbPersistentProperty> {

		INSTANCE;

		/*
		 * (non-Javadoc)
		 * @see java.util.Comparator#compare(java.lang.Object, java.lang.Object)
		 */
		public int compare(SequoiadbPersistentProperty o1, SequoiadbPersistentProperty o2) {

			if (o1.getFieldOrder() == Integer.MAX_VALUE) {
				return 1;
			}

			if (o2.getFieldOrder() == Integer.MAX_VALUE) {
				return -1;
			}

			return o1.getFieldOrder() - o2.getFieldOrder();
		}
	}

	/**
	 * As a general note: An implicit id property has a name that matches "id" or "_id". An explicit id property is one
	 * that is annotated with @see {@link Id}. The property id is updated according to the following rules: 1) An id
	 * property which is defined explicitly takes precedence over an implicitly defined id property. 2) In case of any
	 * ambiguity a @see {@link MappingException} is thrown.
	 * 
	 * @param property - the new id property candidate
	 * @return
	 */
	@Override
	protected SequoiadbPersistentProperty returnPropertyIfBetterIdPropertyCandidateOrNull(SequoiadbPersistentProperty property) {

		Assert.notNull(property);

		if (!property.isIdProperty()) {
			return null;
		}

		SequoiadbPersistentProperty currentIdProperty = getIdProperty();

		boolean currentIdPropertyIsSet = currentIdProperty != null;
		@SuppressWarnings("null")
		boolean currentIdPropertyIsExplicit = currentIdPropertyIsSet ? currentIdProperty.isExplicitIdProperty() : false;
		boolean newIdPropertyIsExplicit = property.isExplicitIdProperty();

		if (!currentIdPropertyIsSet) {
			return property;

		}

		@SuppressWarnings("null")
		Field currentIdPropertyField = currentIdProperty.getField();

		if (newIdPropertyIsExplicit && currentIdPropertyIsExplicit) {
			throw new MappingException(String.format(
					"Attempt to add explicit id property %s but already have an property %s registered "
							+ "as explicit id. Check your mapping configuration!", property.getField(), currentIdPropertyField));

		} else if (newIdPropertyIsExplicit && !currentIdPropertyIsExplicit) {
			// explicit id property takes precedence over implicit id property
			return property;

		} else if (!newIdPropertyIsExplicit && currentIdPropertyIsExplicit) {
			// no id property override - current property is explicitly defined

		} else {
			throw new MappingException(String.format(
					"Attempt to add id property %s but already have an property %s registered "
							+ "as id. Check your mapping configuration!", property.getField(), currentIdPropertyField));
		}

		return null;
	}

	/**
	 * Handler to collect {@link SequoiadbPersistentProperty} instances and check that each of them is mapped to a distinct
	 * field name.
	 * 

	 */
	private static class AssertFieldNameUniquenessHandler implements PropertyHandler<SequoiadbPersistentProperty>,
			AssociationHandler<SequoiadbPersistentProperty> {

		private final Map<String, SequoiadbPersistentProperty> properties = new HashMap<String, SequoiadbPersistentProperty>();

		public void doWithPersistentProperty(SequoiadbPersistentProperty persistentProperty) {
			assertUniqueness(persistentProperty);
		}

		public void doWithAssociation(Association<SequoiadbPersistentProperty> association) {
			assertUniqueness(association.getInverse());
		}

		private void assertUniqueness(SequoiadbPersistentProperty property) {

			String fieldName = property.getFieldName();
			SequoiadbPersistentProperty existingProperty = properties.get(fieldName);

			if (existingProperty != null) {
				throw new MappingException(String.format(AMBIGUOUS_FIELD_MAPPING, property.toString(),
						existingProperty.toString(), fieldName));
			}

			properties.put(fieldName, property);
		}
	}

	/**

	 * @since 1.6
	 */
	private static class PropertyTypeAssertionHandler implements PropertyHandler<SequoiadbPersistentProperty> {

		@Override
		public void doWithPersistentProperty(SequoiadbPersistentProperty persistentProperty) {

			potentiallyAssertTextScoreType(persistentProperty);
			potentiallyAssertLanguageType(persistentProperty);
		}

		private void potentiallyAssertLanguageType(SequoiadbPersistentProperty persistentProperty) {

			if (persistentProperty.isExplicitLanguageProperty()) {
				assertPropertyType(persistentProperty, String.class);
			}
		}

		private void potentiallyAssertTextScoreType(SequoiadbPersistentProperty persistentProperty) {

			if (persistentProperty.isTextScoreProperty()) {
				assertPropertyType(persistentProperty, Float.class, Double.class);
			}
		}

		private void assertPropertyType(SequoiadbPersistentProperty persistentProperty, Class<?>... validMatches) {

			for (Class<?> potentialMatch : validMatches) {
				if (ClassUtils.isAssignable(potentialMatch, persistentProperty.getActualType())) {
					return;
				}
			}

			throw new MappingException(String.format("Missmatching types for %s. Found %s expected one of %s.",
					persistentProperty.getField(), persistentProperty.getActualType(),
					StringUtils.arrayToCommaDelimitedString(validMatches)));
		}
	}

}
