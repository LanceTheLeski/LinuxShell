all: shell352

shell352: project-starter.c
	gcc -o shell352 project-starter.c -I. -lm

clean:
	rm shell352

gensine: main.c gensnd.c
	gcc -o gensine main.c gensnd.c -I. -lm

gendial: main2.c gensnd.c
	gcc -o gendial main2.c gensnd.c -I. -lm

dtmf: main1b.c gensnd.c
	gcc -o dtmf main1b.c gensnd.c -I. -lm