/*
 * Copyright 2013 the original author or authors.
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
package org.springframework.data.sequoiadb.core.spel;

import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.springframework.util.Assert;




/**
 * The context for an {@link ExpressionNode} transformation.
 * 


 */
public class ExpressionTransformationContextSupport<T extends ExpressionNode> {

	private final T currentNode;
	private final ExpressionNode parentNode;
	private final BSONObject previousOperationObject;

	/**
	 * Creates a new {@link ExpressionTransformationContextSupport} for the given {@link ExpressionNode}s and an optional
	 * previous operation.
	 * 
	 * @param currentNode must not be {@literal null}.
	 * @param parentNode
	 * @param previousOperationObject
	 */
	public ExpressionTransformationContextSupport(T currentNode, ExpressionNode parentNode,
			BSONObject previousOperationObject) {

		Assert.notNull(currentNode, "currentNode must not be null!");

		this.currentNode = currentNode;
		this.parentNode = parentNode;
		this.previousOperationObject = previousOperationObject;
	}

	/**
	 * Returns the current {@link ExpressionNode}.
	 * 
	 * @return
	 */
	public T getCurrentNode() {
		return currentNode;
	}

	/**
	 * Returns the parent {@link ExpressionNode} or {@literal null} if none available.
	 * 
	 * @return
	 */
	public ExpressionNode getParentNode() {
		return parentNode;
	}

	/**
	 * Returns the previously accumulated operaton object or {@literal null} if none available. Rather than manually
	 * adding stuff to the object prefer using {@link #addToPreviousOrReturn(Object)} to transparently do if one is
	 * present.
	 * 
	 * @see #hasPreviousOperation()
	 * @see #addToPreviousOrReturn(Object)
	 * @return
	 */
	public BSONObject getPreviousOperationObject() {
		return previousOperationObject;
	}

	/**
	 * Returns whether a previous operation is present.
	 * 
	 * @return
	 */
	public boolean hasPreviousOperation() {
		return getPreviousOperationObject() != null;
	}

	/**
	 * Returns whether the parent node is of the same operation as the current node.
	 * 
	 * @return
	 */
	public boolean parentIsSameOperation() {
		return parentNode == null ? false : currentNode.isOfSameTypeAs(parentNode);
	}

	/**
	 * Adds the given value to the previous operation and returns it.
	 * 
	 * @param value
	 * @return
	 */
	public BSONObject addToPreviousOperation(Object value) {
		extractArgumentListFrom(previousOperationObject).add(value);
		return previousOperationObject;
	}

	/**
	 * Adds the given value to the previous operation if one is present or returns the value to add as is.
	 * 
	 * @param value
	 * @return
	 */
	public Object addToPreviousOrReturn(Object value) {
		return hasPreviousOperation() ? addToPreviousOperation(value) : value;
	}

	private BasicBSONList extractArgumentListFrom(BSONObject context) {
		return (BasicBSONList) context.get(context.keySet().iterator().next());
	}
}
