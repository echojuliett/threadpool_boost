/*
 *
 *    Boss/worker Model with a Thread pool using shared_ptr based on Boost C++ Libraries 1.57.0
 *
 *    Written by EuiJong Hwang
 *
 */

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <iostream>
#include <csignal>
#include <string>
#include <vector>
#include <list>

#define MAX_QUEUE_SIZE	20
#define MAX_POOL_SIZE	10

class TaskQueue
{
	public:
		TaskQueue(int queue_size);
		~TaskQueue();
		void PushTask(int i);
		void PopTask(std::string msg);
		static void StartThread(TaskQueue* taskQueue); // function executed by worker threads
		void GetPacketData(); // function called by StartThread function
		void ListeningTaskQueue();
	private:
		std::list<int> task_queue;
		boost::mutex mutex;
		boost::condition_variable cond;
};

typedef boost::shared_ptr<boost::thread> ThreadSharedPtr;

class ThreadPool
{
	public:
		ThreadPool(int pool_size, TaskQueue* taskQueue);
		~ThreadPool();
		void Join();
	private:
		std::vector<ThreadSharedPtr> worker_threads;
};

bool SIGINT_FLAG = false;

/*
 * SIGINT Handler
 */
void SignalHandler(int sig_value) {
	SIGINT_FLAG = true;
}

boost::random::mt19937 gen;

/*
 * Task Queue Constructor
 */
TaskQueue::TaskQueue(int queue_size) {

	task_queue.clear();

	for (int i = 0; i < queue_size; ++i) {
		task_queue.push_back(i);
	}
}

/*
 * Task Queue Destructor
 */
TaskQueue::~TaskQueue() {

	task_queue.clear();
}

/*
 * Push data into Task Queue
 */
void TaskQueue::PushTask(int i) {

	task_queue.push_back(i);
}

/*
 * Pop data from Task Queue
 */
void TaskQueue::PopTask(std::string msg) {

	std::cout << msg  << ": task_queue.pop_front(): " << task_queue.front() << std::endl;
	std::cout << msg << " ID: " << boost::this_thread::get_id() << std::endl;
	task_queue.pop_front();
}

/*
 * static Start Thread
 */
void TaskQueue::StartThread(TaskQueue* taskQueue) {

	taskQueue->GetPacketData();
}

/*
 * Get Packet Data
 */
void TaskQueue::GetPacketData() {

	std::string msg = "Worker";

	while (true) {

		boost::unique_lock<boost::mutex> lock(mutex);
		cond.wait(lock);

		/*
		 * if the SIGINT event occurs, all worker threads exit the loop
		 */
		if (SIGINT_FLAG) {
			lock.unlock();
			break;
		}

		PopTask(msg); // Critical Section
		lock.unlock();
	}
}

/*
 * Listen Task Queue
 */
void TaskQueue::ListeningTaskQueue() {

	std::string msg = "Boss";
	boost::random::mt19937 gen;
	boost::random::uniform_int_distribution<> dist(0, 9);

	signal(SIGINT, SignalHandler);

	while (true) {

		boost::unique_lock<boost::mutex> lock(mutex);

		/*
		 *
		 * if the SIGINT event occurs, the Boss wake up all worker threads
		 *
		 */
		if (SIGINT_FLAG) {
			std::cout << msg << ": Listening Queue is stopped..." << std::endl;
			cond.notify_all();
			lock.unlock();
			break;
		}

		if (task_queue.empty()) {
			std::cout << msg << ": Task Queue is Empty..." << std::endl;
			PushTask(dist(gen));
			std::cout << msg << ": Push into Task Queue: " << task_queue.back() << std::endl;
			boost::this_thread::sleep_for(boost::chrono::milliseconds(500));

			lock.unlock();
			continue;
		}

		/*
		 * Task Queue is Not Empty
		 */
		cond.notify_one();
		lock.unlock();
		boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
	}
}

/*
 * Thread pool Constructor
 */
ThreadPool::ThreadPool(int pool_size, TaskQueue* taskQueue) {

	worker_threads.clear();

	for (int i = 0; i < pool_size; ++i) {
		worker_threads.push_back(ThreadSharedPtr(new boost::thread(TaskQueue::StartThread, taskQueue)));
		boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
	}
}

/*
 * Thread pool Destructor
 */
ThreadPool::~ThreadPool() {

	worker_threads.clear();
}

/*
 * Join Thread
 */
void ThreadPool::Join() {

	for (int i = 0; i < worker_threads.size(); ++i) {
		worker_threads[i]->join();
	}
}

/*
 * Check Arguments
 */
int CheckArgs(int args, char** argv) {

	if (args == 3) {

		int queue_size = atoi(argv[1]);
		int pool_size = atoi(argv[2]);

		if ((queue_size > 0 && queue_size <= MAX_QUEUE_SIZE) && (pool_size > 0 && pool_size <= MAX_POOL_SIZE)) {
			return 0;
		}
	}
	std::cout << "\nusage: ./pool [Queue Size] [Thread Count] // MAX_QUEUE_SIZE == 20, MAX_POOL_SIZE == 10\n" << std::endl;
	return -1;
}

int main(int args, char** argv) {

	if (CheckArgs(args, argv) != 0) {
		return -1;
	}

	TaskQueue* taskQueue = new TaskQueue(atoi(argv[1]));
	ThreadPool* threadPool = new ThreadPool(atoi(argv[2]), taskQueue);

	taskQueue->ListeningTaskQueue();
	threadPool->Join();

	delete taskQueue;
	delete threadPool;
	
	return 0;
}
