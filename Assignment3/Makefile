all: run

clean:
	rm -rf files graphs

init: clean
	mkdir files files/inputs/ files/outputs/ files/logs
	g++ graph_generator.cpp -o graphs

run: init
	./graphs
	python runner.py -n 8 -f files/inputs/infile1.txt -o files/outputs/output1 -t 11 > files/logs/log1.txt
	python runner.py -n 8 -f files/inputs/infile2.txt -o files/outputs/output2 -t 11 > files/logs/log2.txt
