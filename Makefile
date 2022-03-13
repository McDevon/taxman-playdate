HEAP_SIZE      = 8388208
STACK_SIZE     = 61800

PRODUCT = Taxman.pdx

# Locate the SDK
SDK = ${PLAYDATE_SDK_PATH}
ifeq ($(SDK),)
	SDK = $(shell egrep '^\s*SDKRoot' ~/.Playdate/config | head -n 1 | cut -c9-)
endif

ifeq ($(SDK),)
$(error SDK path not found; set ENV value PLAYDATE_SDK_PATH)
endif

CSRC = $(wildcard src/*.c) \
       $(wildcard taxman-engine/Engine/Actions/*.c) \
       $(wildcard taxman-engine/Engine/Components/*.c) \
       $(wildcard taxman-engine/Engine/Logic/*.c) \
       $(wildcard taxman-engine/Engine/Math/*.c) \
       $(wildcard taxman-engine/Engine/Render/*.c) \
       $(wildcard taxman-engine/Engine/Resources/*.c) \
       $(wildcard taxman-engine/Engine/Scene/*.c) \
       $(wildcard taxman-engine/Engine/Strings/*.c) \
       $(wildcard taxman-engine/Engine/Tests/*.c) \
       $(wildcard taxman-engine/Engine/Utils/*.c) \
       $(wildcard taxman-engine/Tools/Camera/*.c) \
       $(wildcard taxman-engine/Tools/Components/*.c) \
       $(wildcard taxman-engine/Tools/Physics/*.c) \
       $(wildcard taxman-engine/Tools/Tilemap/*.c) \
       $(wildcard game/*.c)

######
# IMPORTANT: You must add your source folders to VPATH for make to find them
# ex: VPATH += src1:src2
######

VPATH += src

# List C source files here
SRC = $(CSRC)

# List all user directories here
UINCDIR = src taxman-engine/Engine/Actions taxman-engine/Engine/Components taxman-engine/Engine/Logic taxman-engine/Engine/Math taxman-engine/Engine/Render taxman-engine/Engine/Resources taxman-engine/Engine/Scene taxman-engine/Engine/Strings taxman-engine/Engine/Tests taxman-engine/Engine/Utils taxman-engine/Tools/Camera taxman-engine/Tools/Components taxman-engine/Tools/Physics taxman-engine/Tools/Tilemap game

# List user asm files
UASRC = 

# List all user C define here, like -D_DEBUG=1
UDEFS =

# Define ASM defines here
UADEFS =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

include $(SDK)/C_API/buildsupport/common.mk

