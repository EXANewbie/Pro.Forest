//#include "que.h"
//
//void SYNCHED_QUEUE::push(msg i) {
//	mtx.lock();
//	que.push(i);
//	mtx.unlock();
//}
//
//msg& SYNCHED_QUEUE::front() {
//	mtx.lock();
//	msg t = que.front();
//	mtx.unlock();
//	return t;
//}
//
//void SYNCHED_QUEUE::pop() {
//	mtx.lock();
//	que.pop();
//	mtx.unlock();
//}
//
//int SYNCHED_QUEUE::size() {
//	mtx.lock();
//	int t = que.size();
//	mtx.unlock();
//	return t;
//}
//
//bool SYNCHED_QUEUE::empty() {
//	mtx.lock();
//	bool t = que.empty();
//	mtx.unlock();
//	return t;
//}