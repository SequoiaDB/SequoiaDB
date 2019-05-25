/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*
 * JavaScript Debugging support - Value and Property support
 */

#include "jsd.h"
#include "jsapi.h"
#include "jsfriendapi.h"

#ifdef DEBUG
void JSD_ASSERT_VALID_VALUE(JSDValue* jsdval)
{
    JS_ASSERT(jsdval);
    JS_ASSERT(jsdval->nref > 0);
    if(!JS_CLIST_IS_EMPTY(&jsdval->props))
    {
        JS_ASSERT(CHECK_BIT_FLAG(jsdval->flags, GOT_PROPS));
        JS_ASSERT(JSVAL_IS_OBJECT(jsdval->val));
    }

    if(jsdval->proto)
    {
        JS_ASSERT(CHECK_BIT_FLAG(jsdval->flags, GOT_PROTO));
        JS_ASSERT(jsdval->proto->nref > 0);
    }
    if(jsdval->parent)
    {
        JS_ASSERT(CHECK_BIT_FLAG(jsdval->flags, GOT_PARENT));
        JS_ASSERT(jsdval->parent->nref > 0);
    }
    if(jsdval->ctor)
    {
        JS_ASSERT(CHECK_BIT_FLAG(jsdval->flags, GOT_CTOR));
        JS_ASSERT(jsdval->ctor->nref > 0);
    }
}

void JSD_ASSERT_VALID_PROPERTY(JSDProperty* jsdprop)
{
    JS_ASSERT(jsdprop);
    JS_ASSERT(jsdprop->name);
    JS_ASSERT(jsdprop->name->nref > 0);
    JS_ASSERT(jsdprop->val);
    JS_ASSERT(jsdprop->val->nref > 0);
    if(jsdprop->alias)
        JS_ASSERT(jsdprop->alias->nref > 0);
}
#endif


JSBool
jsd_IsValueObject(JSDContext* jsdc, JSDValue* jsdval)
{
    return JSVAL_IS_OBJECT(jsdval->val);
}

JSBool
jsd_IsValueNumber(JSDContext* jsdc, JSDValue* jsdval)
{
    return JSVAL_IS_NUMBER(jsdval->val);
}

JSBool
jsd_IsValueInt(JSDContext* jsdc, JSDValue* jsdval)
{
    return JSVAL_IS_INT(jsdval->val);
}

JSBool
jsd_IsValueDouble(JSDContext* jsdc, JSDValue* jsdval)
{
    return JSVAL_IS_DOUBLE(jsdval->val);
}

JSBool
jsd_IsValueString(JSDContext* jsdc, JSDValue* jsdval)
{
    return JSVAL_IS_STRING(jsdval->val);
}

JSBool
jsd_IsValueBoolean(JSDContext* jsdc, JSDValue* jsdval)
{
    return JSVAL_IS_BOOLEAN(jsdval->val);
}

JSBool
jsd_IsValueNull(JSDContext* jsdc, JSDValue* jsdval)
{
    return JSVAL_IS_NULL(jsdval->val);
}

JSBool
jsd_IsValueVoid(JSDContext* jsdc, JSDValue* jsdval)
{
    return JSVAL_IS_VOID(jsdval->val);
}

JSBool
jsd_IsValuePrimitive(JSDContext* jsdc, JSDValue* jsdval)
{
    return JSVAL_IS_PRIMITIVE(jsdval->val);
}

JSBool
jsd_IsValueFunction(JSDContext* jsdc, JSDValue* jsdval)
{
    return !JSVAL_IS_PRIMITIVE(jsdval->val) &&
           JS_ObjectIsCallable(jsdc->dumbContext, JSVAL_TO_OBJECT(jsdval->val));
}

