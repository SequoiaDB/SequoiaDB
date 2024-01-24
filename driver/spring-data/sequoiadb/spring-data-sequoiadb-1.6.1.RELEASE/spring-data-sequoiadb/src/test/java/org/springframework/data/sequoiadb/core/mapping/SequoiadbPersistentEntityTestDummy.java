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
package org.springframework.data.sequoiadb.core.mapping;

import java.lang.annotation.Annotation;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

import org.springframework.data.annotation.Id;
import org.springframework.data.annotation.Version;
import org.springframework.data.mapping.AssociationHandler;
import org.springframework.data.mapping.PersistentProperty;
import org.springframework.data.mapping.PreferredConstructor;
import org.springframework.data.mapping.PropertyHandler;
import org.springframework.data.mapping.SimpleAssociationHandler;
import org.springframework.data.mapping.SimplePropertyHandler;
import org.springframework.data.util.TypeInformation;

/**
 * Trivial dummy implementation of {@link SequoiadbPersistentEntity} to be used in tests.
 * 

 * @param <T>
 */
public class SequoiadbPersistentEntityTestDummy<T> implements SequoiadbPersistentEntity<T> {

	private Map<Class<?>, Annotation> annotations = new HashMap<Class<?>, Annotation>();
	private Collection<SequoiadbPersistentProperty> properties = new ArrayList<SequoiadbPersistentProperty>();
	private String collection;
	private String name;
	private Class<T> type;

	@Override
	public String getName() {
		return name;
	}

	@Override
	public PreferredConstructor<T, SequoiadbPersistentProperty> getPersistenceConstructor() {
		return null;
	}

	@Override
	public boolean isConstructorArgument(PersistentProperty<?> property) {
		return false;
	}

	@Override
	public boolean isIdProperty(PersistentProperty<?> property) {
		return property != null ? property.isIdProperty() : false;
	}

	@Override
	public boolean isVersionProperty(PersistentProperty<?> property) {
		return property != null ? property.isIdProperty() : false;
	}

	@Override
	public SequoiadbPersistentProperty getIdProperty() {
		return getPersistentProperty(Id.class);
	}

	@Override
	public SequoiadbPersistentProperty getVersionProperty() {
		return getPersistentProperty(Version.class);
	}

	@Override
	public SequoiadbPersistentProperty getPersistentProperty(String name) {

		for (SequoiadbPersistentProperty p : this.properties) {
			if (p.getName().equals(name)) {
				return p;
			}
		}
		return null;
	}

	@Override
	public SequoiadbPersistentProperty getPersistentProperty(Class<? extends Annotation> annotationType) {

		for (SequoiadbPersistentProperty p : this.properties) {
			if (p.isAnnotationPresent(annotationType)) {
				return p;
			}
		}
		return null;
	}

	@Override
	public boolean hasIdProperty() {
		return false;
	}

	@Override
	public boolean hasVersionProperty() {
		return getVersionProperty() != null;
	}

	@Override
	public Class<T> getType() {
		return this.type;
	}

	@Override
	public Object getTypeAlias() {
		return null;
	}

	@Override
	public TypeInformation<T> getTypeInformation() {
		return null;
	}

	@Override
	public void doWithProperties(PropertyHandler<SequoiadbPersistentProperty> handler) {

		for (SequoiadbPersistentProperty p : this.properties) {
			handler.doWithPersistentProperty(p);
		}
	}

	@Override
	public void doWithProperties(SimplePropertyHandler handler) {

		for (SequoiadbPersistentProperty p : this.properties) {
			handler.doWithPersistentProperty(p);
		}
	}

	@Override
	public void doWithAssociations(AssociationHandler<SequoiadbPersistentProperty> handler) {

	}

	@Override
	public void doWithAssociations(SimpleAssociationHandler handler) {

	}

	@SuppressWarnings("unchecked")
	@Override
	public <A extends Annotation> A findAnnotation(Class<A> annotationType) {
		return (A) this.annotations.get(annotationType);
	}

	@Override
	public String getCollection() {
		return this.collection;
	}

	/**
	 * Simple builder to create {@link SequoiadbPersistentEntityTestDummy} with defined properties.
	 * 

	 * @param <T>
	 */
	public static class SequoiadbPersistentEntityDummyBuilder<T> {

		private SequoiadbPersistentEntityTestDummy<T> instance;

		private SequoiadbPersistentEntityDummyBuilder(Class<T> type) {
			this.instance = new SequoiadbPersistentEntityTestDummy<T>();
			this.instance.type = type;
		}

		@SuppressWarnings({ "rawtypes", "unchecked" })
		public static <T> SequoiadbPersistentEntityDummyBuilder<T> forClass(Class<T> type) {
			return new SequoiadbPersistentEntityDummyBuilder(type);
		}

		public SequoiadbPersistentEntityDummyBuilder<T> withName(String name) {
			this.instance.name = name;
			return this;
		}

		public SequoiadbPersistentEntityDummyBuilder<T> and(SequoiadbPersistentProperty property) {
			this.instance.properties.add(property);
			return this;
		}

		public SequoiadbPersistentEntityDummyBuilder<T> withCollection(String collection) {
			this.instance.collection = collection;
			return this;
		}

		public SequoiadbPersistentEntityDummyBuilder<T> and(Annotation annotation) {
			this.instance.annotations.put(annotation.annotationType(), annotation);
			return this;
		}

		public SequoiadbPersistentEntityTestDummy<T> build() {
			return this.instance;
		}

	}

	@Override
	public String getLanguage() {
		return null;
	}

	@Override
	public SequoiadbPersistentProperty getTextScoreProperty() {
		return null;
	}

	@Override
	public boolean hasTextScoreProperty() {
		return false;
	}
}
