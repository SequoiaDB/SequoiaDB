/*
 * Copyright 2022 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sequoiadb.flink.table.pushdown;

import com.sequoiadb.flink.common.exception.SDBException;

import org.apache.flink.calcite.shaded.com.google.common.collect.ImmutableMap;
import org.apache.flink.table.expressions.CallExpression;
import org.apache.flink.table.expressions.Expression;
import org.apache.flink.table.expressions.FieldReferenceExpression;
import org.apache.flink.table.expressions.ValueLiteralExpression;
import org.apache.flink.table.expressions.ResolvedExpression;
import org.apache.flink.table.functions.BuiltInFunctionDefinitions;
import org.apache.flink.table.functions.FunctionDefinition;
import org.apache.flink.table.types.DataType;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDate;
import org.bson.types.BSONTimestamp;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.Serializable;
import java.sql.Timestamp;
import java.time.Instant;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;
import java.util.function.BiFunction;
import java.util.function.Function;


/**
 * get a collection of expressions,process each expression,and finally merge to get
 * a bson-format expression
 */
public class FilterPushDownSupport {

    private static final Logger LOG = LoggerFactory.getLogger(FilterPushDownSupport.class);

    //map operator----processing function
    private static final ImmutableMap<FunctionDefinition, Function<CallExpression, BSONObject>>
            FILTERS =
            new ImmutableMap.Builder<FunctionDefinition, Function<CallExpression, BSONObject>>()
                    // logical func
                    .put(BuiltInFunctionDefinitions.OR, FilterPushDownSupport::convertOr)
                    .put(BuiltInFunctionDefinitions.AND, FilterPushDownSupport::convertAnd)
                    // comparison func
                    .put(BuiltInFunctionDefinitions.IS_NULL, FilterPushDownSupport::convertIsNull)
                    .put(BuiltInFunctionDefinitions.IS_NOT_NULL, FilterPushDownSupport::convertIsNotNull)
                    .put(BuiltInFunctionDefinitions.NOT, FilterPushDownSupport::convertNot)
                    .put(BuiltInFunctionDefinitions.IS_NOT_TRUE, FilterPushDownSupport::convertNotTrue)
                    .put(BuiltInFunctionDefinitions.IS_NOT_FALSE, FilterPushDownSupport::convertNotFalse)
                    .put(
                            BuiltInFunctionDefinitions.GREATER_THAN,
                            call -> convertBidirectionally(
                                    call,
                                    FilterPushDownSupport::convertGreaterThan,
                                    FilterPushDownSupport::convertLessThan))
                    .put(
                            BuiltInFunctionDefinitions.GREATER_THAN_OR_EQUAL,
                            call -> convertBidirectionally(
                                    call,
                                    FilterPushDownSupport::convertGreaterThanOrEqual,
                                    FilterPushDownSupport::convertLessThanOrEqual))
                    .put(
                            BuiltInFunctionDefinitions.LESS_THAN,
                            call -> convertBidirectionally(
                                    call,
                                    FilterPushDownSupport::convertLessThan,
                                    FilterPushDownSupport::convertGreaterThan))
                    .put(
                            BuiltInFunctionDefinitions.LESS_THAN_OR_EQUAL,
                            call -> convertBidirectionally(
                                    call,
                                    FilterPushDownSupport::convertLessThanOrEqual,
                                    FilterPushDownSupport::convertGreaterThanOrEqual))
                    .put(
                            BuiltInFunctionDefinitions.EQUALS,
                            call -> convertBidirectionally(
                                    call,
                                    FilterPushDownSupport::convertEquals,
                                    FilterPushDownSupport::convertEquals))
                    .put(
                            BuiltInFunctionDefinitions.NOT_EQUALS,
                            call -> convertBidirectionally(
                                    call,
                                    FilterPushDownSupport::convertNotEquals,
                                    FilterPushDownSupport::convertNotEquals))
                    .build();


    /**
     * Loop through a list of expressions to get BSON result
     *
     * @param resolvedExpressionList list of expression
     * @return BSONObject expression
     */
    public static BSONObject toBsonMatcher(List<ResolvedExpression> resolvedExpressionList) {
        List<BSONObject> matchers = new ArrayList<>();

        for (ResolvedExpression resolvedExpression : resolvedExpressionList) {
            //recursively processing
            BSONObject matcher = toBsonMatcher(resolvedExpression);
            matchers.add(matcher);
        }

        return BsonMatcher.andMatcher(matchers);
    }