JSBool
jsd_IsValueNative(JSDContext* jsdc, JSDValue* jsdval)
{
    JSContext* cx = jsdc->dumbContext;
    JSFunction* fun;
    JSExceptionState* exceptionState;
    JSCrossCompartmentCall *call = NULL;

    if(jsd_IsValueFunction(jsdc, jsdval))
    {
        JSBool ok = JS_FALSE;
        JS_BeginRequest(cx);
        call = JS_EnterCrossCompartmentCall(jsdc->dumbContext, JSVAL_TO_OBJECT(jsdval->val));
        if(!call) {
            JS_EndRequest(cx);

            return JS_FALSE;
        }

        exceptionState = JS_SaveExceptionState(cx);
        fun = JSD_GetValueFunction(jsdc, jsdval);
        JS_RestoreExceptionState(cx, exceptionState);
        if(fun)
            ok = JS_GetFunctionScript(cx, fun) ? JS_FALSE : JS_TRUE;
        JS_LeaveCrossCompartmentCall(call);
        JS_EndRequest(cx);
        JS_ASSERT(fun);
        return ok;
    }
    return !JSVAL_IS_PRIMITIVE(jsdval->val);
}

/***************************************************************************/

JSBool
jsd_GetValueBoolean(JSDContext* jsdc, JSDValue* jsdval)
{
    jsval val = jsdval->val;
    if(!JSVAL_IS_BOOLEAN(val))
        return JS_FALSE;
    return JSVAL_TO_BOOLEAN(val);
}

int32
jsd_GetValueInt(JSDContext* jsdc, JSDValue* jsdval)
{
    jsval val = jsdval->val;
    if(!JSVAL_IS_INT(val))
        return 0;
    return JSVAL_TO_INT(val);
}

jsdouble
jsd_GetValueDouble(JSDContext* jsdc, JSDValue* jsdval)
{
    if(!JSVAL_IS_DOUBLE(jsdval->val))
        return 0;
    return JSVAL_TO_DOUBLE(jsdval->val);
}

JSString*
jsd_GetValueString(JSDContext* jsdc, JSDValue* jsdval)
{
    JSContext* cx = jsdc->dumbContext;
    JSExceptionState* exceptionState;
    JSCrossCompartmentCall *call = NULL;
    jsval stringval;
    JSString *string;
    JSBool needWrap;
    JSObject *scopeObj;

    if(jsdval->string)
        return jsdval->string;

    /* Reuse the string without copying or re-rooting it */
    if(JSVAL_IS_STRING(jsdval->val)) {
        jsdval->string = JSVAL_TO_STRING(jsdval->val);
        return jsdval->string;
    }

    JS_BeginRequest(cx);

    /* Objects call JS_ValueToString in their own compartment. */
    scopeObj = JSVAL_IS_OBJECT(jsdval->val) ? JSVAL_TO_OBJECT(jsdval->val) : jsdc->glob;
    call = JS_EnterCrossCompartmentCall(cx, scopeObj);
    if(!call) {
        JS_EndRequest(cx);
        return NULL;
    }
    exceptionState = JS_SaveExceptionState(cx);

    string = JS_ValueToString(cx, jsdval->val);

    JS_RestoreExceptionState(cx, exceptionState);
    JS_LeaveCrossCompartmentCall(call);

    if(string) {
        stringval = STRING_TO_JSVAL(string);
        call = JS_EnterCrossCompartmentCall(cx, jsdc->glob);
    }
    if(!string || !call || !JS_WrapValue(cx, &stringval)) {
        if(call)
            JS_LeaveCrossCompartmentCall(call);
        JS_EndRequest(cx);
        return NULL;
    }

    jsdval->string = JSVAL_TO_STRING(stringval);
    if(!JS_AddNamedStringRoot(cx, &jsdval->string, "ValueString"))
        jsdval->string = NULL;

    JS_LeaveCrossCompartmentCall(call);
    JS_EndRequest(cx);

    return jsdval->string;
}

JSString*
jsd_GetValueFunctionId(JSDContext* jsdc, JSDValue* jsdval)
{
    JSContext* cx = jsdc->dumbContext;
    JSFunction* fun;
    JSExceptionState* exceptionState;
    JSCrossCompartmentCall *call = NULL;

    if(!jsdval->funName && jsd_IsValueFunction(jsdc, jsdval))
    {
        JS_BeginRequest(cx);

        call = JS_EnterCrossCompartmentCall(jsdc->dumbContext, JSVAL_TO_OBJECT(jsdval->val));
        if(!call) {
            JS_EndRequest(cx);

            return NULL;
        }

        exceptionState = JS_SaveExceptionState(cx);
        fun = JSD_GetValueFunction(jsdc, jsdval);
        JS_RestoreExceptionState(cx, exceptionState);
        JS_LeaveCrossCompartmentCall(call);
        JS_EndRequest(cx);
        if(!fun)
            return NULL;
        jsdval->funName = JS_GetFunctionId(fun);

        /* For compatibility we return "anonymous", not an empty string here. */
        if (!jsdval->funName)
            jsdval->funName = JS_GetAnonymousString(jsdc->jsrt);
    }
    return jsdval->funName;
}

