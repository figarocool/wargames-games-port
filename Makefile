all:
	$(MAKE) -C rebuild
	cp rebuild/wargame-c ./wargame-linux

clean:
	$(MAKE) -C rebuild clean
	rm -f wargame-linux

.PHONY: all clean
