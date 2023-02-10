class MessageLog : public std::deque<std::string>
{
public:
	MessageLog(size_t maxRows=20):
		std::deque<std::string>(maxRows, "")
	{}

	void AddMessage(const std::string& message)
	{
		pop_front();
		push_back(message);
	}
};