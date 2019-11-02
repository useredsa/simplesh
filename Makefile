#
# 'make'        build executable file
# 'make clean'  removes all .o and executable files
# 'make cleandep' deletes all .d files
#

### Define according to folder structure ###
INCDIR=include#	headers folder
SRCDIR=src#		source folder/s and file extension
BUILDDIR=build#	object files folder
DEPDIR=build#	dependencies folder
BINDIR=bin#		binaries folder

#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
# define the C source files
SRCS=$(shell find $(SRCDIR) -type f -name "*.c")
# define the C object files 
OBJS=$(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SRCS:.c=.o))
# define the auto-generated dependency files
DEPS=$(patsubst $(SRCDIR)/%,$(DEPDIR)/%,$(SRCS:.c=.d))

### Define compiling process ###
CC=cc	# C compiler
CFLAGS=-ggdb3 -Wall -Werror -Wno-unused -std=c11 # Compile-time flags
INC=-I $(INCDIR)
CPPFLAGS=CFLAGS
LDLIBS=-lreadline

# define the main files
TARGET=$(BINDIR)/simplesh

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(INC) $(LDLIBS) -o $(TARGET)

# Include dependencies files
-include $(DEPS)

# rule for .o files
#$(BUILDDIR)/%.o: $(SRCDIR)/%.c
#	$(CC) $(CFLAGS) $(INC) $(LDLIBS) -c $< -o $@

# rule to generate a dep file by using the C preprocessor
# (see man cpp for details on the -MM and -MT options)
$(DEPDIR)/%.d: $(SRCDIR)/%.c
	($(CPP) $(CFLAGS) $(INC) $(LDLIBS) $< -MM -MT $(@:.d=.o); echo '\t$(CC) $(CFLAGS) $(INC) $(LDLIBS) -c $< -o $(@:.d=.o)';) >$@

.PHONY: clean cleandep

clean:
	rm -f $(OBJS) $(TARGET)

cleandep:
	rm -f $(DEPS)
