#pragma once
#include "pch.h"
#include <atomic>
#include <vector>
#include <mutex>
#include <Windows.h>
#include <afxsmartdockingmanager.h>

#pragma warning(disable:4407)

class ThreadFuncBase {};
typedef int (ThreadFuncBase::* FUNCTYPE)(); //成员函数指针，指向ThreadFuncBase类的成员函数，返回值为int，参数列表为空
class ThreadWorker {
public:
	ThreadWorker():
		thiz(nullptr),
		func(nullptr){}
	ThreadWorker(void* obj, FUNCTYPE f) :thiz((ThreadFuncBase*)obj), func(f) {} //构造函数，接受一个对象指针和一个成员函数指针，并将它们分别赋值给thiz和func成员变量
	ThreadWorker(const ThreadWorker& worker) { //拷贝构造函数，接受一个ThreadWorker对象，并将其thiz和func成员变量分别赋值给当前对象的thiz和func成员变量
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& worker) { //赋值运算符重载
		if (this != &worker) {
			thiz = worker.thiz; 
			func = worker.func;
		}
		return *this;
	}

	int operator()() { //重载函数调用运算符，使得ThreadWorker对象可以像函数一样被调用
		if (IsValid()) {
			return (thiz->*func)(); //调用thiz对象的func成员函数，并返回其结果
		}
		return -1; //或者抛出异常，表示调用失败
	}

	bool IsValid() const { //检查ThreadWorker对象是否有效，即thiz和func都不为nullptr
		return thiz != nullptr && func != nullptr;
	}

private:
	ThreadFuncBase* thiz;
	FUNCTYPE func;

};

class MirrorThread {
public:
	MirrorThread() {
		m_hThread = NULL;
		m_bStatus = false;
	}
	~MirrorThread() {
		Stop();
	}
	bool Start() {
		m_bStatus = true; //将线程状态设置为正在运行
		m_hThread = (HANDLE)_beginthread(&MirrorThread::ThreadEntry, 0, this);
		if (!IsValid()) {
			m_bStatus = false;
		}
		return m_bStatus;
	} 

	bool IsValid() {//返回true表示有效 返回false表示线程异常或者已经终止
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE))return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}

	bool Stop() {
		if (m_bStatus == false) return true;
		m_bStatus = false;
		bool ret = TerminateThread(m_hThread, 0) != 0; //强制终止线程，返回值为非零表示成功，0表示失败
		UpdateWorker();
		return ret;
	}

	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) {  //更新线程工作对象，接受一个ThreadWorker对象作为参数，默认为一个无效的ThreadWorker对象
		if (m_worker.load() != NULL && m_worker.load() != &worker) { //如果当前线程工作对象不为nullptr且不等于传入的worker对象
			::ThreadWorker* pWorker = m_worker.load(); //获取当前线程工作对象的指针
			m_worker.store(NULL); //将当前线程工作对象更新为传入的worker对象
			delete pWorker; //删除原来的线程工作对象，释放内存
		}
		if (!worker.IsValid()) { //如果传入的worker对象无效，即thiz或func为nullptr
			m_worker.store(NULL); //将当前线程工作对象更新为nullptr
			return;
		}
		m_worker.store(new ::ThreadWorker(worker)); //将当前线程工作对象更新为传入的worker对象的指针，使用new运算符动态分配内存
		
	}
	bool IsIdle() { //检查线程是否空闲，即当前线程工作对象为nullptr
		if (m_worker == NULL)return true;
		return !m_worker.load()->IsValid();
	}
private:
	void ThreadWorker() {
		while (m_bStatus) {
			if (m_worker == NULL) {
				Sleep(1);
				continue;
			}
			::ThreadWorker worker = *m_worker.load();
			if (worker.IsValid()) {
				int ret = worker();
				if (ret != 0) {
					CString str;
					str.Format(_T("thread found warning code %d\r\n"), ret);
					OutputDebugString(str);
				}
				if (ret < 0) {
					m_worker.store(NULL);
				}
			}
			else {
				Sleep(1);
			}
		}
	}
	static void ThreadEntry(void* arg) {
		MirrorThread* thiz = (MirrorThread*)arg;
		if (thiz) {
			thiz->ThreadWorker();
		}
		_endthread();
	}
private:
	HANDLE m_hThread;
	bool m_bStatus; //线程状态，true表示正在运行，false表示已停止
	std::atomic<::ThreadWorker*> m_worker; //线程工作对象，使用std::atomic保证线程安全，可以在多个线程之间共享和修改
};


class MirrorThreadPool {
public:
	MirrorThreadPool(size_t size) { //构造函数，接受一个size参数，表示线程池的大小
		m_threads.resize(size);
		for(int i = 0; i < size; i++) {
			m_threads[i] = new MirrorThread();
		}
	}
	MirrorThreadPool() {}; //默认构造函数，创建一个空的线程池
	~MirrorThreadPool() {
		Stop();
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			MirrorThread* pThread = m_threads[i];
			m_threads[i] = NULL;
			delete pThread;
		}

		m_threads.clear();
	}
	bool Invoke() {
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++) { //遍历线程池中的每个线程对象，
			//调用其Start()方法启动线程，如果有任何一个线程启动失败，则将ret设置为false并跳出循环
			if (m_threads[i]->Start() == false) {
				ret = false;
				break;
			}
		}
		if (ret == false) {
			for (size_t i = 0; i < m_threads.size(); i++) {
				m_threads[i]->Stop();
			}
		}
		return ret;
	}

	void Stop() {
		for (size_t i = 0; i < m_threads.size(); i++) {
			m_threads[i]->Stop();
		}
	}
	//返回-1 表示分配失败，所有线程都在忙 大于等于0，表示第n个线程分配来做这个事情
	int DispatchWorker(const ThreadWorker& worker) { 
		int index = -1;
		m_lock.lock(); //加锁，保证线程安全
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i] != NULL && m_threads[i]->IsIdle()) {
				m_threads[i]->UpdateWorker(worker);
				index = static_cast<int>(i);
				break;
			}
		}
		m_lock.unlock();
		return index;
	}
	bool CheckThreadValid(size_t index) {
		if (index < m_threads.size()) {
			return m_threads[index]->IsValid();
		}
		return false;
	}
private:
	std::mutex m_lock;   //线程池锁，使用std::mutex保证线程安全，保护对线程池的访问和修改
	std::vector<MirrorThread*> m_threads; //线程池中的线程对象，使用std::vector存储，可以动态调整线程数量
};
