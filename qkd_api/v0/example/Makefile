alice: alice.c	../qkd_api.c ../../cqc.c
	gcc alice.c -I ../../ ../qkd_api.c ../../cqc.c -o bin/alice

bob: bob.c	../qkd_api.c ../../cqc.c
	gcc bob.c -I ../../ ../qkd_api.c ../../cqc.c -o bin/bob

all: alice bob

run:
	simulaqron stop --name network1
	simulaqron start --name network1 --keep