/***************************************************************************/

/*
 * Create a new JSD value referring to a jsval. Copy string values into the
 * JSD compartment. Leave all other GCTHINGs in their native compartments
 * and access them through cross-compartment calls.
 */
JSDValue*
jsd_NewValue(JSDContext* jsdc, jsval val)
{
    JSDValue* jsdval;
    JSCrossCompartmentCall *call = NULL;

    if(!(jsdval = (JSDValue*) calloc(1, sizeof(JSDValue))))
        return NULL;

    if(JSVAL_IS_GCTHING(val))
    {
        JSBool ok;
        JS_BeginRequest(jsdc->dumbContext);

        call = JS_EnterCrossCompartmentCall(jsdc->dumbContext, jsdc->glob);
        if(!call) {
            JS_EndRequest(jsdc->dumbContext);
            free(jsdval);
            return NULL;
        }

        ok = JS_AddNamedValueRoot(jsdc->dumbContext, &jsdval->val, "JSDValue");
        if(ok && JSVAL_IS_STRING(val)) {
            if(!JS_WrapValue(jsdc->dumbContext, &val)) {
                ok = JS_FALSE;
            }
        }

        JS_LeaveCrossCompartmentCall(call);
        JS_EndRequest(jsdc->dumbContext);
        if(!ok)
        {
            free(jsdval);
            return NULL;
        }
    }
    jsdval->val  = val;
    jsdval->nref = 1;
    JS_INIT_CLIST(&jsdval->props);

    return jsdval;
}

void
jsd_DropValue(JSDContext* jsdc, JSDValue* jsdval)
{
    JSCrossCompartmentCall *call = NULL;

    JS_ASSERT(jsdval->nref > 0);
    if(0 == --jsdval->nref)
    {
        jsd_RefreshValue(jsdc, jsdval);
        if(JSVAL_IS_GCTHING(jsdval->val))
        {
            JS_BeginRequest(jsdc->dumbContext);
            call = JS_EnterCrossCompartmentCall(jsdc->dumbContext, jsdc->glob);
            if(!call) {
                JS_EndRequest(jsdc->dumbContext);

                return;
            }

            JS_RemoveValueRoot(jsdc->dumbContext, &jsdval->val);
            JS_LeaveCrossCompartmentCall(call);
            JS_EndRequest(jsdc->dumbContext);
        }
        free(jsdval);
    }
}

jsval
jsd_GetValueWrappedJSVal(JSDContext* jsdc, JSDValue* jsdval)
{
    JSObject* obj;
    JSContext* cx;
    jsval val = jsdval->val;
    if (!JSVAL_IS_PRIMITIVE(val)) {
        cx = JSD_GetDefaultJSContext(jsdc);
        obj = js_ObjectToOuterObject(cx, JSVAL_TO_OBJECT(val));
        if (!obj)
        {
            JS_ClearPendingException(cx);
            val = JSVAL_NULL;
        }
        else
            val = OBJECT_TO_JSVAL(obj);
    }
    
    return val;
}

static JSDProperty* _newProperty(JSDContext* jsdc, JSPropertyDesc* pd,
                                 uintN additionalFlags)
{
    JSDProperty* jsdprop;

    if(!(jsdprop = (JSDProperty*) calloc(1, sizeof(JSDProperty))))
        return NULL;

    JS_INIT_CLIST(&jsdprop->links);
    jsdprop->nref = 1;
    jsdprop->flags = pd->flags | additionalFlags;
    jsdprop->slot = pd->slot;

    if(!(jsdprop->name = jsd_NewValue(jsdc, pd->id)))
        goto new_prop_fail;

    if(!(jsdprop->val = jsd_NewValue(jsdc, pd->value)))
        goto new_prop_fail;

    if((jsdprop->flags & JSDPD_ALIAS) &&
       !(jsdprop->alias = jsd_NewValue(jsdc, pd->alias)))
        goto new_prop_fail;

    return jsdprop;
new_prop_fail:
    jsd_DropProperty(jsdc, jsdprop);
    return NULL;
}

