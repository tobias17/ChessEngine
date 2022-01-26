CXX         := g++

BIN         := bin
SRC         := src
INCLUDE     := headers

LIBRARIES   :=
DEBUG	    := ${BIN}/main
RELEASE	    := ${BIN}/release
ENTRY_FILE  := ${SRC}/main.cpp

all: build
build: clean
		mkdir -p ${BIN}
		${CXX} ${CXX_FLAGS} -I${INCLUDE} ${ENTRY_FILE} -o ${DEBUG} $(LIBRARIES)
		${CXX} ${CXX_FLAGS} -I${INCLUDE} ${ENTRY_FILE} -o ${RELEASE} $(LIBRARIES) -D NDEBUG
run: build
		# clear
		./${DEBUG}
clean:
		rm -rf ${BIN_DIR}