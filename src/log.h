#pragma once

#include <queue>
#include <string>
#include <SFML/Graphics.hpp>

class Log
{
public:
	enum eMessageTypes {
		Announcement,
		Standard,
		Alert,
		Damage,
	};
	struct message {
		std::string text;
		eMessageTypes type;
	};
	static void push(const std::string& str, eMessageTypes type = eMessageTypes::Standard);
	static void run();
private:

	//helper funcs
	static sf::Text createLogText(const std::string& str, eMessageTypes type);
	static std::deque<message> m_queue;
};