static void _freeProps(JSDContext* jsdc, JSDValue* jsdval)
{
    JSDProperty* jsdprop;

    while(jsdprop = (JSDProperty*)jsdval->props.next,
          jsdprop != (JSDProperty*)&jsdval->props)
    {
        JS_REMOVE_AND_INIT_LINK(&jsdprop->links);
        jsd_DropProperty(jsdc, jsdprop);
    }
    JS_ASSERT(JS_CLIST_IS_EMPTY(&jsdval->props));
    CLEAR_BIT_FLAG(jsdval->flags, GOT_PROPS);
}

static JSBool _buildProps(JSDContext* jsdc, JSDValue* jsdval)
{
    JSContext* cx = jsdc->dumbContext;
    JSObject *obj;
    JSPropertyDescArray pda;
    uintN i;
    JSCrossCompartmentCall *call = NULL;

    JS_ASSERT(JS_CLIST_IS_EMPTY(&jsdval->props));
    JS_ASSERT(!(CHECK_BIT_FLAG(jsdval->flags, GOT_PROPS)));
    JS_ASSERT(JSVAL_IS_OBJECT(jsdval->val));

    if(JSVAL_IS_PRIMITIVE(jsdval->val))
        return JS_FALSE;

    obj = JSVAL_TO_OBJECT(jsdval->val);

    JS_BeginRequest(cx);
    call = JS_EnterCrossCompartmentCall(jsdc->dumbContext, obj);
    if(!call)
    {
        JS_EndRequest(jsdc->dumbContext);
        return JS_FALSE;
    }

    if(!JS_GetPropertyDescArray(cx, obj, &pda))
    {
        JS_EndRequest(cx);
        JS_LeaveCrossCompartmentCall(call);
        return JS_FALSE;
    }

    for(i = 0; i < pda.length; i++)
    {
        JSDProperty* prop = _newProperty(jsdc, &pda.array[i], 0);
        if(!prop)
        {
            _freeProps(jsdc, jsdval);
            break;
        }
        JS_APPEND_LINK(&prop->links, &jsdval->props);
    }
    JS_PutPropertyDescArray(cx, &pda);
    JS_LeaveCrossCompartmentCall(call);
    JS_EndRequest(cx);
    SET_BIT_FLAG(jsdval->flags, GOT_PROPS);
    return !JS_CLIST_IS_EMPTY(&jsdval->props);
}

#undef  DROP_CLEAR_VALUE
#define DROP_CLEAR_VALUE(jsdc, x) if(x){jsd_DropValue(jsdc,x); x = NULL;}

void
jsd_RefreshValue(JSDContext* jsdc, JSDValue* jsdval)
{
    JSContext* cx = jsdc->dumbContext;
    JSCrossCompartmentCall *call = NULL;

    if(jsdval->string)
    {
        /* if the jsval is a string, then we didn't need to root the string */
        if(!JSVAL_IS_STRING(jsdval->val))
        {
            JS_BeginRequest(cx);
            call = JS_EnterCrossCompartmentCall(cx, jsdc->glob);
            if(!call) {
                JS_EndRequest(cx);

                return;
            }

            JS_RemoveStringRoot(cx, &jsdval->string);
            JS_LeaveCrossCompartmentCall(call);
            JS_EndRequest(cx);
        }
        jsdval->string = NULL;
    }

    jsdval->funName = NULL;
    jsdval->className = NULL;
    DROP_CLEAR_VALUE(jsdc, jsdval->proto);
    DROP_CLEAR_VALUE(jsdc, jsdval->parent);
    DROP_CLEAR_VALUE(jsdc, jsdval->ctor);
    _freeProps(jsdc, jsdval);
    jsdval->flags = 0;
}

/***************************************************************************/

uintN
jsd_GetCountOfProperties(JSDContext* jsdc, JSDValue* jsdval)
{
    JSDProperty* jsdprop;
    uintN count = 0;

    if(!(CHECK_BIT_FLAG(jsdval->flags, GOT_PROPS)))
        if(!_buildProps(jsdc, jsdval))
            return 0;

    for(jsdprop = (JSDProperty*)jsdval->props.next;
        jsdprop != (JSDProperty*)&jsdval->props;
        jsdprop = (JSDProperty*)jsdprop->links.next)
    {
        count++;
    }
    return count;
}

