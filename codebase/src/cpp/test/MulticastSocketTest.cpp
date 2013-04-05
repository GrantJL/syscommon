/*
 * The contents of this file are subject to the terms of the Common Development
 * and Distribution License (the "License"). You may not use this file except in
 * compliance with the License. You can obtain a copy of the license at
 * SysCommon/license.html or http://www.sun.com/cddl/cddl.html. See the License
 * for the specific language governing permissions and limitations under the
 * License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each file and
 * include the License file at SysCommon/license.html.
 * If applicable, add the following below this CDDL HEADER, with the fields
 * enclosed by brackets "[]" replaced with your own identifying information:
 * Portions Copyright [yyyy] [name of copyright owner]
 */
#include "MulticastSocketTest.h"
#include "net/MulticastSocket.h"
#include "Platform.h"

CPPUNIT_TEST_SUITE_REGISTRATION( MulticastSocketTest );

//----------------------------------------------------------
//                      CONSTRUCTORS
//----------------------------------------------------------
MulticastSocketTest::MulticastSocketTest()
{

}

MulticastSocketTest::~MulticastSocketTest()
{

}

//----------------------------------------------------------
//                    INSTANCE METHODS
//----------------------------------------------------------
void MulticastSocketTest::setUp()
{

}

void MulticastSocketTest::tearDown()
{

}

void MulticastSocketTest::testPortConstructor()
{
	try
	{
		SysCommon::MulticastSocket mcastSocket( 3030 );
		CPPUNIT_ASSERT( mcastSocket.isBound() );

		mcastSocket.close();
	}
	catch( std::exception& e )
	{
		failTest( "Received exception while creating socket. Error: %s",
				  e.what() );
	}
}

void MulticastSocketTest::testIfaceConstructor()
{
	SysCommon::InetSocketAddress networkIface( INADDR_LOOPBACK, 3033 );

	try
	{
		SysCommon::MulticastSocket mcastSocket( networkIface );
		CPPUNIT_ASSERT( mcastSocket.isBound() );

		mcastSocket.close();
	}
	catch( std::exception& e )
	{
		failTest( "Received exception while creating socket. Error: %s",
				  e.what() );
	}
}

void MulticastSocketTest::testIfaceConstructorInvalid()
{
	SysCommon::InetSocketAddress networkIface( "meh.mclol", 3033 );

	try
	{
		SysCommon::MulticastSocket mcastSocket( networkIface );
		failTestMissingException( "SocketException", "binding a socket to an invalid address" );
	}
	catch( SysCommon::SocketException& )
	{

	}
	catch( std::exception& e )
	{
		failTestWrongException( "SocketException", e, "binding a socket to an invalid address" );
	}
}

void MulticastSocketTest::testJoinLeaveGroup()
{
	SysCommon::InetSocketAddress networkIface( INADDR_LOOPBACK, 3033 );
	SysCommon::InetSocketAddress multicastAddress( TEXT("226.0.1.90"), 3033 );

	SysCommon::MulticastSocket mcastSocket( networkIface );

	try
	{
		mcastSocket.joinGroup( multicastAddress.getAddress() );
		mcastSocket.leaveGroup( multicastAddress.getAddress() );
	}
	catch( std::exception& e )
	{
		failTest( "Received exception while joining a multicast group. Error: %s",
				  e.what() );
	}

	mcastSocket.close();
}

void MulticastSocketTest::testJoinOnClosedSocket()
{
	SysCommon::InetSocketAddress networkIface( INADDR_LOOPBACK, 3033 );
	SysCommon::InetSocketAddress multicastAddress( TEXT("226.0.1.90"), 3033 );

	SysCommon::MulticastSocket mcastSocket( networkIface );
	mcastSocket.close();

	try
	{
		mcastSocket.joinGroup( multicastAddress.getAddress() );
		failTestMissingException( "SocketException", "joining on a closed socket" );
	}
	catch( SysCommon::SocketException& )
	{
		// SUCCESS!
	}
	catch( std::exception& e )
	{
		failTestWrongException( "SocketException", e, "joining on a closed socket" );
	}
}

void MulticastSocketTest::testLeaveGroupWithoutJoin()
{
	SysCommon::InetSocketAddress networkIface( INADDR_LOOPBACK, 3033 );
	SysCommon::InetSocketAddress multicastAddress( TEXT("226.0.1.90"), 3033 );

	SysCommon::MulticastSocket mcastSocket( networkIface );

	try
	{
		mcastSocket.leaveGroup( multicastAddress.getAddress() );
		failTestMissingException( "SocketException", "leaving a group without joining first" );
	}
	catch( SysCommon::SocketException& )
	{
		// SUCCESS!
	}
	catch( std::exception& e )
	{
		failTestWrongException( "SocketException", e, "leaving a group without joining first" );
	}

	mcastSocket.close();
}

void MulticastSocketTest::testAddressReusage()
{
	SysCommon::InetSocketAddress networkIface( INADDR_ANY, 3033 );
	SysCommon::MulticastSocket socketOne( networkIface );

	try
	{
		SysCommon::MulticastSocket socketTwo( networkIface );
		socketTwo.close();
	}
	catch( std::exception& e )
	{
		failTest( "Unexpected exception while binding a second socket to the same address. Reported error [%s]",
				  e.what() );
	}

	socketOne.close();
}

