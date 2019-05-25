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
package org.springframework.data.sequoiadb.core.convert;

import static org.springframework.util.ReflectionUtils.*;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.lang.reflect.Method;

import org.aopalliance.intercept.MethodInterceptor;
import org.aopalliance.intercept.MethodInvocation;
import org.springframework.aop.framework.ProxyFactory;
import org.springframework.cglib.proxy.Callback;
import org.springframework.cglib.proxy.Enhancer;
import org.springframework.cglib.proxy.Factory;
import org.springframework.cglib.proxy.MethodProxy;
import org.springframework.dao.DataAccessException;
import org.springframework.dao.support.PersistenceExceptionTranslator;
import org.springframework.data.sequoiadb.LazyLoadingException;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.objenesis.ObjenesisStd;
import org.springframework.util.Assert;
import org.springframework.util.ReflectionUtils;
import org.springframework.util.StringUtils;

import org.springframework.data.sequoiadb.assist.DB;
import org.springframework.data.sequoiadb.assist.DBRef;

/**
 * A {@link DbRefResolver} that resolves {@link org.springframework.data.sequoiadb.core.mapping.DBRef}s by delegating to a
 * {@link DbRefResolverCallback} than is able to generate lazy loading proxies.
 *
 * @since 1.4
 */
public class DefaultDbRefResolver implements DbRefResolver {

	private final SequoiadbFactory sequoiadbFactory;
	private final PersistenceExceptionTranslator exceptionTranslator;
	private final ObjenesisStd objenesis;

