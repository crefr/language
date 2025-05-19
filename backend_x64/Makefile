FILENAME = backend_x64.exe
OBJDIR 		   = Obj/
SRCDIR 		   = sources/
HEADDIR 	   = headers/

GLOBALOBJDIR   = Obj/
GLOBALSRCDIR   = ../global/sources/
GLOBALHEADDIR  = ../global/headers/

CC = g++
BUILD  = LINUX
# windows
CFLAGS_WINDOWS =-Wshadow -Winit-self -Wredundant-decls -Wcast-align -Wundef -Wfloat-equal -Winline						\
		-Wunreachable-code -Wmissing-declarations -Wmissing-include-dirs -Wswitch-enum -Wswitch-default					\
		-Weffc++ -Wmain -Wextra -Wall -g -pipe -fexceptions -Wcast-qual -Wconversion -Wctor-dtor-privacy 				\
		-Wempty-body -Wformat-security -Wformat=2 -Wignored-qualifiers -Wlogical-op -Wno-missing-field-initializers 	\
		-Wnon-virtual-dtor -Woverloaded-virtual -Wpointer-arith -Wsign-promo -Wstack-usage=8192 -Wstrict-aliasing 		\
		-Wstrict-null-sentinel -Wtype-limits -Wwrite-strings -Werror=vla -D_DEBUG -D_EJUDGE_CLIENT_SIDE

# linux
CFLAGS_LINUX = -D _DEBUG -ggdb3 -std=c++17 -O0 -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations 						\
		-Wc++14-compat -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts -Wconditionally-supported 			\
		-Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security 					\
		-Wformat-signedness -Wformat=2 -Winline -Wlogical-op -Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual 			\
		-Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo 						\
		-Wstrict-null-sentinel -Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods 					\
		-Wsuggest-final-types -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code 		\
		-Wunused -Wuseless-cast -Wvariadic-macros -Wno-literal-suffix -Wno-missing-field-initializers -Wno-narrowing 		\
		-Wno-old-style-cast -Wno-varargs -Wstack-protector -fcheck-new -fsized-deallocation -fstack-protector 				\
		-fstrict-overflow -flto-odr-type-merging -fno-omit-frame-pointer -Wlarger-than=8192 -Wstack-usage=8192 -pie -fPIE	\
		-Werror=vla -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr

# release
CFLAGS_RELEASE = -O3

ifeq ($(BUILD),WIN)
	CFLAGS = $(CFLAGS_WINDOWS)
else ifeq ($(BUILD),LINUX)
	CFLAGS = $(CFLAGS_LINUX)
else ifeq ($(BUILD),RELEASE)
	CFLAGS = $(CFLAGS_RELEASE)
endif

CFLAGS := -I./$(HEADDIR) -I./$(GLOBALHEADDIR) $(CFLAGS)

GLOBALDEPS = $(GLOBALHEADDIR)logger.h $(GLOBALHEADDIR)hashtable.h $(GLOBALHEADDIR)tree.h $(GLOBALHEADDIR)IR_handler.h
LOCALDEPS  = $(HEADDIR)backend_x64.h $(HEADDIR)x64_compile.h

ALLDEPS    = $(LOCALDEPS) $(GLOBALDEPS)

LOCAL_OBJECTS  = main.o backend_x64.o x64_compile.o
LOCAL_OBJECTS_WITH_DIR = $(addprefix $(OBJDIR),$(LOCAL_OBJECTS))

GLOBAL_OBJECTS = logger.o tree.o IR_handler.o
GLOBAL_OBJECTS_WITH_DIR = $(addprefix $(GLOBALOBJDIR),$(GLOBAL_OBJECTS))

# TABLELIB = ../hash-table/Obj/hashtable.a
# TABLELIBFOLDER = ../hash-table/

$(FILENAME): $(LOCAL_OBJECTS_WITH_DIR) $(GLOBAL_OBJECTS_WITH_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(LOCAL_OBJECTS_WITH_DIR): $(OBJDIR)%.o: $(SRCDIR)%.c $(ALLDEPS)
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(GLOBAL_OBJECTS_WITH_DIR): $(GLOBALOBJDIR)%.o: $(GLOBALSRCDIR)%.c $(ALLDEPS)
	mkdir -p $(GLOBALOBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TABLELIB):
	cd $(TABLELIBFOLDER) && make static_lib

clean:
	rm $(OBJDIR)*

run:
	./$(FILENAME)
