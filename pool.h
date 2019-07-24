/*
    Copyright (C) 2015-2019 Christian Lockley

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
 
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <unistd.h>

#define MAX_WORKERS 16

static 	_Atomic(bool) canceled = false;

struct threadpool
{
	pthread_t thread;
	sem_t sem;
	void *(*func)(void*);
	void * arg;
	_Atomic(int) active;
};

static struct threadpool threads[MAX_WORKERS] = {0};

static void *Worker(void *arg)
{
	struct threadpool * t = (struct threadpool *)arg;
	pthread_detach(pthread_self());
	while (true) {
		sem_wait(&t->sem);
		__sync_synchronize();
		t->func(t->arg);
		t->active = 0;
		__sync_synchronize();
	}
}

static bool ThreadPoolNew(void)
{
	for (size_t i = 0; i < MAX_WORKERS; i++) {
		sem_init(&threads[i].sem, 0, 0);
		pthread_create(&threads[i].thread, NULL, Worker, &threads[i]);
	}
	return true;
}

static bool ThreadPoolCancel(void)
{
	canceled = true;
	for (size_t i = 0; i < MAX_WORKERS; i++) {
		pthread_cancel(threads[i].thread);
	}
}

static bool ThreadPoolAddTask(void *(*entry)(void*), void * arg, bool retry)
{
	if (canceled == true) {
		return false;
	}

	do {
		for (size_t i = 0; i < MAX_WORKERS; i++) {
			if (__sync_val_compare_and_swap(&threads[i].active, 0, 1) == 0) {
				threads[i].func = entry;
				threads[i].arg = arg;
				__sync_synchronize();
				sem_post(&threads[i].sem);
				return true;
			}
		}
	} while (retry);

	return false;
}