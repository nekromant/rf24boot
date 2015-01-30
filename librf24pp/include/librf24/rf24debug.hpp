#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>


namespace librf24 { 
	class LibRF24DebugStream: public std::ostringstream
	{
	public:
		typedef
		enum
		{
			silent,   
			error, 
			warning,
			info,   
			debug, 
			invalid,
		} logLevel;
		
	private:

		logLevel mLogLevel;
		logLevel mCurrentLogLevel;
		std::string prefix = "none";
		const char * logLevelStrings[invalid] = { 
			"", 
			"error", 
			"warning",
			"info",
			"debug",
		};

	protected:
		friend class LibRF24Debuggable;
		std::ostream *out = &std::cerr;
				
	public:
	
		void flush();
		LibRF24DebugStream(std::ostream *logStream, const char *p, logLevel level=error) :
			mLogLevel(level),
			mCurrentLogLevel(debug),
			out(logStream) { 
			prefix.assign(p);
		}

		LibRF24DebugStream() : LibRF24DebugStream(&std::cerr, "none", error) { };
		
		~LibRF24DebugStream(); 

		
		/* Handle loglevel changes */
		inline LibRF24DebugStream & operator<<(const logLevel & level) { 
			mCurrentLogLevel = level; 
			return *this;
		}
		
		inline const char *endl() { 
			*out << std::endl;
			flush();
			return "";
		}
		
		inline void setPrefix(const char *prefix)
			{
				this->prefix.assign(prefix);
			}

		inline void setCurrentLevel(logLevel level) 
			{ 
				mCurrentLogLevel = level; 
			}
		
		inline const logLevel getCurrentLevel()  
			{ 
				return mCurrentLogLevel; 
			}
		
		inline void setLogLevel(logLevel level) 
			{
				mLogLevel = level; 
			}
		
		inline const logLevel getLogLevel() 
			{ 
				return mLogLevel; 
			}
	};

	class LibRF24Debuggable 
	{
	protected:
		LibRF24DebugStream dbg;
		inline void setDebuggingPrefix(const char *prefix)
			{
				dbg.setPrefix(prefix);
			}
	public:
		inline void setDebugLevel(LibRF24DebugStream::logLevel lvl) 
			{
				dbg.setLogLevel(lvl);
			}

		void setDebugStream(std::ostream *o); 
		
		inline LibRF24DebugStream::logLevel getDebugLevel() 
			{
				return dbg.getLogLevel();
			}	
	};
};

