# Compiler settings
CC = cc
CCFLAGS := -Wall -ggdb -lssl -lcrypto -lpthread

# Folders
SRC = src
BUILD = build
BIN = bin
CFL := $(BUILD) $(BIN)		# Create Folder List

# Files
EXE = $(BIN)/clichat
CFILES := main.c sock.c conf.c err.c client.c server.c
OFILES := $(addprefix $(BUILD)/, $(notdir $(CFILES:.c=.o)))

# Aesthetics
GREEN = \033[0;32m
RESET = \033[0m

# Compile
all: $(CFL) $(EXE)

$(EXE): $(OFILES)
	@echo -n 'Creating executable $@: '
	@ $(CC) -o $@ $^ $(CCFLAGS)
	@echo -e '		$(GREEN)Done$(RESET)'

$(BUILD)/%.o: $(SRC)/%.c
	@echo -n 'Building object $<: '
	@ $(CC) -o $@ -c $^
	@echo -e '			$(GREEN)Done$(RESET)'

# Others
$(CFL):
ifeq ("$(wildcard $@)", "")
	@echo -n 'Creating $@ folder: '
	@ mkdir $@
	@echo -e '				$(GREEN)Done$(RESET)'
endif

server: $(CFL) $(EXE)
	$(EXE) -t s
client: $(CFL) $(EXE)
	$(EXE) -t c

clean:
	rm $(BIN)/* $(BUILD)/*

cleanall:
	rm -r $(BIN)/ $(BUILD)/ 
