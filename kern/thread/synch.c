/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Synchronization primitives.
 * See synch.h for specifications of the functions.
 */

#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>


////////////////////////////////////////////////////////////
//
// Semaphore.


struct semaphore *
sem_create(const char *namearg, int initial_count)
{
	struct semaphore *sem;

	assert(initial_count >= 0);

	sem = kmalloc(sizeof(struct semaphore));
	if (sem == NULL) {
		return NULL;
	}

	sem->name = kstrdup(namearg);
	if (sem->name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->count = initial_count;
	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	spl = splhigh();
	assert(thread_hassleepers(sem)==0);
	splx(spl);

	/*
	 * Note: while someone could theoretically start sleeping on
	 * the semaphore after the above test but before we free it,
	 * if they're going to do that, they can just as easily wait
	 * a bit and start sleeping on the semaphore after it's been
	 * freed. Consequently, there's not a whole lot of point in 
	 * including the kfrees in the splhigh block, so we don't.
	 */

	kfree(sem->name);
	kfree(sem);
}

void P(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	assert(in_interrupt==0);

	spl = splhigh();
	while (sem->count==0) {
		thread_sleep(sem);
	}
  if (sem->count < 0)
    kprintf("\nGuys, this is sem->count: %d\n",sem->count);
	assert(sem->count>0);
	sem->count--;
	splx(spl);
}

void V(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count>0);
	thread_wakeup(sem);
	splx(spl);
}

////////////////////////////////////////////////////////////
//
// Lock.

int test_and_set(struct lock *lock) 
{ // atomic in hardware
  int old = lock->held;
  lock->held = 1;
  return old;
}

struct lock * lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(struct lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->name = kstrdup(name);
	lock->held=0;
	if (lock->name == NULL) {
		kfree(lock);
		return NULL;
	}


	// add stuff here as needed
  lock->held = 0;
	
	return lock;
}

void lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	// add stuff here as needed
  int spl;
	spl = splhigh();
	assert(thread_hassleepers(lock)==0);
	splx(spl);
	
	kfree(lock->name);
	kfree(lock);
}

void lock_acquire(struct lock *lock)
{
	// Write this
	int spl;
	assert(lock != NULL);
	spl = splhigh();
	while (test_and_set(lock)) {
	  thread_sleep(lock);
	}
	splx(spl);
}


void
lock_acquire2(struct lock *lock1,struct lock *lock2)
{
	// Write this
	int spl;
	assert(lock1 != NULL);
	assert(lock2 != NULL);
	spl = splhigh();
	while (1) {
		if (test_and_set(lock1)==0) {
			if(test_and_set(lock2)==0) break;
			
			else {
			  lock_release(lock1);
			  thread_sleep(lock2);
			}			
		}
		else thread_sleep(lock1);
	}
	splx(spl);
}

void
lock_acquire3(struct lock *lock1,struct lock *lock2,struct lock *lock3)
{
	// Write this
	int spl;
	assert(lock1 != NULL);
	assert(lock2 != NULL);
	assert(lock3 != NULL);
	spl = splhigh();
	while (1) {
		if (test_and_set(lock1)==0) {
		  if(test_and_set(lock2)==0) {
				if(test_and_set(lock3)==0) break;
				else {
			    lock_release(lock1);
					lock_release(lock2);
					thread_sleep(lock3);
			  }
      }
			
			else {
			lock_release(lock1);
			thread_sleep(lock2);
			}
    }
		else thread_sleep(lock1);
	}
	splx(spl);
}


void
lock_release(struct lock *lock)
{	
	int spl;
	spl = splhigh();
	lock->held=0;
	thread_wakeup(lock);
	splx(spl);
}

void
lock_release2(struct lock *lock1,struct lock *lock2)
{	
	int spl;
	spl = splhigh();
	lock1->held=0;
	lock2->held=0;
	thread_wakeup(lock1);
	thread_wakeup(lock2);
	splx(spl);
}
void
lock_release3(struct lock *lock1,struct lock *lock2,struct lock *lock3)
{	
	int spl;
	spl = splhigh();
	lock1->held=0;
	lock2->held=0;
	lock3->held=0;
	thread_wakeup(lock1);
	thread_wakeup(lock2);
	thread_wakeup(lock3);

	splx(spl);
}


int
lock_do_i_hold(struct lock *lock)
{
  //if (!lock->held) 
	//return 0;
  return lock->held;

  //if (lock->holder == curthread) 
	//return 1;
  //else return 0;
	return lock->held;
}

////////////////////////////////////////////////////////////
//
// CV


struct cv * cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name==NULL) {
		kfree(cv);
		return NULL;
	}
	
	// add stuff here as needed
	
	return cv;
}

void cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	// add stuff here as needed
	
	kfree(cv->name);
	kfree(cv);
}

void cv_wait(struct cv *cv, struct lock *lock)
{
	int spl;
	lock_release(lock);
	spl = splhigh();
	thread_sleep(cv);
	splx(spl);
	lock_acquire(lock);
	
}

void cv_signal(struct cv *cv, struct lock *lock)
{
	int spl;
	spl = splhigh();
	thread_wakeupone(cv);
	splx(spl);
  (void) lock;
}

void cv_broadcast(struct cv *cv, struct lock *lock)
{
	int spl;
	spl = splhigh();
	thread_wakeup(cv);
	splx(spl);
  (void) lock;
}