JSDProperty*
jsd_IterateProperties(JSDContext* jsdc, JSDValue* jsdval, JSDProperty **iterp)
{
    JSDProperty* jsdprop = *iterp;
    if(!(CHECK_BIT_FLAG(jsdval->flags, GOT_PROPS)))
    {
        JS_ASSERT(!jsdprop);
        if(!_buildProps(jsdc, jsdval))
            return NULL;
    }

    if(!jsdprop)
        jsdprop = (JSDProperty*)jsdval->props.next;
    if(jsdprop == (JSDProperty*)&jsdval->props)
        return NULL;
    *iterp = (JSDProperty*)jsdprop->links.next;

    JS_ASSERT(jsdprop);
    jsdprop->nref++;
    return jsdprop;
}

JSDProperty*
jsd_GetValueProperty(JSDContext* jsdc, JSDValue* jsdval, JSString* name)
{
    JSContext* cx = jsdc->dumbContext;
    JSDProperty* jsdprop;
    JSDProperty* iter = NULL;
    JSObject* obj;
    uintN  attrs = 0;
    JSBool found;
    JSPropertyDesc pd;
    const jschar * nameChars;
    size_t nameLen;
    jsval val, nameval;
    jsid nameid;
    JSCrossCompartmentCall *call = NULL;

    if(!jsd_IsValueObject(jsdc, jsdval))
        return NULL;

    /* If we already have the prop, then return it */
    while(NULL != (jsdprop = jsd_IterateProperties(jsdc, jsdval, &iter)))
    {
        JSString* propName = jsd_GetValueString(jsdc, jsdprop->name);
        if(propName) {
            intN result;
            if (JS_CompareStrings(cx, propName, name, &result) && !result)
                return jsdprop;
        }
        JSD_DropProperty(jsdc, jsdprop);
    }
    /* Not found in property list, look it up explicitly */

    if(!(obj = JSVAL_TO_OBJECT(jsdval->val)))
        return NULL;

    if (!(nameChars = JS_GetStringCharsZAndLength(cx, name, &nameLen)))
        return NULL;

    JS_BeginRequest(cx);
    call = JS_EnterCrossCompartmentCall(cx, obj);
    if(!call) {
        JS_EndRequest(cx);

        return NULL;
    }

    JS_GetUCPropertyAttributes(cx, obj, nameChars, nameLen, &attrs, &found);
    if (!found)
    {
        JS_LeaveCrossCompartmentCall(call);
        JS_EndRequest(cx);
        return NULL;
    }

    JS_ClearPendingException(cx);

    if(!JS_GetUCProperty(cx, obj, nameChars, nameLen, &val))
    {
        if (JS_IsExceptionPending(cx))
        {
            if (!JS_GetPendingException(cx, &pd.value))
            {
                JS_LeaveCrossCompartmentCall(call);
                JS_EndRequest(cx);
                return NULL;
            }
            pd.flags = JSPD_EXCEPTION;
        }
        else
        {
            pd.flags = JSPD_ERROR;
            pd.value = JSVAL_VOID;
        }
    }
    else
    {
        pd.value = val;
    }

    JS_LeaveCrossCompartmentCall(call);
    JS_EndRequest(cx);

    nameval = STRING_TO_JSVAL(name);
    if (!JS_ValueToId(cx, nameval, &nameid) ||
        !JS_IdToValue(cx, nameid, &pd.id)) {
        return NULL;
    }

    pd.slot = pd.spare = 0;
    pd.alias = JSVAL_NULL;
    pd.flags |= (attrs & JSPROP_ENUMERATE) ? JSPD_ENUMERATE : 0
        | (attrs & JSPROP_READONLY)  ? JSPD_READONLY  : 0
        | (attrs & JSPROP_PERMANENT) ? JSPD_PERMANENT : 0;

    return _newProperty(jsdc, &pd, JSDPD_HINTED);
}

/*
 * Retrieve a JSFunction* from a JSDValue*. This differs from
 * JS_ValueToFunction by fully unwrapping the object first.
 */
