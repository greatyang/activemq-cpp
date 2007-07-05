/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MutexTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION( activemq::concurrent::MutexTest );

using namespace std;
using namespace activemq;
using namespace activemq::concurrent;

///////////////////////////////////////////////////////////////////////////////
void MutexTest::testTimedWait(){

   try
   {
      MyTimedWaitingThread test;
      time_t startTime = time( NULL );
      test.start();
      test.join();
      time_t endTime = time( NULL );

      time_t delta = endTime - startTime;

      CPPUNIT_ASSERT( delta >= 1 && delta <= 3 );
  }
  catch(exceptions::ActiveMQException& ex)
  {
     std::cout << ex.getMessage() << std::endl;
  }
}

///////////////////////////////////////////////////////////////////////////////
void MutexTest::testWait(){

    try
    {
        MyWaitingThread test;
        test.start();

        Thread::sleep(1000);

        synchronized(&test)
        {
            for( int ix=0; ix<100; ix++ ){
                test.value += 1;
            }

            test.notify();
        }

        test.join();

        CPPUNIT_ASSERT( test.value == 2500 );
    }
    catch(exceptions::ActiveMQException& ex)
    {
        ex.setMark( __FILE__, __LINE__ );
    }
}

///////////////////////////////////////////////////////////////////////////////
void MutexTest::test()
{
    MyThread test;

    synchronized(&test){

        test.start();

        for( int ix=0; ix<100; ix++ ){
            test.value += 1;
        }
    }

    test.join();

    CPPUNIT_ASSERT( test.value == 2500 );
}

///////////////////////////////////////////////////////////////////////////////
void MutexTest::testNotify()
{
    try{
        Mutex mutex;
        Mutex started;
        Mutex completed;

        const int numThreads = 30;
        MyNotifiedThread* threads[numThreads];

        // Create and start all the threads.
        for( int ix=0; ix<numThreads; ++ix ){
            threads[ix] = new MyNotifiedThread( &mutex, &started, &completed );
            threads[ix]->start();
        }

        synchronized( &started )
        {
            int count = 0;

            while( count < ( numThreads ) )
            {
                started.wait( 30 );
                count++;
            }
        }

        synchronized(&mutex)
        {
            mutex.notify();
        }

        Thread::sleep( 1000 );

        int counter = 0;
        for( int ix=0; ix<numThreads; ++ix ){
            if( threads[ix]->done ){
                counter++;
            }
        }

        // Make sure only 1 thread was notified.
        CPPUNIT_ASSERT( counter == 1 );

        synchronized(&mutex)
        {
            // Notify all threads.
            for( int ix=0; ix<numThreads-1; ++ix ){
                mutex.notify();
            }
        }

        synchronized( &started )
        {
            int count = 0;

            while( count < ( numThreads ) )
            {
                started.wait( 30 );
                count++;
            }
        }

        int numComplete = 0;
        for( int ix=0; ix<numThreads; ++ix ){
            if( threads[ix]->done ){
                numComplete++;
            }
        }
        CPPUNIT_ASSERT( numComplete == numThreads );

        synchronized( &mutex ){
          mutex.wait( 5 );
        }

        synchronized( &mutex )
        {
            mutex.notifyAll();
        }

        // Delete all the threads.
        for( int ix=0; ix<numThreads; ++ix ){
            delete threads[ix];
        }

    }catch( exceptions::ActiveMQException& ex ){
        ex.setMark( __FILE__, __LINE__ );
    }
}

///////////////////////////////////////////////////////////////////////////////
void MutexTest::testNotifyAll()
{
    try{
        Mutex mutex;
        Mutex started;
        Mutex completed;

        const int numThreads = 100;
        MyNotifiedThread* threads[numThreads];

        // Create and start all the threads.
        for( int ix=0; ix<numThreads; ++ix ){
            threads[ix] = new MyNotifiedThread( &mutex, &started, &completed );
            threads[ix]->start();
        }

        synchronized( &started )
        {
            int count = 0;

            while( count < ( numThreads ) )
            {
                started.wait( 30 );
                count++;
            }
        }

        for( int ix=0; ix<numThreads; ++ix )
        {
            if( threads[ix]->done == true ){
                printf("threads[%d] is done prematurely\n", ix );
            }
            CPPUNIT_ASSERT( threads[ix]->done == false );
        }

        // Notify all threads.
        synchronized( &mutex ){
            mutex.notifyAll();
        }

        synchronized( &completed )
        {
            int count = 0;

            while( count < ( numThreads ) )
            {
                completed.wait( 30 );
                count++;
            }
        }

        int numComplete = 0;
        for( int ix=0; ix<numThreads; ++ix ){
            if( threads[ix]->done ){
                numComplete++;
            }
        }
        //printf("numComplete: %d, numThreads: %d\n", numComplete, numThreads );
        CPPUNIT_ASSERT( numComplete == numThreads );

        // Delete all the threads.
        for( int ix=0; ix<numThreads; ++ix ){
            threads[ix]->join();
            delete threads[ix];
        }

    }catch( exceptions::ActiveMQException& ex ){
        ex.setMark( __FILE__, __LINE__ );
    }
}

///////////////////////////////////////////////////////////////////////////////
void MutexTest::testRecursiveLock()
{
    try{
        Mutex mutex;

        const int numThreads = 30;
        MyRecursiveLockThread* threads[numThreads];

        // Create and start all the threads.
        for( int ix=0; ix<numThreads; ++ix ){
            threads[ix] = new MyRecursiveLockThread( &mutex );
            threads[ix]->start();
        }

        // Sleep so all the threads can get to the wait.
        Thread::sleep( 1000 );

        for( int ix=0; ix<numThreads; ++ix ){
            if( threads[ix]->done == true ){
                std::cout << "threads[" << ix
                          << "] is done prematurely\n";
            }
            CPPUNIT_ASSERT( threads[ix]->done == false );
        }

        // Notify all threads.
        synchronized( &mutex )
        {
            synchronized( &mutex )
            {
                mutex.notifyAll();
            }
        }

        // Sleep to give the threads time to wake up.
        Thread::sleep( 1000 );

        for( int ix=0; ix<numThreads; ++ix ){
            if( threads[ix]->done != true ){
                std::cout<< "threads[" << ix << "] is not done\n";
            }
            CPPUNIT_ASSERT( threads[ix]->done == true );
        }

        // Delete all the threads.
        for( int ix=0; ix<numThreads; ++ix ){
            delete threads[ix];
        }
    }catch( exceptions::ActiveMQException& ex ){
        ex.setMark( __FILE__, __LINE__ );
    }
}

///////////////////////////////////////////////////////////////////////////////
void MutexTest::testDoubleLock() {

    try{
        Mutex mutex1;
        Mutex mutex2;

        MyDoubleLockThread thread(&mutex1, &mutex2);

        thread.start();

        // Let the thread get both locks
        Thread::sleep( 200 );

        // Lock mutex 2, thread is waiting on it
        synchronized(&mutex2)
        {
           mutex2.notify();
        }

        // Let the thread die
        thread.join();

        CPPUNIT_ASSERT( thread.done );
    }catch( exceptions::ActiveMQException& ex ){
        ex.setMark( __FILE__, __LINE__ );
    }
}
