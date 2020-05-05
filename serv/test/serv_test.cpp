#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <vector>
#include <deque>
#include <mutex>
#include <string>
#include <time.h>
#include <thread>
#include "gtest/gtest.h"
#include "serv.hpp"
using namespace std;

class connection_client : public connection {
public:
	explicit connection_client(){};
	~connection_client(){};
	RC read(){
		return conn_reader->read(shared_from_base<connection_client>());
	}

private:
	SHARED_FROM_BASE
	RC dispatcher(bytes header, bytes body){
		static int count = 0;
		bytes expect = "this_is_test" + to_string(count);
		EXPECT_EQ(body, expect);
		count++;
		return RC::OK;
	}
};

TEST(serv, connect_1_request_100000){
	const char* ip = "127.0.0.1";
	int port = 5566;

	shared_ptr<serv> server = make_shared<serv>(ip, port);
	thread t(&serv::start, server);
	this_thread::sleep_for(chrono::seconds(1));
	
	//start request
	fd_events fdes;
	auto curr_conn = make_shared<connection_client>();
	EXPECT_EQ(connection::connect(ip, port, &fdes, curr_conn), RC::OK);

	for (int i = 0; i < 100000; i++){
		bytes cmd = "this_is_test" + to_string(i);
		curr_conn->request(cmd);
		fdes.wait();
		curr_conn->read();
		this_thread::sleep_for(chrono::microseconds(10));
	}

	curr_conn->close();
	server->stop();
	t.join();
}