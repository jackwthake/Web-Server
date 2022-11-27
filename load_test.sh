#!/bin/bash
for i in {1..100}
do
   curl localhost:8000 -s -o /dev/null
done