    /**
     * recursively processing the root expression,obtain the processing method corresponding to
     * the expression,and execute the returned result
     *
     * boolean condition expression,condition: a is true, expression format is (column-name)
     *
     * if expression is a {@code fieldReferenceExpression} and is a boolean condition,return a
     * equal-bson expression
     *
     * @param expression root expression
     * @return BSONObject result
     */
    private static BSONObject toBsonMatcher(Expression expression) {
        if (expression instanceof CallExpression) {
            CallExpression callExpr = (CallExpression) expression;
            Function<CallExpression, BSONObject> exprHandler = FILTERS.get(callExpr.getFunctionDefinition());
            if (exprHandler != null) {
                return exprHandler.apply(callExpr);
            }
        }else if(expression instanceof FieldReferenceExpression){
            FieldReferenceExpression fieldReferenceExpression = (FieldReferenceExpression) expression;
            DataType dataType = fieldReferenceExpression.getOutputDataType();
            Class classz = dataType.getConversionClass();

            if(classz == Boolean.class){
                return BsonMatcher.etMatcher(fieldReferenceExpression.getName(), true);
            }
        }
        LOG.info("Pushing down of expression {} is not supported.", expression);
        return null;
    }


    /**
     * if the expressions are arranged in the order of column names,operator,and values,
     * the forward order method is executed.if the expressions are in the order of values,
     * operator,and column names,the reverse order method needs to be executed,and
     * operator is reversed
     * a > 1 positive
     * 1 > a reverse
     *
     * @param callExpr    expression
     * @param func        positive   column operator value
     * @param reverseFunc reverse value operator column
     * @return BSON result
     */
    private static BSONObject convertBidirectionally(
            CallExpression callExpr,
            BiFunction<String, Serializable, BSONObject> func,
            BiFunction<String, Serializable, BSONObject> reverseFunc) {
        BSONObject result = new BasicBSONObject();

        Expression lExp = getExp(callExpr.getChildren().get(0));
        Expression rExp = getExp(callExpr.getChildren().get(1));

        FieldReferenceExpression fieldReference;
        ValueLiteralExpression valueLiteral;

        // field on left ,value on right
        if (lExp instanceof FieldReferenceExpression && rExp instanceof ValueLiteralExpression) {
            fieldReference = (FieldReferenceExpression) lExp;
            valueLiteral = (ValueLiteralExpression) rExp;

            // get literal and ensure it can be serialized
            result.putAll(func.apply(fieldReference.getName(), getLiteral(valueLiteral)));
            // field on right ,value on left
        } else if (rExp instanceof FieldReferenceExpression && lExp instanceof ValueLiteralExpression) {
            valueLiteral = (ValueLiteralExpression) lExp;
            fieldReference = (FieldReferenceExpression) rExp;

            // get literal and ensure it can be serialized
            result.putAll(reverseFunc.apply(fieldReference.getName(), getLiteral(valueLiteral)));
            //can't parse
        } else {
            LOG.warn("cannot processing expression:{}", callExpr);
            return null;
        }

        return result;
    }

    /**
     * recursive expression until it becomes the simplest expression and then perform merge of and
     *
     * @param callExpr and expression
     * @return BSONObject result
     */
    private static BSONObject convertAnd(CallExpression callExpr) {
        if (callExpr.getChildren().size() != 2) {
            return null;
        }

        Expression lExpr = callExpr.getChildren().get(0);
        Expression rExpr = callExpr.getChildren().get(1);

        //recursion
        BSONObject m1 = toBsonMatcher(lExpr);
        BSONObject m2 = toBsonMatcher(rExpr);

        if (m1 == null && m2 == null) {
            return null;
        }

        List<BSONObject> andList = new ArrayList<>();

        andList.add(m1);
        andList.add(m2);

        return BsonMatcher.andMatcher(andList);
    }


    /**
     * or convert
     *
     * @param callExpr or expression
     * @return BSONObject result
     */
    private static BSONObject convertOr(CallExpression callExpr) {
        if (callExpr.getChildren().size() != 2) {
            return null;
        }

        Expression lExpr = callExpr.getChildren().get(0);
        Expression rExpr = callExpr.getChildren().get(1);

        //recursion
        BSONObject m1 = toBsonMatcher(lExpr);
        BSONObject m2 = toBsonMatcher(rExpr);


        if (m1 == null || m2 == null) {
            return null;
        }
        return BsonMatcher.orMatcher(m1, m2);
    }

    private static BSONObject convertIsNull(CallExpression callExpr) {
        if (callExpr.getChildren().size() != 1) {
            return null;
        }
        FieldReferenceExpression fieldReferenceExpression = (FieldReferenceExpression) callExpr.getChildren().get(0);
        return BsonMatcher.isNullMatcher(fieldReferenceExpression.getName());
    }

