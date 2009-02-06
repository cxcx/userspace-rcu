#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include "urcu.h"

struct test_array {
	int a;
	int b;
	char c[200];
};

static struct test_array *test_rcu_pointer;

#define NR_READ 1000
#define NR_WRITE 50


void *thr_reader(void *arg)
{
	int qparity, i;
	struct test_array *local_ptr;

	printf("thread %s, thread id : %lu, pid %lu\n",
			"reader", pthread_self(), getpid());
	sleep(2);

	urcu_register_thread();

	for (i = 0; i < 1000; i++) {
		qparity = rcu_read_lock();
		local_ptr = rcu_dereference(test_rcu_pointer);
		if (local_ptr) {
			assert(local_ptr->a == 8);
			assert(local_ptr->b == 12);
			assert(local_ptr->c[55] == 2);
		}
		rcu_read_unlock(qparity);
	}

	urcu_unregister_thread();

	return ((void*)1);

}

void *thr_writer(void *arg)
{
	int i;
	struct test_array *new, *old;

	printf("thread %s, thread id : %lu, pid %lu\n",
			"writer", pthread_self(), getpid());
	sleep(2);

	for (i = 0; i < 1000; i++) {
		rcu_write_lock();
		new = malloc(sizeof(struct test_array));
		old = test_rcu_pointer;
		if (old) {
			assert(old->a == 8);
			assert(old->b == 12);
			assert(old->c[55] == 2);
		}
		assert(new->a = 8);
		assert(new->b = 12);
		assert(new->c[55] = 2);
		old = urcu_publish_content(&test_rcu_pointer, new);
		rcu_write_unlock();
		/* can be done after unlock */
		free(old);
	}

	return ((void*)2);
}

int main()
{
	int err;
	pthread_t tid_reader[NR_READ], tid_writer[NR_WRITE];
	void *tret;
	int i;

	for (i = 0; i < NR_READ; i++) {
		err = pthread_create(&tid_reader[i], NULL, thr_reader, NULL);
		if (err != 0)
			exit(1);
	}
	for (i = 0; i < NR_WRITE; i++) {
		err = pthread_create(&tid_writer[i], NULL, thr_writer, NULL);
		if (err != 0)
			exit(1);
	}

	sleep(10);

	for (i = 0; i < NR_WRITE; i++) {
		err = pthread_join(tid_reader[i], &tret);
		if (err != 0)
			exit(1);
	}
	for (i = 0; i < NR_WRITE; i++) {
		err = pthread_join(tid_writer[i], &tret);
		if (err != 0)
			exit(1);
	}

	return 0;
}
