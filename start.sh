#! /bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage : $0 num_servers"
    exit 0
fi

./broker &
brokerID=$!
echo "Broker running with PID $brokerID"
echo "$!" >> zmq.pid

for i in $(eval echo "{1..$1}"); do
  parent=$((i / 2))
  ./server localhost $i localhost:$((parent+4444)) &
  echo "Server running with PID $!"
  echo "$!" >> zmq.pid
done
