all:
	g++ main.cpp -O2 -o traffic_dumper

debug:
	g++ main.cpp -O0 -g -o traffic_dumper

clean:
	rm traffic_dumper