JSFunction*
jsd_GetValueFunction(JSDContext* jsdc, JSDValue* jsdval)
{
    JSObject *obj;
    JSFunction *fun;
    JSCrossCompartmentCall *call = NULL;
    if (!JSVAL_IS_OBJECT(jsdval->val))
        return NULL;
    if(!(obj = JSVAL_TO_OBJECT(jsdval->val)))
        return NULL;
    obj = JS_UnwrapObject(jsdc->dumbContext, obj);

    call = JS_EnterCrossCompartmentCall(jsdc->dumbContext, obj);
    if (!call)
        return NULL;
    fun = JS_ValueToFunction(jsdc->dumbContext, OBJECT_TO_JSVAL(obj));
    JS_LeaveCrossCompartmentCall(call);

    return fun;
}

JSDValue*
jsd_GetValuePrototype(JSDContext* jsdc, JSDValue* jsdval)
{
    JSCrossCompartmentCall *call = NULL;

    if(!(CHECK_BIT_FLAG(jsdval->flags, GOT_PROTO)))
    {
        JSObject* obj;
        JSObject* proto;
        JS_ASSERT(!jsdval->proto);
        SET_BIT_FLAG(jsdval->flags, GOT_PROTO);
        if(!JSVAL_IS_OBJECT(jsdval->val))
            return NULL;
        if(!(obj = JSVAL_TO_OBJECT(jsdval->val)))
            return NULL;
        JS_BeginRequest(jsdc->dumbContext);
        call = JS_EnterCrossCompartmentCall(jsdc->dumbContext, obj);
        if(!call) {
            JS_EndRequest(jsdc->dumbContext);

            return NULL;
        }
        proto = JS_GetPrototype(jsdc->dumbContext, obj);
        JS_LeaveCrossCompartmentCall(call);
        JS_EndRequest(jsdc->dumbContext);
        if(!proto)
            return NULL;
        jsdval->proto = jsd_NewValue(jsdc, OBJECT_TO_JSVAL(proto));
    }
    if(jsdval->proto)
        jsdval->proto->nref++;
    return jsdval->proto;
}

JSDValue*
jsd_GetValueParent(JSDContext* jsdc, JSDValue* jsdval)
{
    JSCrossCompartmentCall *call = NULL;

    if(!(CHECK_BIT_FLAG(jsdval->flags, GOT_PARENT)))
    {
        JSObject* obj;
        JSObject* parent;
        JS_ASSERT(!jsdval->parent);
        SET_BIT_FLAG(jsdval->flags, GOT_PARENT);
        if(!JSVAL_IS_OBJECT(jsdval->val))
            return NULL;
        if(!(obj = JSVAL_TO_OBJECT(jsdval->val)))
            return NULL;
        JS_BeginRequest(jsdc->dumbContext);
        call = JS_EnterCrossCompartmentCall(jsdc->dumbContext, obj);
        if(!call) {
            JS_EndRequest(jsdc->dumbContext);

            return NULL;
        }
        parent = JS_GetParent(jsdc->dumbContext,obj);
        JS_LeaveCrossCompartmentCall(call);
        JS_EndRequest(jsdc->dumbContext);
        if(!parent)
            return NULL;
        jsdval->parent = jsd_NewValue(jsdc, OBJECT_TO_JSVAL(parent));
    }
    if(jsdval->parent)
        jsdval->parent->nref++;
    return jsdval->parent;
}

JSDValue*
jsd_GetValueConstructor(JSDContext* jsdc, JSDValue* jsdval)
{
    JSCrossCompartmentCall *call = NULL;

    if(!(CHECK_BIT_FLAG(jsdval->flags, GOT_CTOR)))
    {
        JSObject* obj;
        JSObject* proto;
        JSObject* ctor;
        JS_ASSERT(!jsdval->ctor);
        SET_BIT_FLAG(jsdval->flags, GOT_CTOR);
        if(!JSVAL_IS_OBJECT(jsdval->val))
            return NULL;
        if(!(obj = JSVAL_TO_OBJECT(jsdval->val)))
            return NULL;
        JS_BeginRequest(jsdc->dumbContext);
        call = JS_EnterCrossCompartmentCall(jsdc->dumbContext, obj);
        if(!call) {
            JS_EndRequest(jsdc->dumbContext);

            return NULL;
        }
        proto = JS_GetPrototype(jsdc->dumbContext,obj);
        if(!proto)
        {
            JS_LeaveCrossCompartmentCall(call);
            JS_EndRequest(jsdc->dumbContext);
            return NULL;
        }
        ctor = JS_GetConstructor(jsdc->dumbContext,proto);
        JS_LeaveCrossCompartmentCall(call);
        JS_EndRequest(jsdc->dumbContext);
        if(!ctor)
            return NULL;
        jsdval->ctor = jsd_NewValue(jsdc, OBJECT_TO_JSVAL(ctor));
    }
    if(jsdval->ctor)
        jsdval->ctor->nref++;
    return jsdval->ctor;
}

