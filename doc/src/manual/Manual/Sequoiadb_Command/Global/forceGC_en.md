
##NAME##

forceGC - Force garbage collection of javascript objects.

##SYNOPSIS##

**forceGC()**

##CATEGORY##

Global

##DESCRIPTION##

Force garbage collection of javascript objects.

##PARAMETERS##

NULL.

##RETURN VALUE##

NULL.

##ERRORS##

NULL.

##HISTORY##

v2.6 and above.

##EXAMPLES##

- Recycle object resources.

```lang-javascript
> for (var i = 0; i < 10000; i++) { var obj = new Object(); }
> forceGC()
```