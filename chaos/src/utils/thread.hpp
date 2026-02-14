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
#pragma once
#include <pthread.h>
#include <atomic>

namespace Chaos {

  /**
   * \brief Abstract class, handles a single thread.  Features mutual exclusion and pause/resume.
 */

class Thread {
public:
	Thread();
	virtual ~Thread() = 0;

	/**
	 * \brief Starts the internal thread method.
   * \return true if the thread was successfully started, false if there was an error starting the thread.
	 */
	bool start();

	/**
	 * \brief Stops the internal thread method.
   */
	void stop();

	/**
	 * \brief Will wait until the thread has finished.
   */
	void WaitForInternalThreadToExit();

	/** 
	 * \brief Performs a mutual exclusion lock.  
   * 
   * Will halt if locked by another thread, and will resume when the other thread performs an unlock.
	 * \see Thread#unlock
	 * \return 0 on success, otherwise an error corresponding to pthreads.
	 */
	int lock();

	/**
	 * \brief Performs a mutual exclusion unlock.  Will allow other threads that have been locked to
   * resume.
   * 
   * \see Thread#lock
	 * \return 0 on success, otherwise an error corresponding to pthreads.
	 */
	int unlock();

	/**
	 * \brief Performs a pause.
   * \see Thread#resume.
	 */
	void pause();

	/**
	 * \brief Resumes the thread from a pause.
   * \see Thread#pause
	 */
	void resume();

	/**
	 * \brief Returns the state of the thread.
   * \return true if running
	 */
	bool running() {
			return isRunning.load();
	}

protected:
	/**
	 * \brief Called once at the beginning of the thread running.
   * 
   * Use this to set up data for the thread.
	 */
	virtual void entryAction();

	/**
	 * \brief Continuously called until the thread is commanded to stop.
   * 
   * If not defined in the base class, the loop will simply exit. Good practice would be to make this non-blocking.
	 */
	virtual void doAction();

	/**
	 * \brief Called once when thread is commanded to stop.
   * 
   * Use this to clean up data in the thread, or place into a stopped state.
	 */
	virtual void exitAction();

	/**
	 * \brief Blocks the thread from a pause() call until resume() is called.
   */
	void checkSuspend();

private:
	static void* InternalThreadEntryFunc(void* This);

	pthread_t _thread;
	pthread_mutex_t _mutex;
	pthread_mutex_t _condMutex;
	pthread_cond_t _cond;

	bool pauseFlag;
	std::atomic<bool> isRunning;
	std::atomic<bool> shouldTerminate;
};

};
