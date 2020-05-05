#pragma once
#include <atomic>
#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include "common.hpp"
#include "fde.hpp"
using namespace std;
class fd_events;
class connection;
class serv;

#include <arpa/inet.h>
#include <chrono>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <string.h>
#include <typeinfo>
#include <unistd.h>
#include <ifaddrs.h>

/*
 * reader
 */
class reader {
public:
	explicit reader(){ DEBUG("+reader"); };
	~reader(){ DEBUG("~reader"); };
	virtual RC read(shared_ptr<connection> conn);
protected:
	enum class READ_STATE {
		HEADER,
		BODY,
	};

	int buf_ptr = 0;
	int header_ptr = 0;
	const int read_size = READER_BUF_SIZE;
	char buf[READER_BUF_SIZE] = { 0 };
	char buf_tmp[READER_BUF_SIZE] = { 0 };
	int curr_body_size = 0;
	READ_STATE read_st = READ_STATE::HEADER;

	//header
	typedef char header[5];
	header rpc_header = { 0 };
	header curr_header;

	RC reset();
	virtual RC read_header(){
		memcpy(&curr_header, buf + header_ptr, header_size());
		curr_body_size = header_size();
		return RC::OK;
	};
	virtual RC read_body(bytes& body){
		body.append(buf + header_ptr + header_size(), body_size());
		return RC::OK;
	};
	virtual int header_size() const { return sizeof(header); };
	virtual int body_size() const { return stoi(curr_header); };
};

/*
 * connection
 */
class connection : public enable_shared_from_this<connection> {
	friend reader;

public:
	explicit connection(bool new_reader = true);
	~connection();

	static RC listen(string ip, int port, shared_ptr<connection>conn);
	static RC connect(string ip, int port, fd_events* fdes, shared_ptr<connection> ret_conn);

	virtual RC read();
	RC accept(shared_ptr<connection> cli_conn);
	RC noblock();
	RC request(bytes& body);
	RC response(bytes& body);
	RC close();
	string get_addr() const { return conn_ip + ":" + to_string(conn_port); };
	inline bool is_alive() const { return conn_alive; };
	inline int get_fd() const { return conn_fd; };

protected:
	int conn_fd;
	string conn_ip;
	int conn_port;
	atomic<bool> conn_alive;
	shared_ptr<reader> conn_reader;
	virtual RC dispatcher(bytes header, bytes body);
};

/*
 * serv
 */
class serv{
public:
	serv(string ip, int port);
	~serv();
	virtual RC start();
	virtual RC stop();

protected:
	string node_ip;
	int node_port;
	atomic<bool> is_stop;
	fd_events* fdes;
	shared_ptr<connection> server_conn = nullptr;

	virtual RC accept();
	RC server_loop();
};