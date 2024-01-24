ps -ef | grep "ecmtest\.jar" | awk '{print $2}' | xargs kill -15
