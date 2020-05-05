#include "src/common.hpp"
#include "src/serv.hpp"

int main(){
	serv* server = new serv("127.0.0.1", 5566);
	server->start();
}