xmairleveltest: main.cpp xmairleveltester.cpp xrm32level.hpp
	g++ -std=c++1y -oxmairleveltest main.cpp xmairleveltester.cpp -I$(HOME)/local/include -L$(HOME)/local/lib -llo -pthread
