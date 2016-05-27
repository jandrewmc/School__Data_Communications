CC := g++ -c
LINKER := g++ -o	

XMTROBJ := Transmitter.o
RCVROBJ := Receiver.o

XMTRTARGET := xmtr
RCVRTARGET := rcvr

SRCDIR := src
OBJDIR := obj

default: $(XMTRTARGET) $(RCVRTARGET) 

#create executable files
$(XMTRTARGET): $(OBJDIR)/$(XMTROBJ)
	$(LINKER) $(XMTRTARGET) $(OBJDIR)/$(XMTROBJ)

$(RCVRTARGET): $(OBJDIR)/$(RCVROBJ)
	$(LINKER) $(RCVRTARGET) $(OBJDIR)/$(RCVROBJ)

#create .o files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) $< -o $@

clean:
	rm -rf $(OBJDIR);
	mkdir $(OBJDIR);
	rm $(XMTRTARGET);
	rm $(RCVRTARGET);
