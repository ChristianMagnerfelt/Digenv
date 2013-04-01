debug: digenv.c
	gcc -O0 -Wall -Wextra -g -o Digenv digenv.c 

release: digenv.c
	gcc -O2 -Wall -Wextra -o Digenv digenv.c

clean: 
	rm Digenv
