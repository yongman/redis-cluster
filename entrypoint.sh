#! /bin/sh
#
# entrypoint.sh
# Copyright (C) 2017 yanming02 <yanming02@baidu.com>
#
# Distributed under terms of the MIT license.
#
port=$1
sed -i "s/PORT/$port/g" /opt/redis/redis-demo.conf

/opt/redis/src/redis-server /opt/redis/redis-demo.conf
