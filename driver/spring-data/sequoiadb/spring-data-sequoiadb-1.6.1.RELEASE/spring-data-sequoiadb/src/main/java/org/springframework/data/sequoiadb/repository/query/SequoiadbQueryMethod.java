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
package org.springframework.data.sequoiadb.repository.query;

import java.io.Serializable;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.List;

import org.springframework.core.annotation.AnnotationUtils;
import org.springframework.data.geo.GeoPage;
import org.springframework.data.geo.GeoResult;
import org.springframework.data.geo.GeoResults;
import org.springframework.data.mapping.context.MappingContext;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.data.sequoiadb.repository.Meta;
import org.springframework.data.sequoiadb.repository.Query;
import org.springframework.data.repository.core.RepositoryMetadata;
import org.springframework.data.repository.query.QueryMethod;
import org.springframework.data.util.ClassTypeInformation;
import org.springframework.data.util.TypeInformation;
import org.springframework.util.Assert;
import org.springframework.util.StringUtils;

/**
 * Sdb specific implementation of {@link QueryMethod}.
 * 


 */
public class SequoiadbQueryMethod extends QueryMethod {

	@SuppressWarnings("unchecked") private static final List<Class<? extends Serializable>> GEO_NEAR_RESULTS = Arrays
			.asList(GeoResult.class, GeoResults.class, GeoPage.class);

	private final Method method;
	private final MappingContext<? extends SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> mappingContext;

	private SequoiadbEntityMetadata<?> metadata;

	/**
	 * Creates a new {@link SequoiadbQueryMethod} from the given {@link Method}.
	 * 
	 * @param method
	 */
	public SequoiadbQueryMethod(Method method, RepositoryMetadata metadata,
								MappingContext<? extends SequoiadbPersistentEntity<?>, SequoiadbPersistentProperty> mappingContext) {

		super(method, metadata);

		Assert.notNull(mappingContext, "MappingContext must not be null!");

		this.method = method;
		this.mappingContext = mappingContext;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.query.QueryMethod#getParameters(java.lang.reflect.Method)
	 */
	@Override
	protected SequoiadbParameters createParameters(Method method) {
		return new SequoiadbParameters(method, isGeoNearQuery(method));
	}

	/**
	 * Returns whether the method has an annotated query.
	 * 
	 * @return
	 */
	public boolean hasAnnotatedQuery() {
		return getAnnotatedQuery() != null;
	}

	/**
	 * Returns the query string declared in a {@link Query} annotation or {@literal null} if neither the annotation found
	 * nor the attribute was specified.
	 * 
	 * @return
	 */
	String getAnnotatedQuery() {

		String query = (String) AnnotationUtils.getValue(getQueryAnnotation());
		return StringUtils.hasText(query) ? query : null;
	}

	/**
	 * Returns the field specification to be used for the query.
	 * 
	 * @return
	 */
	String getFieldSpecification() {

		String value = (String) AnnotationUtils.getValue(getQueryAnnotation(), "fields");
		return StringUtils.hasText(value) ? value : null;
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.query.QueryMethod#getEntityInformation()
	 */
	@Override
	@SuppressWarnings("unchecked")
	public SequoiadbEntityMetadata<?> getEntityInformation() {

		if (metadata == null) {

			Class<?> returnedObjectType = getReturnedObjectType();
			Class<?> domainClass = getDomainClass();

			SequoiadbPersistentEntity<?> returnedEntity = mappingContext.getPersistentEntity(getReturnedObjectType());
			SequoiadbPersistentEntity<?> managedEntity = mappingContext.getPersistentEntity(domainClass);
			returnedEntity = returnedEntity == null ? managedEntity : returnedEntity;
			SequoiadbPersistentEntity<?> collectionEntity = domainClass.isAssignableFrom(returnedObjectType) ? returnedEntity
					: managedEntity;

			this.metadata = new SimpleSequoiadbEntityMetadata<Object>((Class<Object>) returnedEntity.getType(),
					collectionEntity.getCollection());
		}

		return this.metadata;
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.query.QueryMethod#getParameters()
	 */
	@Override
	public SequoiadbParameters getParameters() {
		return (SequoiadbParameters) super.getParameters();
	}

	/**
	 * Returns whether the query is a geo near query.
	 * 
	 * @return
	 */
	public boolean isGeoNearQuery() {
		return isGeoNearQuery(this.method);
	}

	private boolean isGeoNearQuery(Method method) {

		Class<?> returnType = method.getReturnType();

		for (Class<?> type : GEO_NEAR_RESULTS) {
			if (type.isAssignableFrom(returnType)) {
				return true;
			}
		}

		if (Iterable.class.isAssignableFrom(returnType)) {
			TypeInformation<?> from = ClassTypeInformation.fromReturnTypeOf(method);
			return GeoResult.class.equals(from.getComponentType().getType());
		}

		return false;
	}

	/**
	 * Returns the {@link Query} annotation that is applied to the method or {@code null} if none available.
	 * 
	 * @return
	 */
	Query getQueryAnnotation() {
		return method.getAnnotation(Query.class);
	}

	TypeInformation<?> getReturnType() {
		return ClassTypeInformation.fromReturnTypeOf(method);
	}

	/**
	 * @return return true if {@link Meta} annotation is available.
	 * @since 1.6
	 */
	public boolean hasQueryMetaAttributes() {
		return getMetaAnnotation() != null;
	}

	/**
	 * Returns the {@link Meta} annotation that is applied to the method or {@code null} if not available.
	 * 
	 * @return
	 * @since 1.6
	 */
	Meta getMetaAnnotation() {
		return method.getAnnotation(Meta.class);
	}

	/**
	 * Returns the {@link org.springframework.data.sequoiadb.core.query.Meta} attributes to be applied.
	 * 
	 * @return never {@literal null}.
	 * @since 1.6
	 */
	public org.springframework.data.sequoiadb.core.query.Meta getQueryMetaAttributes() {

		Meta meta = getMetaAnnotation();
		if (meta == null) {
			return new org.springframework.data.sequoiadb.core.query.Meta();
		}

		org.springframework.data.sequoiadb.core.query.Meta metaAttributes = new org.springframework.data.sequoiadb.core.query.Meta();
		if (meta.maxExcecutionTime() > 0) {
			metaAttributes.setMaxTimeMsec(meta.maxExcecutionTime());
		}

		if (meta.maxScanDocuments() > 0) {
			metaAttributes.setMaxScan(meta.maxScanDocuments());
		}

		if (StringUtils.hasText(meta.comment())) {
			metaAttributes.setComment(meta.comment());
		}

		if (meta.snapshot()) {
			metaAttributes.setSnapshot(meta.snapshot());
		}

		return metaAttributes;
	}
}
