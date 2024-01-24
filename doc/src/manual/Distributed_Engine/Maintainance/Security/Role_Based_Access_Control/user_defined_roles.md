
用户自定义角色是指用户根据自己的需求创建的角色。用户自定义角色可以被授予给其他自定义角色，也可以被直接授予给用户。

用户自定义角色的命名不能以`_`开头。

###用户自定义角色模型###

用户自定义角色模型如下所示，其中`Privileges`是权限列表，定义了该角色直接拥有的权限；`Roles`是角色列表，定义了该角色继承的角色，从继承角色上间接获取权限。

```lang-javascript
{
   "Role": <role name>,
   "Privileges": [
      {
         Resource: <Resource>
         Actions: [<action>, <action>, ...]
      },
      ...
   ],
   "Roles": [
      <role name>,
      ...
   ]
}
```