const char*
jsd_GetValueClassName(JSDContext* jsdc, JSDValue* jsdval)
{
    jsval val = jsdval->val;
    JSCrossCompartmentCall *call = NULL;

    if(!jsdval->className && JSVAL_IS_OBJECT(val))
    {
        JSObject* obj;
        if(!(obj = JSVAL_TO_OBJECT(val)))
            return NULL;
        JS_BeginRequest(jsdc->dumbContext);
        call = JS_EnterCrossCompartmentCall(jsdc->dumbContext, obj);
        if(!call) {
            JS_EndRequest(jsdc->dumbContext);

            return NULL;
        }
        if(JS_GET_CLASS(jsdc->dumbContext, obj))
            jsdval->className = JS_GET_CLASS(jsdc->dumbContext, obj)->name;
        JS_LeaveCrossCompartmentCall(call);
        JS_EndRequest(jsdc->dumbContext);
    }
    return jsdval->className;
}

JSDScript*
jsd_GetScriptForValue(JSDContext* jsdc, JSDValue* jsdval)
{
    JSContext* cx = jsdc->dumbContext;
    jsval val = jsdval->val;
    JSFunction* fun = NULL;
    JSExceptionState* exceptionState;
    JSScript* script = NULL;
    JSDScript* jsdscript;
    JSCrossCompartmentCall *call = NULL;

    if (!jsd_IsValueFunction(jsdc, jsdval))
        return NULL;

    JS_BeginRequest(cx);
    call = JS_EnterCrossCompartmentCall(cx, JSVAL_TO_OBJECT(val));
    if (!call) {
        JS_EndRequest(cx);

        return NULL;
    }

    exceptionState = JS_SaveExceptionState(cx);
    fun = JSD_GetValueFunction(jsdc, jsdval);
    JS_RestoreExceptionState(cx, exceptionState);
    if (fun)
        script = JS_GetFunctionScript(cx, fun);
    JS_LeaveCrossCompartmentCall(call);
    JS_EndRequest(cx);

    if (!script)
        return NULL;

    JSD_LOCK_SCRIPTS(jsdc);
    jsdscript = jsd_FindJSDScript(jsdc, script);
    JSD_UNLOCK_SCRIPTS(jsdc);
    return jsdscript;
}


/***************************************************************************/
/***************************************************************************/

JSDValue*
jsd_GetPropertyName(JSDContext* jsdc, JSDProperty* jsdprop)
{
    jsdprop->name->nref++;
    return jsdprop->name;
}

JSDValue*
jsd_GetPropertyValue(JSDContext* jsdc, JSDProperty* jsdprop)
{
    jsdprop->val->nref++;
    return jsdprop->val;
}

JSDValue*
jsd_GetPropertyAlias(JSDContext* jsdc, JSDProperty* jsdprop)
{
    if(jsdprop->alias)
        jsdprop->alias->nref++;
    return jsdprop->alias;
}

uintN
jsd_GetPropertyFlags(JSDContext* jsdc, JSDProperty* jsdprop)
{
    return jsdprop->flags;
}

uintN
jsd_GetPropertyVarArgSlot(JSDContext* jsdc, JSDProperty* jsdprop)
{
    return jsdprop->slot;
}

void
jsd_DropProperty(JSDContext* jsdc, JSDProperty* jsdprop)
{
    JS_ASSERT(jsdprop->nref > 0);
    if(0 == --jsdprop->nref)
    {
        JS_ASSERT(JS_CLIST_IS_EMPTY(&jsdprop->links));
        DROP_CLEAR_VALUE(jsdc, jsdprop->val);
        DROP_CLEAR_VALUE(jsdc, jsdprop->name);
        DROP_CLEAR_VALUE(jsdc, jsdprop->alias);
        free(jsdprop);
    }
}
