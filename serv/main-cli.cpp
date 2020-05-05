#include "src/common.hpp"
#include "src/serv.hpp"
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
		cout << "echo:" << body << endl; 
		return RC::OK;
	}
};

int main(){
	fd_events fdes;
	auto curr_conn = make_shared<connection_client>();
	if (connection::connect("127.0.0.1", 5566, &fdes, curr_conn) == RC::OK){
		for (int i = 0; i < 10; i++){
			bytes cmd = "this_is_test";
			curr_conn->request(cmd);
			fdes.wait();
			curr_conn->read();
			this_thread::sleep_for(chrono::milliseconds(1));
		}
	}
	else assert(false);
}
