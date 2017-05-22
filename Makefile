xmairleveltest: main.cpp xmairleveltester.cpp xrm32level.hpp
	g++ -std=c++1y -oxmairleveltest main.cpp xmairleveltester.cpp -I/home/fex/local/include -L/home/fex/local/lib -llo -pthread
