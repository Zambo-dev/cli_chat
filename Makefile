# Compiler settings
CC = clang
CCFLAGS := -ggdb -lpthread

# Folders
SRC = src
BUILD = build
BIN = bin
CFL := $(BUILD) $(BIN)		# Create Folder List

# Files
EXE = $(BIN)/clichat
CFILES := $(wildcard $(SRC)/*.c)
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
	$(EXE) s 4444
client: $(CFL) $(EXE)
	$(EXE) c 127.0.0.1 4444

clean:
	rm $(BIN)/* $(BUILD)/*

cleanall:
	rm -r $(BIN)/ $(BUILD)/ 
