/*
 * Copyright 2014 the original author or authors.
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
package org.springframework.data.mongodb.core.convert;

import org.springframework.data.mapping.context.MappingContext;
import org.springframework.data.mapping.model.BeanWrapper;
import org.springframework.data.mapping.model.DefaultSpELExpressionEvaluator;
import org.springframework.data.mapping.model.SpELContext;
import org.springframework.data.mapping.model.SpELExpressionEvaluator;
import org.springframework.data.mongodb.core.mapping.MongoPersistentEntity;
import org.springframework.data.mongodb.core.mapping.MongoPersistentProperty;

import org.springframework.data.mongodb.assist.BasicDBObject;
import org.springframework.data.mongodb.assist.DBObject;
import org.springframework.data.mongodb.assist.DBRef;

/**
 * @author Oliver Gierke
 */
class DefaultDbRefProxyHandler implements DbRefProxyHandler {

	private final SpELContext spELContext;
	private final MappingContext<? extends MongoPersistentEntity<?>, MongoPersistentProperty> mappingContext;
	private final ValueResolver resolver;

	/**
	 * @param spELContext must not be {@literal null}.
	 * @param conversionService must not be {@literal null}.
	 * @param mappingContext must not be {@literal null}.
	 */
	public DefaultDbRefProxyHandler(SpELContext spELContext,
			MappingContext<? extends MongoPersistentEntity<?>, MongoPersistentProperty> mappingContext, ValueResolver resolver) {

		this.spELContext = spELContext;
		this.mappingContext = mappingContext;
		this.resolver = resolver;
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.mongodb.core.convert.DbRefProxyHandler#populateId(org.springframework.data.mongodb.assist.DBRef, java.lang.Object)
	 */
	@Override
	public Object populateId(MongoPersistentProperty property, DBRef source, Object proxy) {

		if (source == null) {
			return proxy;
		}

		MongoPersistentEntity<?> persistentEntity = mappingContext.getPersistentEntity(property);
		MongoPersistentProperty idProperty = persistentEntity.getIdProperty();
		
		if(idProperty.usePropertyAccess()) {
			return proxy;
		}
		
		SpELExpressionEvaluator evaluator = new DefaultSpELExpressionEvaluator(proxy, spELContext);
		BeanWrapper<Object> proxyWrapper = BeanWrapper.create(proxy, null);
		
		DBObject object = new BasicDBObject(idProperty.getFieldName(), source.getId());
		ObjectPath objectPath = ObjectPath.ROOT.push(proxy, persistentEntity, null);
		proxyWrapper.setProperty(idProperty, resolver.getValueInternal(idProperty, object, evaluator, objectPath));

		return proxy;
	}
}
