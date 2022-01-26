CXX         := g++

BIN         := ../bin
SRC         := ../src
INCLUDE     := ../headers

LIBRARIES   :=
GENERATE	:= move_generator_wrapper
APPLY		:= move_application_wrapper

all: build
build: clean
		mkdir -p ${BIN}
		${CXX} ${CXX_FLAGS} -I${INCLUDE} ${GENERATE}.cpp -o ${BIN}/${GENERATE} $(LIBRARIES)
		${CXX} ${CXX_FLAGS} -I${INCLUDE} ${APPLY}.cpp -o ${BIN}/${APPLY} $(LIBRARIES)
clean:
		rm -rf ${BIN_DIR}