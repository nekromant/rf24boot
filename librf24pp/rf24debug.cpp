#include <librf24/rf24debug.hpp>

using namespace librf24;

void LibRF24DebugStream::flush()
{
	if (mCurrentLogLevel <= mLogLevel)
	{
		*out << prefix << "/" << logLevelStrings[mCurrentLogLevel] << ": ";
		*out << str();
		out->flush();
	}
	str("");
	mCurrentLogLevel=debug;
}

LibRF24DebugStream::~LibRF24DebugStream() 
{ 
	flush();
}



void LibRF24Debuggable::setDebugStream(std::ostream *o)
{
	dbg.out = o;
}
