[^_^]:
   目录名：告警消息接收

SequoiaPerf 的默认配置模板是不发送任何告警信息给任何邮箱或者下游客户端。

用户可以在告警概览页面看到所有告警，但是除告警概览页面外不会额外收到告警信息。

```lang-bash
cat /opt/sequoiaperf/instances/SequoiaPerf1/sequoiaperf_alertmanager/conf/alertmanager.yml

global:
  resolve_timeout: 5m
#  smtp_smarthost: 'smtp.qiye.163.com:25'
#  smtp_from: 'xxx@sequoiadb.com'
#  smtp_auth_username: 'xxx@sequoiadb.com'
#  smtp_auth_password: 'xxxxxxx'
#  smtp_require_tls: true
route:
  group_by: ['alertname']
  group_wait: 10s
  group_interval: 10s
  repeat_interval: 1h
  receiver: 'default'
receivers:
  - name: 'default'
#    webhook_configs:
#    - url: 'http://127.0.0.1:5001/'
#    email_configs:
#    - to: 'xxx@sequoiadb.com'
inhibit_rules:
  - source_match:
      severity: 'critical'
    target_match:
      severity: 'warning'
    equal: ['alertname', 'dev', 'instance']
```


如果需要配置发送告警，需修改相应实例中的 `alertmanager.yml` 文件填入相应的正确邮箱信息或者下游客户端监听信息。更详细信息可参阅 [prometheus 官方文档][alertmanager_website]


[^_^]:
    本文使用的所有引用及链接

[alertmanager_website]:https://prometheus.io/docs/alerting/latest/configuration/


