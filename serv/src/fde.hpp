#pragma once
#include <sys/epoll.h>
#include <memory>
#include "common.hpp"
#include "serv.hpp"
class connection;

class fd_events{
public:
	explicit fd_events(int timeout_ = 0);
	~fd_events();
	RC set(shared_ptr<connection> conn);
	RC remove(shared_ptr<connection> conn);
	shared_ptr<connection> get_conn(int fd);
	int wait();

private:
	struct fd_event{
		shared_ptr<connection> conn = nullptr;
	};

	int efd;
	int timeout = 0;
	struct epoll_event* events;

};