    private static BSONObject convertIsNotNull(CallExpression callExpr) {
        if (callExpr.getChildren().size() != 1) {
            return null;
        }
        FieldReferenceExpression fieldReferenceExpression = (FieldReferenceExpression) callExpr.getChildren().get(0);
        return BsonMatcher.isNotNullMatcher(fieldReferenceExpression.getName());
    }

    /**
     * processing boolean expression,return a equal-bson expression
     *
     * condition: a is false, expression format is NOT(column-name)
     *
     * @param callExpr expression
     * @return bson expression
     */
    private static BSONObject convertNot(CallExpression callExpr) {
        if (callExpr.getChildren().size() != 1) {
            return null;
        }
        FieldReferenceExpression fieldReferenceExpression = (FieldReferenceExpression) callExpr.getChildren().get(0);
        return BsonMatcher.etMatcher(fieldReferenceExpression.getName(), false);
    }

    /**
     * processing boolean expression,return a notEquals-bson expression
     *
     * @param callExpr expression
     * @return bson Expression
     */
    private static BSONObject convertNotTrue(CallExpression callExpr) {
        if (callExpr.getChildren().size() != 1) {
            return null;
        }
        FieldReferenceExpression fieldReferenceExpression = (FieldReferenceExpression) callExpr.getChildren().get(0);
        return BsonMatcher.neMatcher(fieldReferenceExpression.getName(), true);
    }

    /**
     * processing boolean expression,return a notEquals-bson expression
     *
     * @param callExpr expression
     * @return bson Expression
     */
    private static BSONObject convertNotFalse(CallExpression callExpr) {
        if (callExpr.getChildren().size() != 1) {
            return null;
        }
        FieldReferenceExpression fieldReferenceExpression = (FieldReferenceExpression) callExpr.getChildren().get(0);
        return BsonMatcher.neMatcher(fieldReferenceExpression.getName(), false);
    }

    // ============================================
    // generate bson matcher for comparison func
    // ============================================

    private static BSONObject convertLessThan(
            String fieldName, Serializable literal) {
        return BsonMatcher.ltMatcher(fieldName, literal);
    }

    private static BSONObject convertLessThanOrEqual(
            String fieldName, Serializable literal) {
        return BsonMatcher.lteMatcher(fieldName, literal);
    }

    private static BSONObject convertGreaterThan(
            String fieldName, Serializable literal) {
        return BsonMatcher.gtMatcher(fieldName, literal);
    }

    private static BSONObject convertGreaterThanOrEqual(
            String fieldName, Serializable literal) {
        return BsonMatcher.gteMatcher(fieldName, literal);
    }

    private static BSONObject convertEquals(
            String fieldName, Serializable literal) {
        return BsonMatcher.etMatcher(fieldName, literal);
    }

    private static BSONObject convertNotEquals(
            String fieldName, Serializable literal) {
        return BsonMatcher.neMatcher(fieldName, literal);
    }

    /**
     * cast expression,get field or value expression
     *
     * @param callExpr nesting expression
     * @return field or value expression
     */
    private static Expression getExp(Expression callExpr) {
        if (!(callExpr instanceof FieldReferenceExpression) && !(callExpr instanceof ValueLiteralExpression)) {
            Expression exp1 = callExpr.getChildren().get(0);
            Expression exp2 = callExpr.getChildren().get(1);

            if (exp1 instanceof FieldReferenceExpression || exp1 instanceof ValueLiteralExpression) {
                return exp1;
            } else if (exp2 instanceof FieldReferenceExpression || exp2 instanceof ValueLiteralExpression) {
                return exp2;
            }
        } else {
            return callExpr;
        }
        return null;
    }

    /**
     * get value of valueExpression
     *
     * @param expression expression
     * @return serializable value
     */
    private static Serializable getLiteral(Expression expression) {
        ValueLiteralExpression valueLiteral = (ValueLiteralExpression) expression;
        Class<?> classz = valueLiteral.getOutputDataType().getConversionClass();
        Object value = valueLiteral.getValueAs(classz).orElseThrow(() ->
                new SDBException(String.format("cannot get value %s", valueLiteral.toString())));

        Object resultValue = value;

        if (value instanceof LocalDate) {
            resultValue = BSONDate.valueOf((LocalDate) value);
        } else if (value instanceof LocalDateTime) {
            Timestamp timestamp = Timestamp.valueOf((LocalDateTime) value);
            resultValue = new BSONTimestamp(timestamp);
        } else if (value instanceof Instant) {
            Instant instant = (Instant) value;
            resultValue = new BSONTimestamp((int)instant.getEpochSecond(), instant.getNano() / 1000);
        }

        return (Serializable) resultValue;
    }
}
