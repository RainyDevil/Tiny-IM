# Makefile for ChatServer project

PROJECT_NAME = ChatServer

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinc -I$(BOOST_INCLUDE_DIR)

LDFLAGS = -lsqlite3 -lssl -lcrypto -lboost_system -lboost_thread -lpthread

BUILD_TYPE ?= Debug

ifeq ($(BUILD_TYPE), Debug)
    CXXFLAGS += -g -O0
else ifeq ($(BUILD_TYPE), Release)
    CXXFLAGS += -O2
endif

SRC_DIR = src
OBJ_DIR = build
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

TARGET = $(PROJECT_NAME)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET)


.PHONY: all clean