void MulticastSocketTest::testSendReceive()
{
	SysCommon::InetSocketAddress networkIface( INADDR_ANY, 3033 );
	SysCommon::InetSocketAddress multicastAddress( TEXT("226.0.1.3"), 3033 );

	SysCommon::MulticastSocket sender( networkIface );
	SysCommon::MulticastSocket receiver( networkIface );
	try
	{
		// Join the multicast group
		sender.joinGroup( multicastAddress.getAddress() );
		receiver.joinGroup( multicastAddress.getAddress() );

		// Construct the send buffer and packet
		char sendBuffer[1024];
		const char* sendString = "Hello World";
		size_t stringLength = ::strlen( sendString );
		size_t sendLength = sizeof(size_t) + stringLength;
		::memcpy( sendBuffer, &stringLength, sizeof(size_t) );
		::memcpy( sendBuffer + sizeof(size_t), sendString, stringLength );

		SysCommon::DatagramPacket sendPacket( sendBuffer,
											  0,
											  sendLength,
											  multicastAddress );

		// Construct the receive buffer and packet
		char receiveBuffer[1024];
		::memset( receiveBuffer, 0, sizeof(receiveBuffer) );

		SysCommon::DatagramPacket receivePacket( receiveBuffer, sizeof(receiveBuffer) );

		// Send and Receive
		sender.send( sendPacket );
		receiver.receive( receivePacket );

		// Is the received data the same as what was sent?
		CPPUNIT_ASSERT( (size_t)receivePacket.getLength() == sendLength );
		CPPUNIT_ASSERT( ::memcmp(sendBuffer, receiveBuffer, sendLength) == 0 );

		sender.leaveGroup( multicastAddress.getAddress() );
		receiver.leaveGroup( multicastAddress.getAddress() );
	}
	catch( std::exception& e )
	{
		failTest( "Unexpected exception while sending a datagram. Reported error %s\n",
				  e.what() );
	}

	sender.close();
	receiver.close();
}

void MulticastSocketTest::testSendWhileClosed()
{
	SysCommon::InetSocketAddress networkIface( INADDR_ANY, 3033 );
	SysCommon::InetSocketAddress multicastAddress( TEXT("226.0.1.3"), 3033 );

	// Create the socket, and immediately close it
	SysCommon::MulticastSocket sender( networkIface );
	sender.close();

	// Construct the send buffer and packet
	char sendBuffer[1024];
	const char* sendString = "Hello World";
	size_t stringLength = ::strlen( sendString );
	size_t sendLength = sizeof(size_t) + stringLength;
	::memcpy( sendBuffer, &stringLength, sizeof(size_t) );
	::memcpy( sendBuffer + sizeof(size_t), sendString, stringLength );

	SysCommon::DatagramPacket sendPacket( sendBuffer,
										  0,
										  sendLength,
										  multicastAddress );

	// Attempting to send should fail
	try
	{
		sender.send( sendPacket );
		failTestMissingException( "SocketException", "sending on a closed socket" );
	}
	catch( SysCommon::SocketException& )
	{
		// SUCCESS!
	}
	catch( std::exception& e )
	{
		failTestWrongException( "SocketException", e, "sending on a closed socket" );
	}
}

void MulticastSocketTest::testSendNoAddress()
{
	SysCommon::InetSocketAddress networkIface( INADDR_ANY, 3033 );
	SysCommon::InetSocketAddress multicastAddress( TEXT("226.0.1.3"), 3033 );

	// Create the sender socket
	SysCommon::MulticastSocket sender( networkIface );
	sender.joinGroup( multicastAddress.getAddress() );

	// Construct the send buffer
	char sendBuffer[1024];
	const char* sendString = "Hello World";
	size_t stringLength = ::strlen( sendString );
	size_t sendLength = sizeof(size_t) + stringLength;
	::memcpy( sendBuffer, &stringLength, sizeof(size_t) );
	::memcpy( sendBuffer + sizeof(size_t), sendString, stringLength );

	// Create a packet with no address
	SysCommon::DatagramPacket sendPacket( sendBuffer, sendLength );

	// Attempting to send should fail
	try
	{
		sender.send( sendPacket );
		failTestMissingException( "SocketException", "sending without a destination address" );
	}
	catch( SysCommon::SocketException& )
	{
		// SUCCESS!
	}
	catch( std::exception& e )
	{
		failTestWrongException( "SocketException", e, "sending without a destination address" );
	}

	sender.close();
}

void MulticastSocketTest::testReceiveWhileClosed()
{
	SysCommon::InetSocketAddress networkIface( INADDR_ANY, 3033 );
	SysCommon::InetSocketAddress multicastAddress( TEXT("226.0.1.3"), 3033 );

	// Create the socket, and immediately close it
	SysCommon::MulticastSocket receiver( networkIface );
	receiver.close();

	// Construct the send buffer and packet
	char receiveBuffer[1024];
	SysCommon::DatagramPacket receivePacket( receiveBuffer,
											 1024 );

	// Attempting to receive should fail
	try
	{
		receiver.receive( receivePacket );
		failTestMissingException( "SocketException", "receiving on a closed socket" );
	}
	catch( SysCommon::SocketException& )
	{
		// SUCCESS!
	}
	catch( std::exception& e )
	{
		failTestWrongException( "SocketException", e, "receiving on a closed socket" );
	}
}
