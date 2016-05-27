CC := g++ -c
LINKER := g++ -o	

XMTRTARGET := xmtr
RCVRTARGET := rcvr

XMTRSRCDIR := src/Transmitter
RCVRSRCDIR := src/Receiver

OBJDIR := obj

default: $(RCVRTARGET) $(XMTRTARGET)

$(XMTRTARGET): $(OBJDIR)/XMTRApplication.o $(OBJDIR)/XMTRDataLink.o $(OBJDIR)/XMTRPhysical.o
	$(LINKER) $(XMTRTARGET) $(OBJDIR)/XMTRApplication.o $(OBJDIR)/XMTRDataLink.o $(OBJDIR)/XMTRPhysical.o

$(RCVRTARGET): $(OBJDIR)/RCVRApplication.o $(OBJDIR)/RCVRDataLink.o $(OBJDIR)/RCVRPhysical.o
	$(LINKER) $(RCVRTARGET) $(OBJDIR)/RCVRApplication.o $(OBJDIR)/RCVRDataLink.o $(OBJDIR)/RCVRPhysical.o

#create .o files
$(OBJDIR)/%.o: $(RCVRSRCDIR)/%.cpp
	$(CC) $< -o $@

$(OBJDIR)/%.o: $(XMTRSRCDIR)/%.cpp
	$(CC) $< -o $@

clean:
	rm -rf $(OBJDIR);
	mkdir $(OBJDIR);
	rm $(XMTRTARGET);
	rm $(RCVRTARGET);