	/**
	 * Creates a new {@link DefaultDbRefResolver} with the given {@link SequoiadbFactory}.
	 * 
	 * @param sequoiadbFactory must not be {@literal null}.
	 */
	public DefaultDbRefResolver(SequoiadbFactory sequoiadbFactory) {

		Assert.notNull(sequoiadbFactory, "SequoiadbFactory translator must not be null!");

		this.sequoiadbFactory = sequoiadbFactory;
		this.exceptionTranslator = sequoiadbFactory.getExceptionTranslator();
		this.objenesis = new ObjenesisStd(true);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.convert.DbRefResolver#resolveDbRef(org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty, org.springframework.data.sequoiadb.core.convert.DbRefResolverCallback)
	 */
	@Override
	public Object resolveDbRef(SequoiadbPersistentProperty property, DBRef dbref, DbRefResolverCallback callback,
                               DbRefProxyHandler handler) {

		Assert.notNull(property, "Property must not be null!");
		Assert.notNull(callback, "Callback must not be null!");

		if (isLazyDbRef(property)) {
			return createLazyLoadingProxy(property, dbref, callback, handler);
		}

		return callback.resolve(property);
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.core.convert.DbRefResolver#created(org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty, org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentEntity, java.lang.Object)
	 */
	@Override
	public DBRef createDbRef(org.springframework.data.sequoiadb.core.mapping.DBRef annotation,
                             SequoiadbPersistentEntity<?> entity, Object id) {

		DB db = sequoiadbFactory.getDb();
		db = annotation != null && StringUtils.hasText(annotation.db()) ? sequoiadbFactory.getDb(annotation.db()) : db;

		return new DBRef(db, entity.getCollection(), id);
	}

	/**
	 * Creates a proxy for the given {@link SequoiadbPersistentProperty} using the given {@link DbRefResolverCallback} to
	 * eventually resolve the value of the property.
	 * 
	 * @param property must not be {@literal null}.
	 * @param dbref can be {@literal null}.
	 * @param callback must not be {@literal null}.
	 * @return
	 */
	private Object createLazyLoadingProxy(SequoiadbPersistentProperty property, DBRef dbref, DbRefResolverCallback callback,
                                          DbRefProxyHandler handler) {

		Class<?> propertyType = property.getType();
		LazyLoadingInterceptor interceptor = new LazyLoadingInterceptor(property, dbref, exceptionTranslator, callback);

		if (!propertyType.isInterface()) {

			Factory factory = (Factory) objenesis.newInstance(getEnhancedTypeFor(propertyType));
			factory.setCallbacks(new Callback[] { interceptor });

			return handler.populateId(property, dbref, factory);
		}

		ProxyFactory proxyFactory = new ProxyFactory();

		for (Class<?> type : propertyType.getInterfaces()) {
			proxyFactory.addInterface(type);
		}

		proxyFactory.addInterface(LazyLoadingProxy.class);
		proxyFactory.addInterface(propertyType);
		proxyFactory.addAdvice(interceptor);

		return handler.populateId(property, dbref, proxyFactory.getProxy());
	}

	/**
	 * Returns the CGLib enhanced type for the given source type.
	 * 
	 * @param type
	 * @return
	 */
	private Class<?> getEnhancedTypeFor(Class<?> type) {

		Enhancer enhancer = new Enhancer();
		enhancer.setSuperclass(type);
		enhancer.setCallbackType(org.springframework.cglib.proxy.MethodInterceptor.class);
		enhancer.setInterfaces(new Class[] { LazyLoadingProxy.class });

		return enhancer.createClass();
	}

	/**
	 * Returns whether the property shall be resolved lazily.
	 * 
	 * @param property must not be {@literal null}.
	 * @return
	 */
	private boolean isLazyDbRef(SequoiadbPersistentProperty property) {
		return property.getDBRef() != null && property.getDBRef().lazy();
	}

	/**
	 * A {@link MethodInterceptor} that is used within a lazy loading proxy. The property resolving is delegated to a
	 * {@link DbRefResolverCallback}. The resolving process is triggered by a method invocation on the proxy and is
	 * guaranteed to be performed only once.
	 * 



	 */
	static class LazyLoadingInterceptor implements MethodInterceptor, org.springframework.cglib.proxy.MethodInterceptor,
			Serializable {

		private static final Method INITIALIZE_METHOD, TO_DBREF_METHOD, FINALIZE_METHOD;

		private final DbRefResolverCallback callback;
		private final SequoiadbPersistentProperty property;
		private final PersistenceExceptionTranslator exceptionTranslator;

		private volatile boolean resolved;
		private Object result;
		private DBRef dbref;

		static {
			try {
				INITIALIZE_METHOD = LazyLoadingProxy.class.getMethod("getTarget");
				TO_DBREF_METHOD = LazyLoadingProxy.class.getMethod("toDBRef");
				FINALIZE_METHOD = Object.class.getDeclaredMethod("finalize");
			} catch (Exception e) {
				throw new RuntimeException(e);
			}
		}

		/**
		 * Creates a new {@link LazyLoadingInterceptor} for the given {@link SequoiadbPersistentProperty},
		 * {@link PersistenceExceptionTranslator} and {@link DbRefResolverCallback}.
		 * 
		 * @param property must not be {@literal null}.
		 * @param dbref can be {@literal null}.
		 * @param callback must not be {@literal null}.
		 */
		public LazyLoadingInterceptor(SequoiadbPersistentProperty property, DBRef dbref,
                                      PersistenceExceptionTranslator exceptionTranslator, DbRefResolverCallback callback) {

			Assert.notNull(property, "Property must not be null!");
			Assert.notNull(exceptionTranslator, "Exception translator must not be null!");
			Assert.notNull(callback, "Callback must not be null!");

			this.dbref = dbref;
			this.callback = callback;
			this.exceptionTranslator = exceptionTranslator;
			this.property = property;
		}

		/*
		 * (non-Javadoc)
		 * @see org.aopalliance.intercept.MethodInterceptor#invoke(org.aopalliance.intercept.MethodInvocation)
		 */
		@Override
		public Object invoke(MethodInvocation invocation) throws Throwable {
			return intercept(invocation.getThis(), invocation.getMethod(), invocation.getArguments(), null);
		}

		/* 
		 * (non-Javadoc)
		 * @see org.springframework.cglib.proxy.MethodInterceptor#intercept(java.lang.Object, java.lang.reflect.Method, java.lang.Object[], org.springframework.cglib.proxy.MethodProxy)
		 */
		@Override
		public Object intercept(Object obj, Method method, Object[] args, MethodProxy proxy) throws Throwable {

			if (INITIALIZE_METHOD.equals(method)) {
				return ensureResolved();
			}

			if (TO_DBREF_METHOD.equals(method)) {
				return this.dbref;
			}

			if (isObjectMethod(method) && Object.class.equals(method.getDeclaringClass())) {

				if (ReflectionUtils.isToStringMethod(method)) {
					return proxyToString(proxy);
				}

				if (ReflectionUtils.isEqualsMethod(method)) {
					return proxyEquals(proxy, args[0]);
				}

				if (ReflectionUtils.isHashCodeMethod(method)) {
					return proxyHashCode(proxy);
				}

				if (FINALIZE_METHOD.equals(method)) {
					return null;
				}
			}

			Object target = ensureResolved();

			if (target == null) {
				return null;
			}

			return method.invoke(target, args);
		}

		/**
		 * Returns a to string representation for the given {@code proxy}.
		 * 
		 * @param proxy
		 * @return
		 */
		private String proxyToString(Object proxy) {

			StringBuilder description = new StringBuilder();
			if (dbref != null) {
				description.append(dbref.getRef());
				description.append(":");
				description.append(dbref.getId());
			} else {
				description.append(System.identityHashCode(proxy));
			}
			description.append("$").append(LazyLoadingProxy.class.getSimpleName());

			return description.toString();
		}

		/**
		 * Returns the hashcode for the given {@code proxy}.
		 * 
		 * @param proxy
		 * @return
		 */
		private int proxyHashCode(Object proxy) {
			return proxyToString(proxy).hashCode();
		}

		/**
		 * Performs an equality check for the given {@code proxy}.
		 * 
		 * @param proxy
		 * @param that
		 * @return
		 */
		private boolean proxyEquals(Object proxy, Object that) {

			if (!(that instanceof LazyLoadingProxy)) {
				return false;
			}

			if (that == proxy) {
				return true;
			}

			return proxyToString(proxy).equals(that.toString());
		}

		/**
		 * Will trigger the resolution if the proxy is not resolved already or return a previously resolved result.
		 * 
		 * @return
		 */
		private Object ensureResolved() {

			if (!resolved) {
				this.result = resolve();
				this.resolved = true;
			}

			return this.result;
		}

		/**
		 * Callback method for serialization.
		 * 
		 * @param out
		 * @throws IOException
		 */
		private void writeObject(ObjectOutputStream out) throws IOException {

			ensureResolved();
			out.writeObject(this.result);
		}

		/**
		 * Callback method for deserialization.
		 * 
		 * @param in
		 * @throws IOException
		 */
		private void readObject(ObjectInputStream in) throws IOException {

			try {
				this.resolved = true;
				this.result = in.readObject();
			} catch (ClassNotFoundException e) {
				throw new LazyLoadingException("Could not deserialize result", e);
			}
		}

		/**
		 * Resolves the proxy into its backing object.
		 * 
		 * @return
		 */
		private synchronized Object resolve() {

			if (!resolved) {

				try {

					return callback.resolve(property);

				} catch (RuntimeException ex) {

					DataAccessException translatedException = this.exceptionTranslator.translateExceptionIfPossible(ex);
					throw new LazyLoadingException("Unable to lazily resolve DBRef!", translatedException);
				}
			}

			return result;
		}
	}
}
