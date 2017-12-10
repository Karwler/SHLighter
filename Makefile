SHLighter: src/shlighter_sender.c
	mkdir build
	gcc -lSDL2 -lSDL2_net -o build/shlighter_sender src/shlighter_sender.c
	cp src/shlighter_receiver.py build
	chmod +x build/shlighter_receiver.py
