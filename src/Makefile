CFLAGS=-pthread -lvirt -g
OBJS=csapp.o main.o libvirt.o
Kserver:$(OBJS)
	gcc $(OBJS) -o $@ $(CFLAGS)
clean: $(OBJS)
	$(RM) -rf $(OBJS) Kserver
