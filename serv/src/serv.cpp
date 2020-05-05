#include "serv.hpp"

/*
 * reader
 */
RC reader::reset(){
	int remain_size = buf_ptr - header_ptr;
	memcpy(buf_tmp, buf + header_ptr, remain_size);
	memset(buf, 0, READER_BUF_SIZE);
	memcpy(buf, buf_tmp, remain_size);

	header_ptr = 0;
	buf_ptr = remain_size;
	return RC::OK;
}

RC reader::read(shared_ptr<connection> conn){
	if (READER_BUF_SIZE <= buf_ptr + read_size) reset();
	int len = ::read(conn->get_fd(), buf + buf_ptr, read_size);
	buf_ptr += len;

	for (;;){
		//reader header
		if (read_st == READ_STATE::HEADER && buf_ptr - header_ptr > header_size()){
			read_header();
			read_st = READ_STATE::BODY;
			continue;
		}

		//header finish read
		if (read_st == READ_STATE::BODY
			&& header_ptr + header_size() + body_size() <= buf_ptr) {

			string header;
			string body;
			header.append(buf + header_ptr, header_size());
			read_body(body);
			conn->dispatcher(header, body);

			read_st = READ_STATE::HEADER;
			header_ptr = header_ptr + header_size() + body_size();
			curr_body_size = 0;
			continue;
		}
		break;
	}
	return RC::OK;
}

/*
 * connection
 */
connection::connection(bool new_reader):
conn_fd(0),
conn_alive(false)
{
	if (new_reader) conn_reader = make_shared<reader>();
}

connection::~connection(){
	DEBUG("~connection");
}

RC connection::listen(string ip, int port, shared_ptr<connection> conn){
	struct sockaddr_in addr;
	int sock = -1;

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((short)port);
	inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

	if ((sock = ::socket(AF_INET, SOCK_STREAM, 0)) == -1) assert(false);
	if (::bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) assert(false);
	if (::listen(sock, 1024) == -1) assert(false);

	conn->conn_fd = sock;
	return RC::OK;
}

RC connection::accept(shared_ptr<connection> cli_conn){
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	int infd = ::accept(conn_fd, (struct sockaddr *)&addr, &addrlen);

	cli_conn->conn_fd = infd;
	cli_conn->noblock();
	cli_conn->conn_alive = true;
	return RC::OK;
}

RC connection::noblock(){
	int flags;
	if (-1 == (flags = fcntl(conn_fd, F_GETFL, 0)))
		flags |= O_NONBLOCK;
	fcntl(conn_fd, F_SETFL, flags | O_NONBLOCK);
	return RC::OK;
}

RC connection::close(){
	DEBUG("close connection %d", conn_fd);
	::close(conn_fd);
	conn_fd = 0;
	conn_alive = false;
	return RC::OK;
}

RC connection::read(){
	return conn_reader->read(shared_from_this());
};

RC connection::connect(string ip, int port, fd_events* fdes, shared_ptr<connection> ret_conn){
	int sock = -1;
	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((short)port);
	inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

	if ((sock = ::socket(AF_INET, SOCK_STREAM, 0)) == -1) return RC::ERROR;
	if (::connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) return RC::ERROR;

	ret_conn->conn_ip = ip;
	ret_conn->conn_port = port;
	ret_conn->conn_fd = sock;
	ret_conn->conn_alive = true;
	fdes->set(ret_conn);
	return RC::OK;
}

RC connection::request(bytes& body){
	int ctx_size = 5 + body.size();
	char* ctx = new char[ctx_size]();

	char header[5] = { 0 };
	sprintf(header, "%d", (int)body.size());

	memcpy(ctx, header, 5);
	memcpy(ctx + 5, body.data(), body.size());
	auto retval = ::write(conn_fd, ctx, ctx_size);

	delete[] ctx;
	return retval > 0 ? RC::OK : RC::ERROR;
}

RC connection::response(bytes& body){
	int ctx_size = 5 + body.size();
	char* ctx = new char[ctx_size]();

	char header[5] = { 0 };
	sprintf(header, "%d", (int)body.size());

	memcpy(ctx, header, 5);
	memcpy(ctx + 5, body.data(), body.size());
	auto retval = ::write(conn_fd, ctx, ctx_size);

	delete[] ctx;
	return retval > 0 ? RC::OK : RC::ERROR;
}

RC connection::dispatcher(bytes header, bytes body){
	DEBUG("serv dispatcher %s", body.c_str());
	return response(body);
}

/*
* serv
*/
serv::serv(string ip_, int port_){
	node_ip = ip_;
	node_port = port_;
	is_stop = true;
	fdes = new fd_events(3);
};

serv::~serv(){
	delete fdes;
};

RC serv::start(){
	server_conn = make_shared<connection>(false);
	auto retval = connection::listen(node_ip, node_port, server_conn);
	assert(retval == RC::OK);
	fdes->set(server_conn);
	is_stop = false;
	server_loop();
	return RC::OK;
}

RC serv::server_loop(){
	int n = -1;
	while ((n = fdes->wait()) != -1 && !is_stop){
		for (int i = 0; i < n; i++) {
			shared_ptr<connection> conn = nullptr;
			if ((conn = fdes->get_conn(i)) != nullptr){
				if (conn == server_conn) accept();
				else conn->read();
			}
		}
	}
	return RC::OK;
}

RC serv::accept(){
	DEBUG("server accept");
	shared_ptr<connection> cli_conn = make_shared<connection>();
	server_conn->accept(cli_conn);
	return fdes->set(cli_conn);
}

RC serv::stop(){
	is_stop = true;
	DEBUG("server stop");
	return RC::OK;
}

