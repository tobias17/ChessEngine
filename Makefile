CXX         := g++

BIN         := bin
SRC         := src
INCLUDE     := headers

LIBRARIES   :=
DEBUG	    := ${BIN}/main
RELEASE	    := ${BIN}/release
TIMING 		:= ${BIN}/timing
ENTRY_FILE  := ${SRC}/main.cpp

all: build
build: clean
		mkdir -p ${BIN}
		${CXX} ${CXX_FLAGS} -I${INCLUDE} ${ENTRY_FILE} -o ${DEBUG} $(LIBRARIES)
		${CXX} ${CXX_FLAGS} -I${INCLUDE} ${ENTRY_FILE} -o ${RELEASE} $(LIBRARIES) -D NDEBUG
		${CXX} ${CXX_FLAGS} -I${INCLUDE} tests/timing.cpp -o ${TIMING} $(LIBRARIES) -D NDEBUG
run: build
		# clear
		./${DEBUG}
time: build
		./${TIMING}
clean:
		rm -rf ${BIN_DIR}