#pragma once

enum class ClientMessage
{
	Hello,
	Input,
	Ping
};

enum class ServerMessage
{
	Welcome,
	Unwelcome,
	Ping, 
	Replication
	
};
