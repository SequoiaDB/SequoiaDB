
##名称##

forceGC - 强制 javascript 引擎回收已经释放的对象资源

##语法##

**forceGC()**

##类别##

Global

##描述##

当用户不再使用某些 javascript 对象后，该对象持有的资源不能马上释放。用户可调用该接口，要求 javascript 引擎回收已经释放的对象资源。

##参数##

无

##返回值##

无

##错误##

无

##版本##

v2.6 及以上版本

##示例##

回收对象资源

```lang-javascript
> for (var i = 0; i < 10000; i++) { var obj = new Object(); }
> forceGC()
```