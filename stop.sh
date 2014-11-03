#! /bin/bash

while read line
do
    kill -9 $line
done < zmq.pid

rm -rf zmq.pid
