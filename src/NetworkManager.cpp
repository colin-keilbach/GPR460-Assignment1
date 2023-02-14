#include "RoboCatPCH.h"
#include "NetworkManager.h"
#include <iostream>

/// <summary>
/// Initialize listenSocket so it can receive incoming connections.
/// listenSocket must not block on any future calls.
/// </summary>
void NetworkManager::Init()
{
	// Create Socket
	listenSocket = SocketUtil::CreateTCPSocket(SocketAddressFamily::INET);
	assert(listenSocket != nullptr);

	// Set Non-Blocking to true
	listenSocket->SetNonBlockingMode(true);

	// Bind Socket to Address
	// 127.0.0.1 is only self connections
	// 0.0.0.0 is any connections
	SocketAddressPtr address = SocketAddressFactory::CreateIPv4FromString("0.0.0.0"); // no port chooses its own
	assert(address != nullptr);
	int result = listenSocket->Bind(*address);
	assert(result == 0);

	// Expose socket
	result = listenSocket->Listen();
	assert(result == 0);

	// Print what port it is listening on
	std::cout << "Listening on port: " << listenSocket->GetPortNumber() << std::endl;
}

/// <summary>
/// Called once per frame. Should check listenSocket for new connections.
/// If any come in, should add them to the list of openConnections, and
/// should send a message to each peer listing the addresses
/// of every other peer.
/// 
/// If there is a new connection, log some info about it with
/// messageLog.AddMessage().
/// </summary>
void NetworkManager::CheckForNewConnections()
{
	SocketAddress clientAddress;
	TCPSocketPtr connectionSocket = listenSocket->Accept(clientAddress);
	int error = SocketUtil::GetLastError();
	if (connectionSocket == nullptr) { // no new connection
		if (error != WSAEWOULDBLOCK) { // check to see if something went wrong
			SocketUtil::ReportError("listenSocket->Accept()");
			assert(false);
		}
	}
	else { // Accepted connection
		// set up connection's non-blocking
		int result = connectionSocket->SetNonBlockingMode(true);
		assert(result == 0);

		// add to list of open connections
		openConnections[clientAddress] = connectionSocket;

		// Send message to peers (don't need to do)

		// log some information about the new connection
		std::string message = "New connection from " + clientAddress.ToString() + "\n";
		messageLog.AddMessage(message);
	}
}

/// <summary>
/// Sends the provided message to every connected peer. Called
/// whenever the user presses enter.
/// </summary>
/// <param name="message">Message to send</param>
void NetworkManager::SendMessageToPeers(const std::string& message)
{
	for (auto connection : openConnections) {
		int32_t len = connection.second->Send(message.c_str(), message.length());
		if (len < 0) {
			if (len == -WSAEWOULDBLOCK) {
				len = connection.second->Send(message.c_str(), message.length());
			}
			else {
				SocketUtil::ReportError("Send");
			}
		}
	}
}

void NetworkManager::PostMessagesFromPeers()
{
	for (auto connection : openConnections) {
		const size_t BUFLEN = 4096;
		char buffer[BUFLEN];
		int32_t len = connection.second->Receive(buffer, BUFLEN);
		if (len < 0) {
			if (len == -WSAEWOULDBLOCK)
			{
				//dont care
			}
			else
			{
				SocketUtil::ReportError("Receive");
			}
		}
		else if (len > 0) {
			std::string message(buffer, len);
			messageLog.AddMessage(connection.first.ToString() + ": " + message);
		}
	}
}

/// <summary>
/// Try to connect to the given address.
/// </summary>
/// <param name="targetAddress">The address to try to connect to.</param>
void NetworkManager::AttemptToConnect(SocketAddressPtr targetAddress)
{
	// create client socket
	TCPSocketPtr clientSocket = SocketUtil::CreateTCPSocket(SocketAddressFamily::INET);
	assert(clientSocket != nullptr);

	// create address for client
	SocketAddressPtr address = SocketAddressFactory::CreateIPv4FromString("127.0.0.1");

	// bind client address
	int result = clientSocket->Bind(*address);
	assert(result == 0);

	// connect
	clientSocket->SetNonBlockingMode(false);
	result = clientSocket->Connect(*targetAddress);
	assert(result == 0);
	clientSocket->SetNonBlockingMode(true);

	// save connection
	openConnections[*targetAddress] = clientSocket;
}
