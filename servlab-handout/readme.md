[u1343789@lab1-16 servlab-handout]$ make
cc -O2 -g -Wall -I. -o friendlist friendlist.c dictionary.c more_string.c csapp.c -pthread
[u1343789@lab1-16 servlab-handout]$ ( ulimit -v 20971520 ; ./friendlist 8090 > /dev/null )


[u1343789@lab1-16 servlab-handout]$ racket stress.rkt localhost 8090
friending
friending
friending
friending
checking friends
checking friends
checking friends
unfriending
checking friends
unfriending
unfriending
unfriending
checking friends
checking friends
checking friends
checking friends
introducing
introducing
introducing
introducing
checking friends
checking friends
checking friends
checking friends