/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributers.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 * This file contains code derived from the mogillc/nico library by Matt Bunting, Copyright 2016 by
 * Mogi, LLC and distributed under the LGPL library version 2 license.
 * The original version can be found here: https://github.com/mogillc/nico
 * 
 */
#include "thread.hpp"

using namespace Chaos;

void Thread::entryAction() {
}

void Thread::doAction() {
	shouldTerminate = true;
}

void Thread::exitAction() {
}

void* Thread::InternalThreadEntryFunc(void* This) {
	Thread* thread = (Thread*) This;
	thread->entryAction();
	while (!thread->shouldTerminate.load()) {
		thread->doAction();
		thread->checkSuspend();
	}
	thread->exitAction();
	thread->isRunning.store(false);
	return NULL;
}

Thread::Thread() :
			pauseFlag(false), isRunning(false), shouldTerminate(false) {
	pthread_mutex_init(&_mutex, NULL);
	pthread_mutex_init(&_condMutex, NULL);
	pthread_cond_init(&_cond, NULL);
}

Thread::~Thread() {
	WaitForInternalThreadToExit();
	pthread_mutex_destroy(&_mutex);
	pthread_mutex_destroy(&_condMutex);
	pthread_cond_destroy(&_cond);
}

/** Returns true if the thread was successfully started, false if there was an
 * error starting the thread */
bool Thread::start() {
	shouldTerminate.store(false);
	isRunning.store(true);
	isRunning.store(pthread_create(&_thread, NULL, InternalThreadEntryFunc, this)
			== 0);
	return isRunning.load();
}

void Thread::stop() {
	shouldTerminate.store(true);
	resume();
}

/** Will not return until the internal thread has exited. */
void Thread::WaitForInternalThreadToExit() {
	if (isRunning.load()) {
		stop();
		pthread_join(_thread, NULL);
	}
	// isRunning = false;
}

int Thread::lock() {
	return pthread_mutex_lock(&_mutex);
}

int Thread::unlock() {
	return pthread_mutex_unlock(&_mutex);
}

void Thread::pause() {
	pthread_mutex_lock(&_condMutex);
	pauseFlag = true;
	pthread_mutex_unlock(&_condMutex);
}

void Thread::resume() {
	pthread_mutex_lock(&_condMutex);
	pauseFlag = false;
	pthread_cond_broadcast(&_cond);
	pthread_mutex_unlock(&_condMutex);
}

void Thread::checkSuspend() {
	pthread_mutex_lock(&_condMutex);
	while (pauseFlag) {
		pthread_cond_wait(&_cond, &_condMutex);
	}
	pthread_mutex_unlock(&_condMutex);
}
