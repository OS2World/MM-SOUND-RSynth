# $Id: Makefile.in,v 1.3 1994/11/08 13:30:50 a904209 Exp a904209 $

VPATH        = /rsynth-2.0
SRCDIR       = /rsynth-2.0
CC           = gcc
PREFIX       = @prefix@
BIN_DIR      = $(PREFIX)/bin
LIB_DIR      = $d:/emx/lib
LDLIBS       = -lgdbm -los2me
XLIBS        = @XLIBS@
CFLAGS       = -g -DOS2 -D__STDC__ -Zmt -I./
CPPFLAGS     = 
DEFS         = 
PROGS        = say.exe mkdictdb.exe  dlookup.exe
ADICT        = @ADICT@
BDICT        = @BDICT@
DICTS        = 
INSTALL      = @INSTALL@
INSTALL_PROG = @INSTALL_PROGRAM@
INSTALL_DATA = @INSTALL_DATA@
MESSAGE      = Hello there

.c.o :
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DEFS) -O -c -o $@ $<

all: $(PROGS) $(DICTS)

check : all
	(./say -W $(MESSAGE) || ./nasay $(MESSAGE))

# SAY_OBJS = say.o saynum.o darray.o ASCII.o text.o english.o trie.o phtoelm.o holmes.o elements.o nsynth.o def_pars.o hplay.o dict.o getarg.o
SAY_OBJS = say.o saynum.o darray.o ASCII.o phones.o text.o english.o trie.o phtoelm.o \
           holmes.o elements.o nsynth.o def_pars.o l2u.o aufile.o  dict.o \
           getarg.o Revision.o 

say.exe  : $(SAY_OBJS) hplay.o 
	$(CC) -Zcrtdll -Zmt -s -o $@ $(LDFLAGS) $(SAY_OBJS) hplay.o $(LDLIBS)

nasay : $(SAY_OBJS) naplay.o 
	$(CC) -o $@ $(LDFLAGS) $(SAY_OBJS) naplay.o $(XLIBS) $(LDLIBS)

mkdictdb.exe : mkdictdb.o trie.o phones.o 
	$(CC) -Zcrtdll -s -o $@ $(LDFLAGS) mkdictdb.o phones.o trie.o $(LDLIBS)

dlookup.exe  : dlookup.o phones.o dict.o getarg.o  
	$(CC) -Zcrtdll -s -o $@ $(LDFLAGS) dlookup.o phones.o dict.o getarg.o $(LDLIBS)

ssay : $(SAY_OBJS) hplay.o 
	$(SE_HOME)/sentinel $(CC) -o $@ $(LDFLAGS) $(SAY_OBJS) hplay.o $(LDLIBS)

clean:
	rm -f $(PROGS) $(DICTS) *.o tmp.par core *~ *% ~* ,* .e[cks]1

distclean : clean
	rm -f Makefile config.status config.h config.log config.cache hplay.c 


hplay.o : hplay.c proto.h getargs.h hplay.h 

aDict.db : $(ADICT) mkdictdb
	mkdictdb $(ADICT) aDict.db  

bDict.db : $(BDICT) mkdictdb
	mkdictdb $(BDICT) bDict.db  

install  : $(PROGS) $(DICTS)
	for f in $(PROGS); do \
	$(INSTALL_PROG) $$f $(BIN_DIR); \
	done
	$(INSTALL) -d $(LIB_DIR)
	for f in $(DICTS); do \
	$(INSTALL_DATA) $$f $(LIB_DIR); \
	done



# Now some - semi-automatically generated dependancies
#MM
ASCII.o : ASCII.c ASCII.h 
Revision.o : Revision.c 
au.o : au.c proto.h au.h 
aufile.o : aufile.c proto.h getargs.h l2u.h hplay.h file.h 
darray.o : darray.c proto.h darray.h 
def_pars.o : def_pars.c proto.h getargs.h nsynth.h hplay.h pars.def 
dict.o : dict.c proto.h phones.h phones.def dict.h getargs.h 
dlookup.o : dlookup.c proto.h dict.h phones.h phones.def getargs.h 
elements.o : elements.c elements.h phfeat.h Elements.def 
english.o : english.c 
getarg.o : getarg.c proto.h getargs.h 
holmes.o : holmes.c proto.h nsynth.h elements.h darray.h holmes.h phfeat.h getargs.h 
klatt.o : klatt.c proto.h parwave.h 
l2u.o : l2u.c 
mkdictdb.o : mkdictdb.c proto.h trie.h darray.h phones.h phones.def 
naplay.o : naplay.c proto.h getargs.h hplay.h 
netplay.o : netplay.c proto.h getargs.h hplay.h 
nsynth.o : nsynth.c proto.h nsynth.h 
parwave.o : parwave.c proto.h parwave.h 
phones.o : phones.c phones.h phones.def 
phtoelm.o : phtoelm.c proto.h elements.h phfeat.h darray.h trie.h phtoelm.h \
  hplay.h holmes.h nsynth.h phtoelm.def 
plot.o : plot.c proto.h elements.h 
say.o : say.c proto.h nsynth.h hplay.h dict.h ASCII.h darray.h holmes.h phtoelm.h \
  text.h getargs.h phones.h phones.def file.h say.h 
saynum.o : saynum.c proto.h darray.h say.h 
text.o : text.c proto.h darray.h phtoelm.h text.h say.h 
tidy_elm.o : tidy_elm.c proto.h darray.h elements.h parwave.h phfeat.h phtoelm.h \
  phonemes.def 
trie.o : trie.c proto.h trie.h 
