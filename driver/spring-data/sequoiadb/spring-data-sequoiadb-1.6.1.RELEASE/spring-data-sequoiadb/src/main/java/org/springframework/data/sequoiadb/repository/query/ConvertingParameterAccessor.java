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

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

import org.springframework.data.domain.Pageable;
import org.springframework.data.domain.Sort;
import org.springframework.data.geo.Distance;
import org.springframework.data.geo.Point;
import org.springframework.data.sequoiadb.core.convert.SequoiadbWriter;
import org.springframework.data.sequoiadb.core.mapping.SequoiadbPersistentProperty;
import org.springframework.data.sequoiadb.core.query.TextCriteria;
import org.springframework.data.repository.query.ParameterAccessor;
import org.springframework.data.util.TypeInformation;
import org.springframework.util.Assert;
import org.springframework.util.CollectionUtils;

import org.springframework.data.sequoiadb.assist.DBRef;

/**
 * Custom {@link ParameterAccessor} that uses a {@link SequoiadbWriter} to serialize parameters into Sdb format.
 * 


 */
public class ConvertingParameterAccessor implements SequoiadbParameterAccessor {

	private final SequoiadbWriter<?> writer;
	private final SequoiadbParameterAccessor delegate;

	/**
	 * Creates a new {@link ConvertingParameterAccessor} with the given {@link SequoiadbWriter} and delegate.
	 * 
	 * @param writer must not be {@literal null}.
	 * @param delegate must not be {@literal null}.
	 */
	public ConvertingParameterAccessor(SequoiadbWriter<?> writer, SequoiadbParameterAccessor delegate) {

		Assert.notNull(writer);
		Assert.notNull(delegate);

		this.writer = writer;
		this.delegate = delegate;
	}

	/*
	  * (non-Javadoc)
	  *
	  * @see java.lang.Iterable#iterator()
	  */
	public PotentiallyConvertingIterator iterator() {
		return new ConvertingIterator(delegate.iterator());
	}

	/*
	  * (non-Javadoc)
	  *
	  * @see org.springframework.data.repository.query.ParameterAccessor#getPageable()
	  */
	public Pageable getPageable() {
		return delegate.getPageable();
	}

	/*
	  * (non-Javadoc)
	  *
	  * @see org.springframework.data.repository.query.ParameterAccessor#getSort()
	  */
	public Sort getSort() {
		return delegate.getSort();
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.query.ParameterAccessor#getBindableValue(int)
	 */
	public Object getBindableValue(int index) {
		return getConvertedValue(delegate.getBindableValue(index), null);
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.repository.SequoiadbParameterAccessor#getMaxDistance()
	 */
	public Distance getMaxDistance() {
		return delegate.getMaxDistance();
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.repository.SequoiadbParameterAccessor#getGeoNearLocation()
	 */
	public Point getGeoNearLocation() {
		return delegate.getGeoNearLocation();
	}

	/*
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.repository.query.SequoiadbParameterAccessor#getFullText()
	 */
	public TextCriteria getFullText() {
		return delegate.getFullText();
	}

	/**
	 * Converts the given value with the underlying {@link SequoiadbWriter}.
	 * 
	 * @param value can be {@literal null}.
	 * @param typeInformation can be {@literal null}.
	 * @return
	 */
	private Object getConvertedValue(Object value, TypeInformation<?> typeInformation) {
		return writer.convertToSequoiadbType(value, typeInformation == null ? null : typeInformation.getActualType());
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.repository.query.ParameterAccessor#hasBindableNullValue()
	 */
	public boolean hasBindableNullValue() {
		return delegate.hasBindableNullValue();
	}

	/**
	 * Custom {@link Iterator} to convert items before returning them.
	 * 

	 */
	private class ConvertingIterator implements PotentiallyConvertingIterator {

		private final Iterator<Object> delegate;

		/**
		 * Creates a new {@link ConvertingIterator} for the given delegate.
		 * 
		 * @param delegate
		 */
		public ConvertingIterator(Iterator<Object> delegate) {
			this.delegate = delegate;
		}

		/*
		 * (non-Javadoc)
		 * @see java.util.Iterator#hasNext()
		 */
		public boolean hasNext() {
			return delegate.hasNext();
		}

		/*
		 * (non-Javadoc)
		 * @see java.util.Iterator#next()
		 */
		public Object next() {
			return delegate.next();
		}

		/* 
		 * (non-Javadoc)
		 * @see org.springframework.data.sequoiadb.repository.ConvertingParameterAccessor.PotentiallConvertingIterator#nextConverted()
		 */
		public Object nextConverted(SequoiadbPersistentProperty property) {

			Object next = next();

			if (next == null) {
				return null;
			}

			if (property.isAssociation()) {
				if (next.getClass().isArray() || next instanceof Iterable) {

					List<DBRef> dbRefs = new ArrayList<DBRef>();
					for (Object element : asCollection(next)) {
						dbRefs.add(writer.toDBRef(element, property));
					}

					return dbRefs;
				} else {
					return writer.toDBRef(next, property);
				}
			}

			return getConvertedValue(next, property.getTypeInformation());
		}

		/*
		 * (non-Javadoc)
		 * @see java.util.Iterator#remove()
		 */
		public void remove() {
			delegate.remove();
		}
	}

	/**
	 * Returns the given object as {@link Collection}. Will do a copy of it if it implements {@link Iterable} or is an
	 * array. Will return an empty {@link Collection} in case {@literal null} is given. Will wrap all other types into a
	 * single-element collction
	 * 
	 * @param source
	 * @return
	 */
	private static Collection<?> asCollection(Object source) {

		if (source instanceof Iterable) {

			List<Object> result = new ArrayList<Object>();
			for (Object element : (Iterable<?>) source) {
				result.add(element);
			}

			return result;
		}

		if (source == null) {
			return Collections.emptySet();
		}

		return source.getClass().isArray() ? CollectionUtils.arrayToList(source) : Collections.singleton(source);
	}

	/**
	 * Custom {@link Iterator} that adds a method to access elements in a converted manner.
	 * 

	 */
	public interface PotentiallyConvertingIterator extends Iterator<Object> {

		/**
		 * Returns the next element which has already been converted.
		 * 
		 * @return
		 */
		Object nextConverted(SequoiadbPersistentProperty property);
	}
}
