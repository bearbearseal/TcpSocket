#include "TcpListener.h"

using namespace std;

TcpListener::TcpListener(uint16_t _portNumber) {
	portNumber = _portNumber;
    theProcess = nullptr;
	myShadow = make_shared<Shadow>(*this);
}

TcpListener::~TcpListener() {
	stop();
}

void TcpListener::start() {
	//terminate.store(false);
    printf("Start called.\n");
	lock_guard<mutex> lock(connectionMutex);
	if (theProcess == nullptr) {
		terminate = false;
		theProcess = make_unique<std::thread>(thread_process, this);
	}
}

void TcpListener::stop() {
	terminate = true;
	while (socketAddress2Connection.size()) {
		socketAddress2Connection.begin()->second->stop();
	}
	theProcess->join();
}

void TcpListener::catch_message(std::string& data, size_t handle) {
	printf("Caught message %s from Socket %lX\n", data.c_str(), handle);
	string reply = "Got your message: ";
	reply += data;
	write_message(handle, reply);
}

void TcpListener::catch_connect_event(size_t handle) {
	printf("Caught connect event from Socket %lX\n", handle);
}

void TcpListener::catch_disconnect_event(size_t handle) {
	printf("Caught disconnect event from socket %lX\n", handle);
}

bool TcpListener::write_message(size_t handle, const std::string& message) {
	lock_guard<mutex> lock(connectionMutex);
	auto i = socketAddress2Connection.find(handle);
	if (i != socketAddress2Connection.end()) {
		return ((TcpSocket*)i->first)->write(message);
	}
	return false;
}

void TcpListener::close_socket(size_t handle) {
    /*
    connectionMutex.lock();
    auto i = socketAddress2Connection.find(handle);
    if(i == socketAddress2Connection.end()) {
        connectionMutex.unlock();
        return;
    }
    i->second->stop();
        socketAddress2Connection.erase(i);
    }
    */
}

//Only mark to delete, do it at main thread
void TcpListener::free_connection(size_t handle) {
    //printf("Freeing.\n");
    lock_guard<mutex> lock(markedMutex);
    markedRemove.emplace(handle);
    //printf("End of free.\n");
}

void TcpListener::thread_process(TcpListener* me) {
	printf("Listen socket listening.\n");
	me->listenSocket.listen(me->portNumber);
	while (1) {
		auto tcpSocket = make_unique<TcpSocket>();
		printf("Waiting for connection, socket: %p.\n", tcpSocket.get());
		me->listenSocket.accept(*(tcpSocket.get()));
        printf("Accepted, gonna lock.\n");
		{
			lock_guard<mutex> lock(me->connectionMutex);
			if (me->terminate) {
				tcpSocket->close();
				//delete tcpSocket;
				return;
			}
			else {
                size_t key = (size_t) tcpSocket.get();
                auto singleConnection = make_unique<SingleConnection>(me->myShadow, move(tcpSocket));
				me->socketAddress2Connection.insert({ key, move(singleConnection) });
                me->socketAddress2Connection[key]->start();
				me->catch_connect_event(key);
			}
            {
                lock_guard<mutex> otherLock(me->markedMutex);
                for(size_t key : me->markedRemove) {
                    me->socketAddress2Connection[key]->stop();
                    me->socketAddress2Connection.erase(key);
                }
                me->markedRemove.clear();
            }
		}
	}
}

TcpListener::SingleConnection::SingleConnection(shared_ptr<Shadow> _master, unique_ptr<TcpSocket> _socket) {
	master = _master;
	socket = move(_socket);
}

TcpListener::SingleConnection::~SingleConnection() {
    //printf("Single connection destroyed and my socket address is %p\n", this->socket.get());
}

void TcpListener::SingleConnection::start() {
	theThread = make_unique<thread>(thread_process, this);
}

void TcpListener::SingleConnection::stop() {
    theThread->join();
}

bool TcpListener::SingleConnection::write(const std::string& message) {
	return socket->write(message);
}

void TcpListener::SingleConnection::thread_process(SingleConnection* me) {
	printf("Socket thread starting.\n");
	while (1) {
		printf("Pulling message.\n");
		auto input = me->socket->read();
		if (input.first) {
			auto shared = me->master.lock();
			if (shared != nullptr) {
				printf("Incoming message: %s\n", input.second.c_str());
				shared->catch_message(input.second, (size_t) me->socket.get());
			}
		}
		else {
			auto shared = me->master.lock();
			if (shared != nullptr) {
                size_t socketHandle = (size_t) me->socket.get();
				shared->catch_disconnect_event(socketHandle);
				me->socket->close();
				shared->free_connection(socketHandle);
			}
			printf("Disconnected.\n");
			return;
		}
	}
}